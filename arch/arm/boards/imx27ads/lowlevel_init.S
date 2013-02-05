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

.macro sdram_init_sha
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

.macro sdram_init_mx27_manual
	/*
	 * sdram init sequence, as defined in 18.5.4 of the i.MX27 reference manual
	 */
1:
	ldr r2, =ESD_ESDCTL0		/* base address of registers */
	ldr r3, =PRE_ALL_CMD		/* SMODE=001 */
	str r3,(r2,#0x0)		/* put CSD0 in precharge command mode */
	ldr r4, =SDRAM_CSD0		/* CSD0 precharge address (A10=1) */
	str r1,(r4,#0x0)		/* precharge CSD0 all banks */
	ldr r3, =AUTO_REF_CMD		/* SMODE=010 */
	str r3,(r2,#0x0)		/* put array 0 in auto-refresh mode */
	ldr r4, =SDRAM_CSD0_BASE	/* CSD0 base address */
	ldr r6,=0x7			/* load loop counter */
1:	ldr r5,(r4,#0x0)		/* run auto-refresh cycle to array 0 */
	subs r6,r6,#1			/* decrease counter value */
	bne 1b
	ldr r3, =SET_MODE_REG_CMD	/* SMODE=011 */
	str r3,(r2,#0x0)		/* setup CSD0 for mode register write */
	ldr r3, =MODE_REG_VAL0		/* array 0 mode register value */
	ldrb r5,(r3,#0x0)		/* New mode register value on address bus */
	ldr r3, =NORMAL_MODE		/* SMODE=000 */
	str r3,(r2,#0x0)		/* setup CSD0 for normal operation */

ESD_ESDCTL0		.long 0xD8001000 // system/external device dependent data
SDRAM_CSD0		.long 0x00000000 // system/external device dependent data
SDRAM_CSD0_BASE		.long 0x00000000 // system/external device dependent data
PRE_ALL_CMD		.long 0x00000000 // system/external device dependent data (SMODE=001)
AUTO_REF_CMD		.long 0x00000000 // system/external device dependent data (SMODE=010)
SET_MODE_REG_CMD	.long 0x00000000 // system/external device dependent data (SMODE=011)
MODE_REG_VAL0		.long 0x00000000 // system/external device dependent data
NORMAL_MODE		.long 0x00000000 // system/external device dependent data (SMODE=000)
.endm

.macro sdram_init_barebox
	/* configure 16 bit nor flash on cs0 */
	writel(0x0000CC03, 0xd8002000)
	writel(0xa0330D01, 0xd8002004)
	writel(0x00220800, 0xd8002008)

	/* ddr on csd0 - initial reset */
	writel(0x00000008, 0xD8001010)

	/* configure ddr on csd0 - wait 5000 cycles */
	writel(0x00000004, 0xD8001010)
	writel(0x006ac73a, 0xD8001004)
	writel(0x92100000, 0xD8001000)
	writel(0x12344321, 0xA0000f00)
	writel(0xa2100000, 0xD8001000)
	writel(0x12344321, 0xA0000000)
	writel(0x12344321, 0xA0000000)
	writel(0xb2100000, 0xD8001000)
	ldr		r0, =0xA0000033
	mov		r1, #0xda
	strb		r1, [r0]
	ldr		r0, =0xA1000000
	mov		r1, #0xff
	strb		r1, [r0]
	writel(0x82226080, 0xD8001000)
	writel(0xDEADBEEF, 0xA0000000)
	writel(0x0000000c, 0xD8001010)
.endm

.globl barebox_arm_reset_vector
barebox_arm_reset_vector:

	arm_cpu_lowlevel_init r0

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
	sdram_init_sha

	b imx27_barebox_entry

