#include <config.h>
#include <asm-generic/memory_layout.h>
#include <mach/imx-regs.h>
#include <mach/esdctl.h>
#include <asm/barebox-arm-head.h>

#define writel(val, reg) \
	ldr		r0,	=reg;	\
	ldr		r1,	=val;	\
	str		r1,   [r0];

#if defined CONFIG_EUKREA_CPUIMX27_SDRAM_256MB
#define ROWS0	ESDCTL0_ROW14
#define CFG0	0x0029572D
#elif defined CONFIG_EUKREA_CPUIMX27_SDRAM_128MB
#define ROWS0	ESDCTL0_ROW13
#define CFG0	0x00095728
#endif

#define ESDCTL0_VAL (ESDCTL0_SDE | ROWS0 | ESDCTL0_COL10)

.macro sdram_init
	/*
	 * DDR on CSD0
	 */
	/* Enable DDR SDRAM operation */
	writel(0x0000000C, MX27_ESDCTL_BASE_ADDR + IMX_ESDMISC)

	/* Set the driving strength   */
	writel(0x55555555, MX27_SYSCTRL_BASE_ADDR + MX27_DSCR(3))
	writel(0x55555555, MX27_SYSCTRL_BASE_ADDR + MX27_DSCR(5))
	writel(0x55555555, MX27_SYSCTRL_BASE_ADDR + MX27_DSCR(6))
	writel(0x00005005, MX27_SYSCTRL_BASE_ADDR + MX27_DSCR(7))
	writel(0x15555555, MX27_SYSCTRL_BASE_ADDR + MX27_DSCR(8))

	/* Initial reset */
	writel(0x00000004, MX27_ESDCTL_BASE_ADDR + IMX_ESDMISC)
	writel(CFG0, MX27_ESDCTL_BASE_ADDR + IMX_ESDCFG0)

	/* precharge CSD0 all banks */
	writel(ESDCTL0_VAL | ESDCTL0_SMODE_PRECHARGE,
			MX27_ESDCTL_BASE_ADDR + IMX_ESDCTL0)
	writel(0x00000000, 0xA0000F00)	/* CSD0 precharge address (A10 = 1) */
	writel(ESDCTL0_VAL | ESDCTL0_SMODE_AUTO_REFRESH,
			MX27_ESDCTL_BASE_ADDR + IMX_ESDCTL0)

	ldr	r0, =0xa0000f00
	mov	r1, #0
	mov	r2, #8
1:
	str	r1, [r0]
	subs	r2, #1
	bne	1b

	writel(ESDCTL0_VAL | ESDCTL0_SMODE_LOAD_MODE,
			MX27_ESDCTL_BASE_ADDR + IMX_ESDCTL0)
	ldr		r0, =0xA0000033
	mov		r1, #0xda
	strb		r1, [r0]
#if defined CONFIG_EUKREA_CPUIMX27_SDRAM_256MB
	ldr		r0, =0xA2000000
#elif defined CONFIG_EUKREA_CPUIMX27_SDRAM_128MB
	ldr		r0, =0xA1000000
#endif
	mov		r1, #0xff
	strb		r1, [r0]
	writel(ESDCTL0_VAL | ESDCTL0_DSIZ_31_0 | ESDCTL0_REF4 |
			ESDCTL0_BL | ESDCTL0_SMODE_NORMAL,
			MX27_ESDCTL_BASE_ADDR + IMX_ESDCTL0)
.endm

	.section ".text_bare_init","ax"

.globl reset
reset:

	common_reset r0

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
	 */
	/* MPLL = 399 MHz */
	writel(0x00331C23, MX27_CCM_BASE_ADDR + MX27_MPCTL0)
	/* SPLL = 240 MHz */
	writel(0x040C2403, MX27_CCM_BASE_ADDR + MX27_SPCTL0)
	writel(0x33F38107 | MX27_CSCR_MPLL_RESTART | MX27_CSCR_SPLL_RESTART,
			MX27_CCM_BASE_ADDR + MX27_CSCR)

	/* add some delay here */
	mov r1, #0x1000
1:	subs r1, r1, #0x1
	bne 1b

	/* clock gating enable */
	writel(0x00050f08, MX27_SYSCTRL_BASE_ADDR + MX27_GPCR)

	/* peripheral clock divider */
	/* FIXME                            */
	writel(0x130400c3, MX27_CCM_BASE_ADDR + MX27_PCDR0)
	/* PERDIV1=08 @133 MHz              */
	writel(0x09030208, MX27_CCM_BASE_ADDR + MX27_PCDR1)
	/* PERDIV1=04 @266 MHz              */

	/* skip sdram initialization if we run from ram */
	cmp	pc, #0xa0000000
	bls	1f
	cmp	pc, #0xc0000000
	bhi	1f

	b	board_init_lowlevel_return
1:
	sdram_init

#ifdef CONFIG_NAND_IMX_BOOT
	ldr	sp, =STACK_BASE + STACK_SIZE - 12	/* Setup a temporary stack in SDRAM */

	ldr	r0, =MX27_NFC_BASE_ADDR		/* start of NFC SRAM                */
	ldr	r2, =MX27_NFC_BASE_ADDR + 0x1000	/* end of NFC SRAM                  */

	/* skip NAND boot if not running from NFC space */
	cmp	pc, r0
	bls	ret
	cmp	pc, r2
	bhi	ret

	/* Move ourselves out of NFC SRAM */
	ldr	r1, =_text

copy_loop:
	ldmia	r0!, {r3-r9}		/* copy from source address [r0]    */
	stmia	r1!, {r3-r9}		/* copy to   target address [r1]    */
	cmp	r0, r2			/* until source end address [r2]    */
	ble	copy_loop

	ldr	pc, =1f			/* Jump to SDRAM                    */
1:
	b	nand_boot		/* Load barebox from NAND Flash      */
					/* to SDRAM                            */
#endif /* CONFIG_NAND_IMX_BOOT */

ret:
	b	board_init_lowlevel_return
