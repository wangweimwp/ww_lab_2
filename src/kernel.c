#include "uart.h"
#include <type.h>
#include "memset.h"
#include "sysregs.h"
#include "esr.h"
#include <timer.h>
#include <asm/timer.h>
#include "asm/base.h"
#include "irq.h"
#include "fp_neon_test.h"

  


extern void ldr_str_test(void);
extern void adrp_ldr_test(void);
extern void trigger_alignment(void);

static const char * const bad_mode_handler[] = {
	"Sync Abort",
	"IRQ",
	"FIQ",
	"SError"
};
void parse_esr(unsigned int esr)
{
	unsigned int ec = ESR_ELx_EC(esr);

	printk("ESR info:\n");
	printk("  ESR = 0x%08x\n", esr);
	printk("  Exception class = %s, IL = %u bits\n",
		 esr_get_class_string(esr),
		 (esr & ESR_ELx_IL) ? 32 : 16);


	if (ec == ESR_ELx_EC_DABT_LOW || ec == ESR_ELx_EC_DABT_CUR) {
		printk("  Data abort:\n");
		if ((esr & ESR_ELx_ISV)) {
			printk("  Access size = %u byte(s)\n",
			 1U << ((esr & ESR_ELx_SAS) >> ESR_ELx_SAS_SHIFT));
			printk("  SSE = %lu, SRT = %lu\n",
			 (esr & ESR_ELx_SSE) >> ESR_ELx_SSE_SHIFT,
			 (esr & ESR_ELx_SRT_MASK) >> ESR_ELx_SRT_SHIFT);
			printk("  SF = %lu, AR = %lu\n",
			 (esr & ESR_ELx_SF) >> ESR_ELx_SF_SHIFT,
			 (esr & ESR_ELx_AR) >> ESR_ELx_AR_SHIFT);
		}
		
		printk("  SET = %lu, FnV = %lu\n",
			(esr >> ESR_ELx_SET_SHIFT) & 3,
			(esr >> ESR_ELx_FnV_SHIFT) & 1);
		printk("  EA = %lu, S1PTW = %lu\n",
			(esr >> ESR_ELx_EA_SHIFT) & 1,
			(esr >> ESR_ELx_S1PTW_SHIFT) & 1);
		printk("  CM = %lu, WnR = %lu\n",
		 (esr & ESR_ELx_CM) >> ESR_ELx_CM_SHIFT,
		 (esr & ESR_ELx_WNR) >> ESR_ELx_WNR_SHIFT);
		printk("  DFSC = %s\n", esr_get_dfsc_string(esr));
	}
}


void bad_mode(struct pt_regs *regs, int reason, unsigned int esr)
{
	printk("Bad mode for %s handler detected, far:0x%x esr:0x%x - %s\n",
			bad_mode_handler[reason], read_sysreg(far_el1),
			esr, esr_get_class_string(esr));

	printk("kernel panic\n");
	while(1);
}


void kernel_main(void)
{
	unsigned long p1 = 0;
	unsigned long val;
//	paging_init();

	uart_init();
	init_printk_done();
	uart_send_string("Welcome BenOS!\r\n");
	printk("init_printk_done\n");
	ldr_str_test();
	adrp_ldr_test();
	gic_init(0, GIC_V2_DISTRIBUTOR_BASE, GIC_V2_CPU_INTERFACE_BASE);
	trigger_alignment();
	system_timer_init();
//	timer_init();
	raw_local_irq_enable();


	paging_init();
	dump_pgtable();
	test_walk_pgtable();

	val = my_atomic_write(0x345);
	printk("my_atomic_write: 0x%x\n", val);

	atomic_set(0x11, &p1);
	printk("atomic_set: 0x%x\n", p1);

	test_mmu();

	

	fp_test();
	my_fp_neon_test();
	printk("done\n");
	memset(0x200004, 0x55, 102);
	while (1) {
		uart_send(uart_recv());
	}
}
