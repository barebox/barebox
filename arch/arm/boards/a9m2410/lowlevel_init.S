/*
 *
 */

#include <config.h>
#include <sizes.h>
#include <mach/s3c-iomap.h>
#include <asm/barebox-arm-head.h>

	.section ".text_bare_init.barebox_arm_reset_vector","ax"

.globl barebox_arm_reset_vector
barebox_arm_reset_vector:

	bl arm_cpu_lowlevel_init

	bl s3c24x0_disable_wd

	/* skip everything here if we are already running from SDRAM */
	cmp pc, #S3C_SDRAM_BASE
	blo 1f
	cmp pc, #S3C_SDRAM_END
	bhs 1f

	b out

/* we are running from NOR or NAND/SRAM memory. Do further initialisation */
1:
	bl s3c24x0_pll_init

	bl s3c24x0_sdram_init

#ifdef CONFIG_S3C_NAND_BOOT
/* up to here we are running from the internal SRAM area */
	bl s3c24x0_nand_boot
#endif
out:
	mov r0, #S3C_SDRAM_BASE
	mov r1, #SZ_32M
	mov r2, #0
	b barebox_arm_entry
