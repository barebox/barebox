#include <linux/linkage.h>
#include <init.h>
#include <asm/system.h>

.section ".text_bare_init_","ax"
ENTRY(arm_cpu_lowlevel_init)
	/* set the cpu to SVC32 mode */
	mrs	r12, cpsr
	bic	r12, r12, #0x1f
	orr	r12, r12, #0xd3
	msr	cpsr, r12

#if __LINUX_ARM_ARCH__ >= 7
	isb
#elif __LINUX_ARM_ARCH__ == 6
	mcr	p15, 0, r12, c7, c5, 4
#endif

	/* disable MMU stuff and caches */
	mrc	p15, 0, r12, c1, c0, 0
	bic	r12, r12 , #(CR_M | CR_C | CR_B)
	bic	r12, r12, #(CR_S | CR_R | CR_V)
	orr	r12, r12, #CR_I

#if __LINUX_ARM_ARCH__ >= 6
	orr	r12, r12, #CR_U
	bic	r12, r12, #CR_A
#else
	orr	r12, r12, #CR_A
#endif

#ifdef __ARMEB__
	orr	r12, r12, #CR_B
#endif

	mcr	p15, 0, r12, c1, c0, 0

	mov	pc, lr
ENDPROC(arm_cpu_lowlevel_init)
