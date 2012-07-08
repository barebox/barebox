#include <config.h>
#include <asm-generic/memory_layout.h>
#include <mach/imx-regs.h>

#define writel(val, reg) \
	ldr		r0,	=reg;	\
	ldr		r1,	=val;	\
	str		r1,   [r0];

#if defined CONFIG_EUKREA_CPUIMX27_SDRAM_256MB
#define ROWS0	ESDCTL_ROW14
#define CFG0	0x0029572D
#elif defined CONFIG_EUKREA_CPUIMX27_SDRAM_128MB
#define ROWS0	ESDCTL_ROW13
#define CFG0	0x00095728
#endif

#define ESDCTL0_VAL (ESDCTL0_SDE | ROWS0 | ESDCTL0_COL10)

.macro sdram_init
	/*
	 * DDR on CSD0
	 */
	writel(0x0000000C, ESDMISC) /* Enable DDR SDRAM operation */

	writel(0x55555555, DSCR(3)) /* Set the driving strength   */
	writel(0x55555555, DSCR(5))
	writel(0x55555555, DSCR(6))
	writel(0x00005005, DSCR(7))
	writel(0x15555555, DSCR(8))

	writel(0x00000004, ESDMISC) /* Initial reset */
	writel(CFG0, ESDCFG0)
	
	writel(ESDCTL0_VAL | ESDCTL0_SMODE_PRECHARGE, ESDCTL0) /* precharge CSD0 all banks */
	writel(0x00000000, 0xA0000F00)	/* CSD0 precharge address (A10 = 1) */
	writel(ESDCTL0_VAL | ESDCTL0_SMODE_AUTO_REFRESH, ESDCTL0)

	ldr	r0, =0xa0000f00
	mov	r1, #0
	mov	r2, #8
1:
	str	r1, [r0]
	subs	r2, #1
	bne	1b

	writel(ESDCTL0_VAL | ESDCTL0_SMODE_LOAD_MODE, ESDCTL0)
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
	writel(ESDCTL0_VAL | ESDCTL0_DSIZ_31_0 | ESDCTL0_REF4 | ESDCTL0_BL | ESDCTL0_SMODE_NORMAL, ESDCTL0)
.endm

	.section ".text_bare_init","ax"

.globl board_init_lowlevel
board_init_lowlevel:

	mov	r10, lr

	/* ahb lite ip interface */
	writel(0x20040304, AIPI1_PSR0)
	writel(0xDFFBFCFB, AIPI1_PSR1)
	writel(0x00000000, AIPI2_PSR0)
	writel(0xFFFFFFFF, AIPI2_PSR1)

	/* disable mpll/spll */
	ldr r0, =CSCR
	ldr r1, [r0]
	bic r1, r1, #0x03
	str r1, [r0]
    
	/*
	 * pll clock initialization - see section 3.4.3 of the i.MX27 manual
	 */
	writel(0x00331C23, MPCTL0)	/* MPLL = 399 MHz */
	writel(0x040C2403, SPCTL0)	/* SPLL = 240 MHz */
	writel(0x33F38107 | CSCR_MPLL_RESTART | CSCR_SPLL_RESTART, CSCR)

	/* add some delay here */
	mov r1, #0x1000
1:	subs r1, r1, #0x1
	bne 1b

	/* clock gating enable */
	writel(0x00050f08, GPCR)

	/* peripheral clock divider */
	writel(0x130400c3, PCDR0)	/* FIXME                            */
	writel(0x09030208, PCDR1)	/* PERDIV1=08 @133 MHz              */
					/* PERDIV1=04 @266 MHz              */

	/* skip sdram initialization if we run from ram */
	cmp	pc, #0xa0000000
	bls	1f
	cmp	pc, #0xc0000000
	bhi	1f

	mov	pc,r10
1:
	sdram_init

#ifdef CONFIG_NAND_IMX_BOOT
	ldr	sp, =STACK_BASE + STACK_SIZE - 12	/* Setup a temporary stack in SDRAM */

	ldr	r0, =IMX_NFC_BASE		/* start of NFC SRAM                */
	ldr	r2, =IMX_NFC_BASE + 0x1000	/* end of NFC SRAM                  */

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
	mov	pc,r10
