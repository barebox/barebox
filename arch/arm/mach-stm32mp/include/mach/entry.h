#ifndef _STM32MP_MACH_ENTRY_H_
#define _STM32MP_MACH_ENTRY_H_

#include <linux/kernel.h>
#include <asm/barebox-arm.h>

static __always_inline void stm32mp_cpu_lowlevel_init(void)
{
	unsigned long stack_top;
	arm_cpu_lowlevel_init();

	stack_top = (unsigned long)__image_end + get_runtime_offset() + 64;
	stack_top = ALIGN(stack_top, 16);
	arm_setup_stack(stack_top);
}

void __noreturn stm32mp1_barebox_entry(void *boarddata);

#endif
