/*
 *
 * (c) 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <asm-generic/memory_layout.h>
#include <mach/imx25-regs.h>
#include <mach/imx-pll.h>
#include <mach/esdctl.h>
#include <asm/barebox-arm-head.h>

#define writel(val, reg) \
	ldr		r0,	=reg;	\
	ldr		r1,	=val;	\
	str		r1,	[r0];

#define writeb(val, reg) \
	ldr		r0,	=reg;	\
	ldr		r1,	=val;	\
	strb		r1,	[r0];

/* Assuming 24MHz input clock */
#define MPCTL_PARAM_399     (IMX_PLL_PD(0) | IMX_PLL_MFD(15) | IMX_PLL_MFI(8) | IMX_PLL_MFN(5))
#define MPCTL_PARAM_532     (IMX_PLL_PD(1) | IMX_PLL_MFD(0) | IMX_PLL_MFI(11) | IMX_PLL_MFN(1))
#define PPCTL_PARAM_300     (IMX_PLL_PD(0) | IMX_PLL_MFD(3) | IMX_PLL_MFI(6) | IMX_PLL_MFN(1))

.section ".text_bare_init","ax"

ARM_PPMRR:              .word   0x40000015
L2CACHE_PARAM:          .word   0x00030024
CCM_CCMR_W:             .word   0x003F4208
CCM_PDR0_W:             .word   0x00801000
MPCTL_PARAM_399_W:      .word   MPCTL_PARAM_399
MPCTL_PARAM_532_W:      .word   MPCTL_PARAM_532
PPCTL_PARAM_W:    	.word   PPCTL_PARAM_300
CCM_BASE_ADDR_W:        .word   MX25_CCM_BASE_ADDR

.globl barebox_arm_reset_vector
barebox_arm_reset_vector:
	bl arm_cpu_lowlevel_init

#define MX25_CCM_MCR	0x64

	ldr r0, CCM_BASE_ADDR_W
	/* default CLKO to 1/32 of the ARM core */
	ldr r1, [r0, #MX25_CCM_MCR]
	bic r1, r1, #0x00F00000
	bic r1, r1, #0x7F000000
	mov r2,     #0x5F000000
	add r2, r2, #0x00200000
	orr r1, r1, r2
	str r1, [r0, #MX25_CCM_MCR]

	/* enable all the clocks */
	writel(0x1FFFFFFF, MX25_CCM_BASE_ADDR + MX25_CCM_CGCR0)
	writel(0xFFFFFFFF, MX25_CCM_BASE_ADDR + MX25_CCM_CGCR1)
	writel(0x000FDFFF, MX25_CCM_BASE_ADDR + MX25_CCM_CGCR2)
	writel(0x0000FEFF, MX25_CCM_BASE_ADDR + MX25_CCM_MCR)

	/* Skip SDRAM initialization if we run from RAM */
	cmp	pc, #0x80000000
	bls	1f
	cmp	pc, #0x90000000
	bhi	1f

	b	imx25_barebox_entry

1:
	ldr r0, ESDCTL_BASE_W
	mov r3, #0x2000
	str r3, [r0, #0x0]
	str r3, [r0, #0x8]

	mov r12, #0x00
	mov r2, #0x1	/* mDDR */
	mov r1, #MX25_CSD0_BASE_ADDR
	bl setup_sdram_bank
//	cmp r3, #0x0
//	orreq r12, r12, #1
//	eorne r2, r2, #0x1
//	blne setup_sdram_bank

	ldr r3, ESDCTL_DELAY5
	str r3, [r0, #0x30]

#ifdef CONFIG_NAND_IMX_BOOT
	/* Setup a temporary stack in SRAM */
	ldr	sp, =MX25_IRAM_BASE_ADDR + MX25_IRAM_SIZE - 4

	b	imx25_barebox_boot_nand_external
#endif /* CONFIG_NAND_IMX_BOOT */

ret:
	b	imx25_barebox_entry

/*
 * r0: control base, r1: ram bank base
 * r2: ddr type(0:DDR2, 1:MDDR) r3, r4: working
 */
setup_sdram_bank:
	mov r3, #0xE /* 0xA + 0x4 */
	tst r2, #0x1
	orreq r3, r3, #0x300 /* DDR2 */
	str r3, [r0, #0x10]
	bic r3, r3, #0x00A
	str r3, [r0, #0x10]
	beq 2f

	mov r3, #0x20000
1:	subs r3, r3, #1
	bne 1b

2:	adr r4, ESDCTL_CONFIG
	tst r2, #0x1
	ldreq r3, [r4, #0x0]
	ldrne r3, [r4, #0x4]
	cmp r1, #MX25_CSD1_BASE_ADDR
	strlo r3, [r0, #0x4]
	strhs r3, [r0, #0xC]

	ldr r3, ESDCTL_0x92220000
	strlo r3, [r0, #0x0]
	strhs r3, [r0, #0x8]
	mov r3, #0xDA
	ldr r4, RAM_PARAM1_MDDR
	strb r3, [r1, r4]

	tst r2, #0x1
	bne skip_set_mode

	cmp r1, #MX25_CSD1_BASE_ADDR
	ldr r3, ESDCTL_0xB2220000
	strlo r3, [r0, #0x0]
	strhs r3, [r0, #0x8]
	mov r3, #0xDA
	ldr r4, RAM_PARAM4_MDDR
	strb r3, [r1, r4]
	ldr r4, RAM_PARAM5_MDDR
	strb r3, [r1, r4]
	ldr r4, RAM_PARAM3_MDDR
	strb r3, [r1, r4]
	ldr r4, RAM_PARAM2_MDDR
	strb r3, [r1, r4]

	ldr r3, ESDCTL_0x92220000
	strlo r3, [r0, #0x0]
	strhs r3, [r0, #0x8]
	mov r3, #0xDA
	ldr r4, RAM_PARAM1_MDDR
	strb r3, [r1, r4]

skip_set_mode:
	cmp r1, #MX25_CSD1_BASE_ADDR
	ldr r3, ESDCTL_0xA2220000
	strlo r3, [r0, #0x0]
	strhs r3, [r0, #0x8]
	mov r3, #0xDA
	strb r3, [r1]
	strb r3, [r1]

	ldr r3, ESDCTL_0xB2220000
	strlo r3, [r0, #0x0]
	strhs r3, [r0, #0x8]
	adr r4, RAM_PARAM6_MDDR
	tst r2, #0x1
	ldreq r4, [r4, #0x0]
	ldrne r4, [r4, #0x4]
	mov r3, #0xDA
	strb r3, [r1, r4]
	ldreq r4, RAM_PARAM7_MDDR
	streqb r3, [r1, r4]
	adr r4, RAM_PARAM3_MDDR
	ldreq r4, [r4, #0x0]
	ldrne r4, [r4, #0x4]
	strb r3, [r1, r4]

	cmp r1, #MX25_CSD1_BASE_ADDR
	ldr r3, ESDCTL_0x82226080
	strlo r3, [r0, #0x0]
	strhs r3, [r0, #0x8]

	tst r2, #0x1
	moveq r4, #0x20000
	movne r4, #0x200
1:	subs r4, r4, #1
	bne 1b

	str r3, [r1, #0x100]
	ldr r4, [r1, #0x100]
	cmp r3, r4
	movne r3, #1
	moveq r3, #0

	mov pc, lr

RAM_PARAM1_MDDR:	.word	0x00000400
RAM_PARAM2_MDDR:	.word	0x00000333
RAM_PARAM3_MDDR:	.word	0x02000400
			.word	0x02000000
RAM_PARAM4_MDDR:	.word	0x04000000
RAM_PARAM5_MDDR:	.word	0x06000000
RAM_PARAM6_MDDR:	.word	0x00000233
			.word	0x00000033
RAM_PARAM7_MDDR:	.word	0x02000780
ESDCTL_0x92220000:	.word	0x92210000
ESDCTL_0xA2220000:	.word	0xA2210000
ESDCTL_0xB2220000:	.word	0xB2210000
ESDCTL_0x82226080:	.word	0x82216080
ESDCTL_CONFIG:		.word	0x007FFC3F
			.word	0x007FFC3F
ESDCTL_DELAY5:		.word	0x00F49F00
ESDCTL_BASE_W:		.word	MX25_ESDCTL_BASE_ADDR

