/*
 * For clock initialization, see chapter 3 of the "MCIMX27 Multimedia
 * Applications Processor Reference Manual, Rev. 0.2".
 *
 */

#include <config.h>
#include <mach/imx27-regs.h>
#include <asm/barebox-arm-head.h>

#define writel(val, reg) \
	ldr		r0,	=reg;	\
	ldr		r1,	=val;	\
	str		r1,   [r0];

#define CRM_PLL_PCTL_PARAM(pd, fd, fi, fn)  (((pd-1)<<26) + ((fd-1)<<16) + (fi<<10) + (fn<<0))

.macro sdram_init
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
	writel(0x006ac73a, 0xD8001004)
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
	writel(0xb2100000, 0xD8001000)
	ldr		r0, =0xA0000033
	mov		r1, #0xda
	strb		r1, [r0]
	ldr		r0, =0xA1000000
	mov		r1, #0xff
	strb		r1, [r0]
	writel(0x82226080, 0xD8001000)
.endm

.globl barebox_arm_reset_vector
barebox_arm_reset_vector:

	bl arm_cpu_lowlevel_init

	/* ahb lite ip interface */
	writel(0x20040304, MX27_AIPI_BASE_ADDR + MX27_AIPI1_PSR0)
	writel(0xDFFBFCFB, MX27_AIPI_BASE_ADDR + MX27_AIPI1_PSR1)
	writel(0x00000000, MX27_AIPI_BASE_ADDR + MX27_AIPI2_PSR0)
	writel(0xFFFFFFFF, MX27_AIPI_BASE_ADDR + MX27_AIPI2_PSR1)

	/* disable mpll/spll */
	ldr r0, =MX27_CCM_BASE_ADDR + MX27_CSCR
	ldr r1, [r0]
	bic r1, r1, #0x03
	str r1, [r0]

	/*
	 * pll clock initialization - see section 3.4.3 of the i.MX27 manual
	 *
	 * FIXME: Using the 399*2 MHz values from table 3-8 doens't work
	 *        with 1.2 V core voltage! Find out if this is
	 *        documented somewhere.
	 */
	writel(0x00191403, MX27_CCM_BASE_ADDR + MX27_MPCTL0)	/* MPLL = 199.5*2 MHz               */
	writel(0x040C2403, MX27_CCM_BASE_ADDR + MX27_SPCTL0)	/* SPLL = FIXME (needs review)      */

	/*
	 * ARM clock = (399 MHz / 2) / (ARM divider = 1) = 200 MHz
	 * AHB clock = (399 MHz / 3) / (AHB divider = 2) = 66.5 MHz
	 * System clock (HCLK) = 133 MHz
	 */
	writel(0x33F30307 | MX27_CSCR_MPLL_RESTART | MX27_CSCR_SPLL_RESTART,
			MX27_CCM_BASE_ADDR + MX27_CSCR)

	/* add some delay here */
	mov r1, #0x1000
1:	subs r1, r1, #0x1
	bne 1b

	/* clock gating enable */
	writel(0x00050f08, MX27_SYSCTRL_BASE_ADDR + MX27_GPCR)

	/* peripheral clock divider */
	/* FIXME                            */
	writel(0x23C8F403, MX27_CCM_BASE_ADDR + MX27_PCDR0)
	/* PERDIV1=08 @133 MHz              */
	/* PERDIV1=04 @266 MHz              */
	writel(0x09030913, MX27_CCM_BASE_ADDR + MX27_PCDR1)
	/* skip sdram initialization if we run from ram */
	cmp	pc, #0xa0000000
	bls	1f
	cmp	pc, #0xc0000000
	bhi	1f

	b imx27_barebox_entry
1:
	sdram_init

	b imx27_barebox_entry

