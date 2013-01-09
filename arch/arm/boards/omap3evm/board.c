/**
 * @file
 * @brief Board Initialization routines for OMAP3EVM.
 *
 * This board is based on OMAP3530.
 * More on OMAP3530 (including documentation can be found here):
 * http://focus.ti.com/docs/prod/folders/print/omap3530.html
 *
 * This file provides initialization in two stages:
 * @li Boot time initialization - just get SDRAM working.
 * This is run from SRAM - so no case constructs and global vars can be used.
 * @li Run time initialization - this is for the rest of the initializations
 * such as flash, uart etc.
 *
 * Boot time initialization includes:
 * @li SDRAM initialization.
 * @li Pin Muxing relevant for the EVM.
 *
 * Run time initialization includes
 * @li serial @ref serial_ns16550.c driver device definition
 *
 * Originally from arch/arm/boards/omap/board-beagle.c
 */

/*
 * Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/
 * Sanjeev Premi <premi@ti.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */


#include <common.h>
#include <console.h>
#include <init.h>
#include <driver.h>
#include <io.h>
#include <sizes.h>
#include <ns16550.h>
#include <asm/armlinux.h>
#include <mach/omap3-silicon.h>
#include <mach/sdrc.h>
#include <mach/sys_info.h>
#include <mach/syslib.h>
#include <mach/control.h>
#include <mach/omap3-mux.h>
#include <mach/gpmc.h>
#include <errno.h>
#include <generated/mach-types.h>
#include <mach/omap3-devices.h>


/*
 * Boot-time initialization(s)
 */

/**
 * @brief Initialize the SDRC module
 *
 * @return void
 */
static void sdrc_init(void)
{
	/* SDRAM software reset */
	/* No idle ack and RESET enable */
	writel(0x1A, OMAP3_SDRC_REG(SYSCONFIG));
	sdelay(100);
	/* No idle ack and RESET disable */
	writel(0x18, OMAP3_SDRC_REG(SYSCONFIG));

	/* SDRC Sharing register */
	/* 32-bit SDRAM on data lane [31:0] - CS0 */
	/* pin tri-stated = 1 */
	writel(0x00000100, OMAP3_SDRC_REG(SHARING));

	/* ----- SDRC Registers Configuration --------- */
	/* SDRC_MCFG0 register */
	writel(0x02584099, OMAP3_SDRC_REG(MCFG_0));

	/* SDRC_RFR_CTRL0 register */
	writel(0x54601, OMAP3_SDRC_REG(RFR_CTRL_0));

	/* SDRC_ACTIM_CTRLA0 register */
	writel(0xA29DB4C6, OMAP3_SDRC_REG(ACTIM_CTRLA_0));

	/* SDRC_ACTIM_CTRLB0 register */
	writel(0x12214, OMAP3_SDRC_REG(ACTIM_CTRLB_0));

	/* Disble Power Down of CKE due to 1 CKE on combo part */
	writel(0x00000081, OMAP3_SDRC_REG(POWER));

	/* SDRC_MANUAL command register */
	/* NOP command */
	writel(0x00000000, OMAP3_SDRC_REG(MANUAL_0));
	/* Precharge command */
	writel(0x00000001, OMAP3_SDRC_REG(MANUAL_0));
	/* Auto-refresh command */
	writel(0x00000002, OMAP3_SDRC_REG(MANUAL_0));
	/* Auto-refresh command */
	writel(0x00000002, OMAP3_SDRC_REG(MANUAL_0));

	/* SDRC MR0 register Burst length=4 */
	writel(0x00000032, OMAP3_SDRC_REG(MR_0));

	/* SDRC DLLA control register */
	writel(0x0000000A, OMAP3_SDRC_REG(DLLA_CTRL));

	return;
}

/**
 * @brief Do the necessary pin muxing required for OMAP3EVM. Some pins in OMAP3
 * do not have alternate modes. We don't program these pins.
 *
 * See @ref MUX_VAL for description of the muxing mode.
 *
 * @return void
 */
static void mux_config(void)
{
	/*
	 * SDRC
	 * - SDRC_D0-SDRC_D31: Default MUX mode is mode0.
	 */

	/*
	 * GPMC
	 * - GPMC_D0-GPMC_D7: Default MUX mode is mode0.
	 * - GPMC_NADV_ALE: Default MUX mode is mode0.
	 * - GPMC_NOE: Default MUX mode is mode0.
	 * - GPMC_NWE: Default MUX mode is mode0.
	 * - GPMC_WAIT0: Default MUX mode is mode0.
	 */
	MUX_VAL(CP(GPMC_A1),		(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_A2),		(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_A3),		(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_A4),		(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_A5),		(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_A6),		(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_A7),		(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_A8),		(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_A9),		(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_A10),		(IDIS | PTD | DIS | M0));

	MUX_VAL(CP(GPMC_D8),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_D9),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_D10),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_D11),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_D12),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_D13),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_D14),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_D15),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_CLK),		(IDIS | PTD | DIS | M0));

	MUX_VAL(CP(GPMC_NBE0_CLE),	(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_NBE1),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_NWP),		(IEN  | PTD | DIS | M0));

	MUX_VAL(CP(GPMC_WAIT1),		(IEN  | PTU | EN  | M0));

	/*
	 * Serial Interface
	 */
#if defined(CONFIG_OMAP3EVM_UART1)
	MUX_VAL(CP(UART1_TX),		(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(UART1_RTS),		(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(UART1_CTS),		(IEN  | PTU | DIS | M0));
	MUX_VAL(CP(UART1_RX),		(IEN  | PTD | DIS | M0));
#elif defined(CONFIG_OMAP3EVM_UART3)
	MUX_VAL(CP(UART3_CTS_RCTX),	(IEN  | PTD | EN  | M0));
	MUX_VAL(CP(UART3_RTS_SD),	(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(UART3_RX_IRRX),	(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(UART3_TX_IRTX),	(IDIS | PTD | DIS | M0));
#endif
}

/**
 * @brief The basic entry point for board initialization.
 *
 * This is called as part of machine init (after arch init).
 * This is again called with stack in SRAM, so not too many
 * constructs possible here.
 *
 * @return void
 */
static int omap3_evm_board_init(void)
{
	int in_sdram = running_in_sdram();

	omap3_core_init();

	mux_config();

	/* Dont reconfigure SDRAM while running in SDRAM! */
	if (!in_sdram)
		sdrc_init();

	return 0;
}
pure_initcall(omap3_evm_board_init);

/*
 * Run-time initialization(s)
 */

#ifdef CONFIG_DRIVER_SERIAL_NS16550

/**
 * @brief Initialize the serial port to be used as console.
 *
 * @return result of device registration
 */
static int omap3evm_init_console(void)
{
	if (IS_ENABLED(CONFIG_OMAP3EVM_UART1))
		omap3_add_uart1();
	if (IS_ENABLED(CONFIG_OMAP3EVM_UART3))
		omap3_add_uart3();

	return 0;
}
console_initcall(omap3evm_init_console);
#endif /* CONFIG_DRIVER_SERIAL_NS16550 */

static int omap3evm_mem_init(void)
{
	omap_add_ram0(SZ_128M);

	return 0;
}
mem_initcall(omap3evm_mem_init);

static int omap3evm_init_devices(void)
{
#ifdef CONFIG_OMAP_GPMC
	/*
	 * WP is made high and WAIT1 active Low
	 */
	gpmc_generic_init(0x10);
#endif
	omap3_add_mmc1(NULL);

        armlinux_set_bootparams((void *)0x80000100);
        armlinux_set_architecture(MACH_TYPE_OMAP3EVM);

	return 0;
}
device_initcall(omap3evm_init_devices);
