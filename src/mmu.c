#include <asm/pgtable.h>
#include <asm/pgtable_prot.h>
#include <asm/pgtable_hwdef.h>
#include <sysregs.h>
#include <asm/barrier.h>
#include <string.h>
#include <asm/base.h>

#define NO_BLOCK_MAPPINGS BIT(0)
#define NO_CONT_MAPPINGS BIT(1)


extern char idmap_pg_dir[];

extern char _text_boot[], _etext_boot[];
extern char _text[], _etext[];

static void alloc_init_pte(pmd_t *pmdp, unsigned long addr,
		unsigned long end, unsigned long phys,
		unsigned long prot,
		unsigned long (*alloc_pgtable)(void),
		unsigned long flags)
{
	pmd_t pmd = *pmdp;
	pte_t *ptep;

	if (pmd_none(pmd)) {
		unsigned long pte_phys;

		pte_phys = alloc_pgtable();
		set_pmd(pmdp, __pmd(pte_phys | PMD_TYPE_TABLE));
		pmd = *pmdp;
	}

	ptep = pte_offset_phys(pmdp, addr);
	do {
		set_pte(ptep, pfn_pte(phys >> PAGE_SHIFT, prot));
		phys += PAGE_SIZE;

	}while(ptep++, addr += PAGE_SIZE, addr != end);
	
}

void pmd_set_section(pmd_t *pmdp, unsigned long phys, unsigned long prot)
{
	unsigned long sect_prot = PMD_TYPE_SECT | mk_sect_prot(prot);

	pmd_t new_pmd = pfn_pmd(phys >> PMD_SHIFT, sect_prot);

	set_pmd(pmdp, new_pmd);
}


static void alloc_init_pmd(pud_t *pudp, unsigned long addr,
		unsigned long end, unsigned long phys,
		unsigned long prot,
		unsigned long (*alloc_pgtable)(void),
		unsigned long flags)
{
	pud_t pud = *pudp;
	pmd_t *pmdp;
	unsigned long next;

	if(pud_none(pud)){
		unsigned long pmd_phys;

		pmd_phys = alloc_pgtable();
		set_pud(pudp, __pud(pmd_phys | PUD_TYPE_TABLE));
		pud = *pudp;
	}

	pmdp = pmd_offset_phys(pudp, addr);
	do{
		next = pmd_addr_end(addr, end);
		
		if (((addr | next | phys) & ~SECTION_MASK) == 0 &&
				(flags & NO_BLOCK_MAPPINGS) == 0)
			pmd_set_section(pmdp, phys, prot);
		else
			alloc_init_pte(pmdp, addr, next, phys,
							prot, alloc_pgtable, flags);
		phys += next - addr;
			
	}while(pmdp++, addr = next, addr != end);
	
}


static void alloc_init_pud(pgd_t *pgdp, unsigned long addr,
		unsigned long end, unsigned long phys,
		unsigned long prot,
		unsigned long (*alloc_pgtable)(void),
		unsigned long flags)
{
	pgd_t pgd = *pgdp;
	pud_t *pudp;
	unsigned long next;

	if(pgd_none(pgd)){
		unsigned long pud_phys;

		pud_phys = alloc_pgtable();
		set_pgd(pgdp, __pgd(pud_phys | PUD_TYPE_TABLE));
		pgd = *pgdp;
	}

	pudp = pud_offset_phys(pgdp, addr);
	do{
		next = pud_addr_end(addr, end);
		alloc_init_pmd(pudp, addr, next, phys, prot,
						alloc_pgtable, flags);
		phys += next - addr;

	}while(pudp++, addr = next, addr != end);

	
	
}

static void __create_pgd_mapping(pgd_t *pgdir, unsigned long phys,
		unsigned long virt, unsigned long size,
		unsigned long prot,
		unsigned long (*alloc_pgtable)(void),
		unsigned long flags)
{
	pgd_t *pgdp = pgd_offset_raw(pgdir, virt);
	unsigned long addr, end, next;

	phys &= PAGE_MASK;
	addr = virt & PAGE_MASK;
	end = PAGE_ALIGN(virt + size);

	do{
		next = pgd_addr_end(addr, end);
		alloc_init_pud(pgdp, addr, next, phys,
					prot, alloc_pgtable, flags);
		phys += next - addr;

	}while(pgdp++, addr = next, addr != end);
}

static unsigned long early_pgtable_alloc(void)
{
	unsigned long phys;

	phys = get_free_page();
	memset((void *)phys, 0, PAGE_SIZE);
	return phys;
}


static void create_identical_mapping(void)
{
	unsigned long start =(unsigned long ) _text_boot;
	unsigned long end =(unsigned long ) _etext;
	
	__create_pgd_mapping((pgd_t *)idmap_pg_dir, start, start, end - start,
							PAGE_KERNEL_ROX, early_pgtable_alloc, 0);

	start =PAGE_ALIGN((unsigned long )_etext);
	end = TOTAL_MEMORY;

	__create_pgd_mapping((pgd_t *)idmap_pg_dir, start, start, end - start, 
							PAGE_KERNEL, early_pgtable_alloc, 0);
}

static void create_mmio_mapping(void)
{
	__create_pgd_mapping((pgd_t *)idmap_pg_dir, PBASE, PBASE, DEVICE_SIZE,
						PROT_DEVICE_nGnRnE, early_pgtable_alloc, 0);
}

static cpu_init(void)
{
	unsigned long mair = 0;
	unsigned long tcr = 0;
	unsigned long tmp, parang;

	asm("tlbi vmalle1");
	dsb(nsh);

	/*
	 * Architectural Feature Access Control Register
	 * This control does not cause any instructions to be trapped
	 */
	write_sysreg(3UL << 20, cpacr_el1);

	/*
	 * Monitor Debug System Control Register
	 * EL0 accesses to the AArch64 DCC registers are trapped
	*/
	write_sysreg(1 << 12, mdscr_el1);

	/*?????????????????????????????????mair_el1?????????       arm-v8 P3335*/
	mair = MAIR(0x00UL, MT_DEVICE_nGnRnE) |
			MAIR(0x04UL, MT_DEVICE_nGnRE) |
			MAIR(0x0CUL, MT_DEVICE_GRE) |
			MAIR(0x44UL, MT_NORMAL_NC) |
			MAIR(0xFFUL, MT_NORMAL) |
			MAIR(0xBBUL, MT_NORMAL_WT);
	write_sysreg(mair, mair_el1);

	/*Translation Contr Regist
	 * TG1 TG0?????????????????????????????????4KB or 16KB or 64KB
	 * T1SZ T0SZ ??????TTBR1_EL1 TTBR0_EL1????????????????????????
	 * IPS ????????????????????????48bit or 256TB?????????????????????
	 * IRGN,ORGN:??????normal memory??????????????? write back 
	*/
	tcr = TCR_TxSZ(VA_BITS) | TCR_TG_FLAGS | TCR_CACHE_FLAGS;

	tmp = read_sysreg(ID_AA64MMFR0_EL1);
	parang = tmp & 0xF;
	if(parang > ID_AA64MMFR0_PARANGE_48)
		parang =ID_AA64MMFR0_PARANGE_48;

	tcr |= parang << TCR_IPS_SHIFT;

	write_sysreg(tcr, tcr_el1);
}

static int enable_mmu(void)
{
	unsigned long tmp;
	int tgran4;

	tmp = read_sysreg(ID_AA64MMFR0_EL1);
	tgran4 = (tmp >> ID_AA64MMFR0_TGRAN4_SHIFT) & 0xF;
	if(tgran4 != ID_AA64MMFR0_TGRAN4_SUPPORTED)
		return -1;

	/*
	 * ???????????????16??????1?????????ttbr1_el1???linux??????????????????
	 * ???????????????16??????0?????????ttbr0_el1???linux??????????????????
	*/
	write_sysreg(idmap_pg_dir, ttbr0_el1);
	isb();

	/*
	 * sctlr_el1.M??????????????????mmu
	 * sctlr_el1.C????????????????????????cache
	*/
	write_sysreg(SCTLR_ELx_M | SCTLR_ELx_C, sctlr_el1);
	isb();
	asm("ic iallu");
	dsb(nsh);
	isb();

	return 0;
}


void paging_init(void)
{
	memset(idmap_pg_dir, 0, PAGE_SIZE);
	create_identical_mapping();
	create_mmio_mapping();
	cpu_init();
	enable_mmu();

	printk("enable mmu done\n");

//	dump_pgtable();
}

static int test_access_map_address(void)
{
	unsigned long address = TOTAL_MEMORY - 4096;

	*(unsigned long *)address = 0x55;

	printk("%s access 0x%x done\n", __func__, address);

	return 0;
}

/*
 * ???????????????????????????????????????
 * ??????????????????????????????????????????
 *
 * Translation fault, level 1
 *
 * ???armv8.6?????????2995???
 */
static int test_access_unmap_address(void)
{
	unsigned long address = TOTAL_MEMORY + 4096;

	*(unsigned long *)address = 0x55;

	printk("%s access 0x%x done\n", __func__, address);

	return 0;
}


void test_mmu(void)
{
	test_access_map_address();
	//test_access_unmap_address();
}

