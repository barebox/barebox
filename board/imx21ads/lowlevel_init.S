/*
 * For clock initialization, see chapter 6 of the 
 * "i.MX21 Applications Processor Reference Manual, Rev. 3".
 */

#include <config.h>
#include <mach/imx-regs.h>

#define writel(val, reg) \
	ldr		r0,	=reg;	\
	ldr		r1,	=val;	\
	str		r1,   [r0];

.macro sdram_init_mx21ads
#if 0
	/* Set precharge command */
	writel(0x92120300, SDCTL0);

	/* Issue Precharge all Command */
	ldr  r3, =0xC0200000;
	ldr  r2, [r3];

	/* Set AutoRefresh command */
	writel(0x92120300, SDCTL0);

	/* Issue AutoRefresh command */
	ldr  r3, =0xC0000000;
	ldr  r2, [r3];
	ldr  r2, [r3];
	ldr  r2, [r3];
	ldr  r2, [r3];
	ldr  r2, [r3];
	ldr  r2, [r3];
	ldr  r2, [r3];
	ldr  r2, [r3];

	
	/* Set Mode Register */
	writel(0xB2120300, SDCTL0);
#endif
.endm

.globl board_init_lowlevel
board_init_lowlevel:

	mov	r10, lr

	/* ahb lite ip interface */
	writel(0x00040304, AIPI1_PSR0)
	writel(0xFFFBFCFB, AIPI1_PSR1)
	writel(0x00000000, AIPI2_PSR0)
	writel(0xFFFFFFFF, AIPI2_PSR1)

	/* Set MPLL to 266MHz */
	writel(0x007B1C73, MPCTL0);
	/* PLL 133MHz */
	writel(0x17000607, CSCR);

	/* skip sdram initialization if we run from ram */
	/* (SDRAM is mapped from C0000000 to C3FFFFFF)  */
	cmp	pc, #0xc0000000
	bls	1f
	cmp	pc, #0xc8000000
	bhi	1f

	mov	pc,r10
1:
	sdram_init_mx21ads

	mov	pc,r10
