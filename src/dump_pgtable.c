#include <asm/pgtable.h>
#include <asm/pgtable_prot.h>
#include <asm/pgtable_hwdef.h>
#include <sysregs.h>


extern char idmap_pg_dir[];

struct addr_marker {
	unsigned long start_address;
	char *name;
};

struct prot_bits {
	unsigned long mask;
	unsigned long val;
	const char *set;
	const char *clear;
};

struct pg_level {
	const struct prot_bits *bits;
	const char *name;
	size_t num;
	unsigned long mask;
};

static const struct addr_marker address_markers[] = {
	{ 0, "Identical mapping" },
};


static const struct prot_bits pte_bits[] = {
	{
		.mask	= PTE_VALID,
		.val	= PTE_VALID,
		.set	= " ",
		.clear	= "F",
	}, {
		.mask	= PTE_USER,
		.val	= PTE_USER,
		.set	= "USR",
		.clear	= "   ",
	}, {
		.mask	= PTE_RDONLY,
		.val	= PTE_RDONLY,
		.set	= "ro",
		.clear	= "RW",
	}, {
		.mask	= PTE_PXN,
		.val	= PTE_PXN,
		.set	= "NX",
		.clear	= "x ",
	}, {
		.mask	= PTE_SHARED,
		.val	= PTE_SHARED,
		.set	= "SHD",
		.clear	= "   ",
	}, {
		.mask	= PTE_AF,
		.val	= PTE_AF,
		.set	= "AF",
		.clear	= "  ",
	}, {
		.mask	= PTE_NG,
		.val	= PTE_NG,
		.set	= "NG",
		.clear	= "  ",
	}, {
		.mask	= PTE_CONT,
		.val	= PTE_CONT,
		.set	= "CON",
		.clear	= "   ",
	}, {
		.mask	= PTE_TABLE_BIT,
		.val	= PTE_TABLE_BIT,
		.set	= "   ",
		.clear	= "BLK",
	}, {
		.mask	= PTE_UXN,
		.val	= PTE_UXN,
		.set	= "UXN",
	}, {
		.mask	= PTE_ATTRINDX_MASK,
		.val	= PTE_ATTRINDX(MT_DEVICE_nGnRnE),
		.set	= "DEVICE/nGnRnE",
	}, {
		.mask	= PTE_ATTRINDX_MASK,
		.val	= PTE_ATTRINDX(MT_DEVICE_nGnRE),
		.set	= "DEVICE/nGnRE",
	}, {
		.mask	= PTE_ATTRINDX_MASK,
		.val	= PTE_ATTRINDX(MT_DEVICE_GRE),
		.set	= "DEVICE/GRE",
	}, {
		.mask	= PTE_ATTRINDX_MASK,
		.val	= PTE_ATTRINDX(MT_NORMAL_NC),
		.set	= "MEM/NORMAL-NC",
	}, {
		.mask	= PTE_ATTRINDX_MASK,
		.val	= PTE_ATTRINDX(MT_NORMAL),
		.set	= "MEM/NORMAL",
	}, {
		.mask	= PTE_ATTRINDX_MASK,
		.val	= PTE_ATTRINDX(MT_NORMAL_WT),
		.set	= "MEM/NORMAL_WT",
	}
};
static struct pg_level pg_level[] = {
	{
	}, {
		.name	= "PGD",
		.bits	= pte_bits,
		.num	= ARRAY_SIZE(pte_bits),
	}, {
		.name	= "PUD",
		.bits	= pte_bits,
		.num	= ARRAY_SIZE(pte_bits),
	}, {
		.name	= "PMD",
		.bits	= pte_bits,
		.num	= ARRAY_SIZE(pte_bits),
	},{
		.name	= "PTE",
		.bits	= pte_bits,
		.num	= ARRAY_SIZE(pte_bits),
	},
};

static void pg_level_init(void)
{
	int i, j;
	for(i = 0; i < ARRAY_SIZE(pg_level); i ++){
		if(pg_level[i].bits){
			for(j = 0; j < ARRAY_SIZE(pte_bits); j ++)
				pg_level[i].mask |= pg_level[i].bits[j].mask;
		}
	}
}

static void dump_prot(unsigned long prot, const struct prot_bits * bits,
						size_t num)
{
	unsigned int i;

	for(i = 0; i < num; i++, bits++){
		const char *s;

	if((prot & bits->mask) == bits->val)
		s = bits->set;
	else
		s = bits->clear;
	if(s)
		printk(" %s", s);
	}
	
}

static int print_pgtable(unsigned long start, unsigned long end, 
		int level, unsigned long val)
{
	static const char units[] = "KMGT";
	unsigned long prot = val & pg_level[level].mask;
	unsigned long delta;
	const char *unit = units;
	int i;

	printk("pte val= %llx\n", val);
	for(i = 0; i < ARRAY_SIZE(address_markers); i ++){
		if(start == address_markers[i].start_address)
			printk("---[ %s ]---\n", address_markers[i].name);
	}

	if(val == 0)
		return 0;

	printk("0x%016lx - 0x%016lx", start, end);

	delta = (end - start) >> 10;
	while(!(delta & 1023) && units[1]){
		delta >>= 10;
		unit ++;
	}

	printk("%9lu%c %s", delta, *unit, pg_level[level].name);

	if(pg_level[level].bits)
		dump_prot(prot, pg_level[level].bits, pg_level[level].num);
	printk("\n");
	return 0;
}

static void walk_pte(pmd_t * pmdp, unsigned long start, unsigned long end)
{
	pte_t * ptep = pte_offset_phys(pmdp, start);
	unsigned long addr = start;

	do{
		print_pgtable(addr, addr + PAGE_SIZE, 4, pte_val(*ptep));
	}while(ptep++, addr += PAGE_SIZE, addr != end);

}

static void walk_pmd(pud_t *pudp, unsigned long start, unsigned long end)
{
	unsigned long next, addr = start;
	pmd_t *pmdp = pmd_offset_phys(pudp, addr);
	pmd_t pmd_entry;

	do{
		pmd_entry = *pmdp;
		next = pmd_addr_end(addr, end);
		if(pmd_none(pmd_entry) || pmd_sect(pmd_entry))
			print_pgtable(addr, next, 3, pmd_val(pmd_entry));
		else 
			walk_pte(pmdp, addr, next);
	}while(pmdp++, addr = next, next != end);

}

static void walk_pud(pgd_t *pgdp, unsigned long start, unsigned long end)
{
	unsigned long next, addr = start;
	pud_t *pudp = pud_offset_phys(pgdp, addr);
	pud_t pud_entry;

	do{
		pud_entry = *pudp;
		next = pud_addr_end(addr, end);
		if(pud_none(pud_entry) || pud_sect(pud_entry))
			print_pgtable(addr, next, 2, pud_val(pud_entry));
		else 
			walk_pmd(pudp, addr, next);
	}while(pudp++, addr = next, next != end);
}

static void walk_pgd(pgd_t * pgdir, unsigned long start, unsigned long size)
{
	unsigned long end = start + size;
	unsigned long next, addr = start;
	pgd_t * pgdp;
	pgd_t pgd_entry;

	pgdp = pgd_offset_raw(pgdir, start);

	do{
		pgd_entry = *pgdp;
		next = pgd_addr_end(start, end);
		if(pgd_none(pgd_entry))
			print_pgtable(addr, next, 1, pgd_val(pgd_entry));
		else
			walk_pud(pgdp, addr, next);
	}while(pgdp ++, addr = next, addr != end);
}

void dump_pgtable(void)
{
	pg_level_init();
	walk_pgd((pgd_t *)idmap_pg_dir, 0, TOTAL_MEMORY);
}


static pte_t *walk_table(unsigned long addr)
{
	pgd_t *pgdp;
	pud_t *pudp;
	pmd_t *pmdp;
	pte_t *ptep;

	pgdp = pgd_offset_raw((pgd_t *)idmap_pg_dir, addr);
	if(pgdp == NULL ||pgd_none(*pgdp))
		return NULL;

	pudp = pud_offset_phys(pgdp, addr);
	if(pudp == NULL ||pud_none(*pudp))
		return NULL;

	pmdp = pmd_offset_phys(pudp, addr);
	if(pmdp == NULL ||pmd_none(*pmdp))
		return NULL;
	else if ((pmd_val(*pmdp) & PMD_TYPE_MASK) == PMD_TYPE_SECT)
		return (pte_t *)pmdp;
	
	ptep = pte_offset_phys(pmdp, addr);
	if(pmdp == NULL ||pte_none(*ptep))
		return NULL;
	printk("ben: pte=0x%llx\n", *ptep);
	return ptep;
}


extern char readonly_data[];

void test_walk_pgtable(void)
{
	pte_t pte, *ptep;
	pte_t pte_new;

	unsigned long addr = (unsigned long)readonly_data;
	unsigned long tcr_el1 = 0;

	ptep = walk_table(addr);

	pte = *ptep;
	printk("pte = %llx\n",pte);
	tcr_el1 = read_sysreg(tcr_el1);
	printk("tcr_el1 = %llx\n", tcr_el1);

	pte_new = set_pte_young(pte);			//ww
	pte_new = set_pte_bit(pte_new, PTE_ATTRINDX(MT_NORMAL_WT));
	pte_new = set_pte_mkwrite(pte_new);

	set_pte(ptep, pte_new);
	walk_pgd((pgd_t *)idmap_pg_dir, addr, PAGE_SIZE);

	memset(readonly_data, 0x55, 100);
	printk("write readonly page done\n");
	
}

