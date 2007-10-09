#include <config.h>
#include <asm/arch/imx-regs.h>

#define writel(val, reg) \
	ldr		r0,	=reg;	\
	ldr		r1,	=val;	\
	str		r1,   [r0];

#define CRM_PLL_PCTL_PARAM(pd, fd, fi, fn)  (((pd-1)<<26) + ((fd-1)<<16) + (fi<<10) + (fn<<0))

.globl lowlevel_init
lowlevel_init:

	mov	r10, lr

	/*
	 * AHB-Lite IP Interface
	 */
	writel(0x20040304, AIPI1_PSR0)
	writel(0xDFFBFCFB, AIPI1_PSR1)
	writel(0x00000000, AIPI2_PSR0)
	writel(0xFFFFFFFF, AIPI2_PSR1)

	ldr r0, =CSCR
	ldr r1, [r0]
	bic r1, r1, #0x3
	str r1, [r0] 

	writel(0x00041c02, MPCTL0)
	writel(0x04082008, SPCTL0)

	writel(0x33f00304, CSCR)
	writel(0x33f00304 | CSCR_MPEN | CSCR_SPEN | CSCR_MCU_SEL |
			CSCR_SP_SEL | CSCR_MPLL_RESTART | CSCR_SPLL_RESTART,
			CSCR)

	/* add some delay here */
	mov r1, #0x1000
1:	subs r1, r1, #0x1
	bne 1b

	writel(0x00070f08, GPCR)

	writel(0x130410c3, PCDR0)
	writel(0x09030913, PCDR1)

	/* Skip SDRAM initialization if we run from RAM */
	cmp	pc, #0xa0000000
	bls	1f
	cmp	pc, #0xc0000000
	bhi	1f

	/*
	 * DDR on CSD0
	 */
	writel(0x00000008, 0xD8001010)
	writel(0x55555555, 0x10027828)
	writel(0x55555555, 0x10027830)
	writel(0x55555555, 0x10027834)
	writel(0x00005005, 0x10027838)
	writel(0x15555555, 0x1002783C)
	writel(0x00000004, 0xD8001010)
	writel(0x00795729, 0xD8001004)
	writel(0x92100000, 0xD8001000)
	writel(0x00000000, 0xA0000F00) 
	writel(0xA2100000, 0xD8001000)
	writel(0x00000000, 0xA0000F00)
	writel(0x00000000, 0xA0000F00)
	writel(0x00000000, 0xA0000F00)
	writel(0x00000000, 0xA0000F00)
	writel(0xA2200000, 0xD8001000)
	writel(0x00000000, 0xA0000F00)
	writel(0x00000000, 0xA0000F00)
	writel(0x00000000, 0xA0000F00)
	writel(0x00000000, 0xA0000F00)
	writel(0xB2126080, 0xD8001000)
	ldr		r0, =0xA0000033
	mov		r1, #0x0
	strb		r1, [r0]
	ldr		r0, =0xA1000000
	mov		r1, #0x0
	strb		r1, [r0]
	writel(0x82126080, 0xD8001000)
1:
	mov	pc,r10
