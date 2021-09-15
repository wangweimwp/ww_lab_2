#include "mm.h"
#include "sysregs.h"
#include <asm/pgtable_prot.h>





.section  .rodata
.align 3
.globl el_string1
el_string1:
	.string "Booting at EL"



.section ".text.boot"
.globl _start
_start:
	mrs	x0, mpidr_el1		
	and	x0, x0,#0xFF		// Check processor id
	cbz	x0, master		// Hang for all non-primary CPU
	b	proc_hang

proc_hang: 
	b 	proc_hang

master:
	bl __init_uart
	mrs x0, currentel
	cmp x0, #CurrentEL_EL3
	b.eq entry_el3
	b entry_el2

entry_el3:
	eret

entry_el2:
	bl print_el

	ldr x0, =HCR_HOST_NVHE_FLAGS
	msr hcr_el2, x0

	ldr x1, =SCTLR_VALUE_MMU_DISABLED
	msr sctlr_el1, x1
 
	ldr x2, =SPSR_EL1
	msr spsr_el2, x2

	adr x3, entry_el1
	msr elr_el2, x3

	eret
	
entry_el1:
	bl print_el

	ldr x5, =(3UL << 20)
	msr cpacr_el1, x5	//设置FPEN字段，否则执行时自动陷入到EL1

	ldr x5, =vecters
	msr vbar_el1, x5
	isb
	adr	x0, bss_begin
	adr	x1, bss_end
	sub	x1, x1, x0
	bl 	memzero

//	bl create_page_table
//	bl enable_mmu_asm
	adr x2, kernel_sp
	add sp, x2, #4096

//	mov	sp, #LOW_MEMORY 
	bl	kernel_main
	b 	proc_hang		// should never come here


print_el:
	mov x10, x30

	/*
	   print EL
	 */
	adrp x0, el_string1
	add x0, x0, :lo12:el_string1
	bl put_string_uart

	mrs x5, CurrentEL
	/* get the currentEL value */
	lsr x2, x5, #2
	mov x0, #48
	add x0, x0, x2
	bl put_uart
	/* print the new line tab */
	mov x0, #10
	bl put_uart

	mov x30, x10
	ret


enable_mmu_asm:
	tlbi vmalle1
	dsb nsh

	mov x0, #0x300000
	msr cpacr_el1, x0
	mov x0, #0x1000
	msr mdscr_el1, x0

	ldr x0, =MAIR(0x00UL, MT_DEVICE_nGnRnE) | \
			MAIR(0x04UL, MT_DEVICE_nGnRE) | \
			MAIR(0x0cUL, MT_DEVICE_GRE) | \
			MAIR(0x44UL, MT_NORMAL_NC) | \
			MAIR(0xffUL, MT_NORMAL) | \
			MAIR(0xbbUL, MT_NORMAL_WT);
	msr mair_el1, x0

	ldr x0, =TCR_TxSZ(VA_BITS) | TCR_TG_FLAGS
	ldr x1, =0x500000000
	orr x0, x0, x1
	msr tcr_el1, x0
	

	adrp x0, idmap_pg_dir
	msr ttbr0_el1, x0
	isb

	mov x0, #SCTLR_ELx_M
	msr sctlr_el1, x0
	isb
	ic iallu
	dsb nsh
	isb
	ret

create_page_table:
	mov x25, x30

	adrp x0, idmap_pg_dir
	adrp x1, idmap_pg_end
	sub x1, x1, x0
	bl memzero

	adrp x1, _text_boot
	and x1, x1, SECTION_MASK
	mov x2, x1
	adrp x3, _end
	add x3, x3, SECTION_SIZE
	sub x3, x3, 1
	and x3, x3, SECTION_MASK
	mov x4, SWAPPER_MM_MMUFLAGS
	adrp x0, idmap_pg_dir

	bl __create_section_mapping

	mov x30, x25
	ret

	
/*
	x0: page table base address
	x1: phys address
	x2: virt start address
	x3: virt end address
	x4: prot
*/
__create_section_mapping:
	mov x12, x30

	/* 1. find pgd entry, x8: pgd index */
	lsr x8, x2, #PGDIR_SHIFT
	and x8, x8, #PTRS_PER_PGD - 1

	/*2. fill pgd entry, x9: pud base address*/
	add x9, x0, #PAGE_SIZE
	orr x9, x9, #PUD_TYPE_TABLE
	str x9, [x0, x8, lsl #3]		//每一个pud entry占用8字节 所以左移3位

	add x0, x0, #PAGE_SIZE

	/*3. find pud entry: x8: pud index */
	lsr x8, x2, #PUD_SHIFT
	and x8, x8, #PTRS_PER_PUD - 1

	/*4. fill pmd entry, x9: pmd base address*/
	add x9, x0, #PAGE_SIZE
	orr x9, x9, #PMD_TYPE_TABLE
	str x9, [x0, x8, lsl #3]

	add x0, x0, #PAGE_SIZE

	/*5. find pmd entry*/
1:
	lsr x8, x2, #PMD_SHIFT
	and x8, x8, #PTRS_PER_PMD - 1
	lsr x10, x1, #SECTION_SHIFT
	mov x9, x4
	orr x10, x9, x10, lsl #SECTION_SHIFT
	str x10, [x0, x8, lsl #3]
	
	add x2, x2, #PMD_SIZE
	add x1, x1, #PMD_SIZE

	cmp x3, x2
	b.hi 1b

	mov x30,x12
	ret
	


