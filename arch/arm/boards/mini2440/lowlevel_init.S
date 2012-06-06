/*
 * Low level initialization for the FriendlyARM mini2440 board
 */

#include <config.h>
#include <mach/s3c-iomap.h>

	.section ".text_bare_init.board_init_lowlevel","ax"

/* ------------------------------------------------------------------------ */

.globl board_init_lowlevel
board_init_lowlevel:

	mov r10, lr		/* save the link register */

	bl s3c24x0_disable_wd

	/* skip everything here if we are already running from SDRAM */
	cmp pc, #S3C_SDRAM_BASE
	blo 1f
	cmp pc, #S3C_SDRAM_END
	bhs 1f

	mov pc, r10

/* we are running from NOR or NAND/SRAM memory. Do further initialisation */
1:
	bl s3c24x0_pll_init

	bl s3c24x0_sdram_init

#ifdef CONFIG_S3C_NAND_BOOT
	mov lr, r10		/* restore the link register */
/* up to here we are running from the internal SRAM area */
	b s3c24x0_nand_boot	/* does return directly to our caller into SDRAM */
#else
	mov pc, r10
#endif
