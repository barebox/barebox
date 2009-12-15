/*
 * (C) Copyright 2008
 * Texas Instruments, <www.ti.com>
 * Raghavendra KH <r-khandenahally@ti.com>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/**
 * @file
 * @brief Beagle Specific Board Initialization routines
 */

/**
 * @page ti_beagle Texas Instruments Beagle Board
 *
 * FileName: board/omap/board-beagle.c
 *
 * Beagle Board from Texas Instruments as described here:
 * http://www.beagleboard.org
 *
 * This board is based on OMAP3530.
 * More on OMAP3530 (including documentation can be found here):
 * http://focus.ti.com/docs/prod/folders/print/omap3530.html
 *
 * This file provides initialization in two stages:
 * @li boot time initialization - do basics required to get SDRAM working.
 * This is run from SRAM - so no case constructs and global vars can be used.
 * @li run time initialization - this is for the rest of the initializations
 * such as flash, uart etc.
 *
 * Boot time initialization includes:
 * @li SDRAM initialization.
 * @li Pin Muxing relevant for Beagle.
 *
 * Run time initialization includes
 * @li serial @ref serial_ns16550.c driver device definition
 *
 * Originally from board/omap/board-sdp343x.c
 */

#include <common.h>
#include <console.h>
#include <init.h>
#include <driver.h>
#include <asm/io.h>
#include <ns16550.h>
#include <asm/armlinux.h>
#include <mach/silicon.h>
#include <mach/sdrc.h>
#include <mach/sys_info.h>
#include <mach/syslib.h>
#include <mach/control.h>
#include <mach/omap3-mux.h>
#include <mach/gpmc.h>
#include "board.h"

/******************** Board Boot Time *******************/

/**
 * @brief Do the SDRC initialization for 128Meg Micron DDR for CS0
 *
 * @return void
 */
static void sdrc_init(void)
{
       /* SDRAM software reset */
       /* No idle ack and RESET enable */
       writel(0x1A, SDRC_REG(SYSCONFIG));
       sdelay(100);
       /* No idle ack and RESET disable */
       writel(0x18, SDRC_REG(SYSCONFIG));

       /* SDRC Sharing register */
       /* 32-bit SDRAM on data lane [31:0] - CS0 */
       /* pin tri-stated = 1 */
       writel(0x00000100, SDRC_REG(SHARING));

       /* ----- SDRC Registers Configuration --------- */
       /* SDRC_MCFG0 register */
       writel(0x02584099, SDRC_REG(MCFG_0));

       /* SDRC_RFR_CTRL0 register */
       writel(0x54601, SDRC_REG(RFR_CTRL_0));

       /* SDRC_ACTIM_CTRLA0 register */
       writel(0xA29DB4C6, SDRC_REG(ACTIM_CTRLA_0));

       /* SDRC_ACTIM_CTRLB0 register */
       writel(0x12214, SDRC_REG(ACTIM_CTRLB_0));

       /* Disble Power Down of CKE due to 1 CKE on combo part */
       writel(0x00000081, SDRC_REG(POWER));

       /* SDRC_MANUAL command register */
       /* NOP command */
       writel(0x00000000, SDRC_REG(MANUAL_0));
       /* Precharge command */
       writel(0x00000001, SDRC_REG(MANUAL_0));
       /* Auto-refresh command */
       writel(0x00000002, SDRC_REG(MANUAL_0));
       /* Auto-refresh command */
       writel(0x00000002, SDRC_REG(MANUAL_0));

       /* SDRC MR0 register Burst length=4 */
       writel(0x00000032, SDRC_REG(MR_0));

       /* SDRC DLLA control register */
       writel(0x0000000A, SDRC_REG(DLLA_CTRL));

       return;
}

/**
 * @brief Do the pin muxing required for Board operation.
 * We enable ONLY the pins we require to set. OMAP provides pins which do not
 * have alternate modes. Such pins done need to be set.
 *
 * See @ref MUX_VAL for description of the muxing mode.
 *
 * @return void
 */
static void mux_config(void)
{

       /* SDRC_D0 - SDRC_D31 default mux mode is mode0 */

       /* GPMC */
       MUX_VAL(CP(GPMC_A1), (IDIS | PTD | DIS | M0));
       MUX_VAL(CP(GPMC_A2), (IDIS | PTD | DIS | M0));
       MUX_VAL(CP(GPMC_A3), (IDIS | PTD | DIS | M0));
       MUX_VAL(CP(GPMC_A4), (IDIS | PTD | DIS | M0));
       MUX_VAL(CP(GPMC_A5), (IDIS | PTD | DIS | M0));
       MUX_VAL(CP(GPMC_A6), (IDIS | PTD | DIS | M0));
       MUX_VAL(CP(GPMC_A7), (IDIS | PTD | DIS | M0));
       MUX_VAL(CP(GPMC_A8), (IDIS | PTD | DIS | M0));
       MUX_VAL(CP(GPMC_A9), (IDIS | PTD | DIS | M0));
       MUX_VAL(CP(GPMC_A10), (IDIS | PTD | DIS | M0));

       /* D0-D7 default mux mode is mode0 */
       MUX_VAL(CP(GPMC_D8), (IEN | PTD | DIS | M0));
       MUX_VAL(CP(GPMC_D9), (IEN | PTD | DIS | M0));
       MUX_VAL(CP(GPMC_D10), (IEN | PTD | DIS | M0));
       MUX_VAL(CP(GPMC_D11), (IEN | PTD | DIS | M0));
       MUX_VAL(CP(GPMC_D12), (IEN | PTD | DIS | M0));
       MUX_VAL(CP(GPMC_D13), (IEN | PTD | DIS | M0));
       MUX_VAL(CP(GPMC_D14), (IEN | PTD | DIS | M0));
       MUX_VAL(CP(GPMC_D15), (IEN | PTD | DIS | M0));
       MUX_VAL(CP(GPMC_CLK), (IDIS | PTD | DIS | M0));
       /* GPMC_NADV_ALE default mux mode is mode0 */
       /* GPMC_NOE default mux mode is mode0 */
       /* GPMC_NWE default mux mode is mode0 */
       /* GPMC_NBE0_CLE default mux mode is mode0 */
       MUX_VAL(CP(GPMC_NBE0_CLE), (IDIS | PTD | DIS | M0));
       MUX_VAL(CP(GPMC_NBE1), (IEN | PTD | DIS | M0));
       MUX_VAL(CP(GPMC_NWP), (IEN | PTD | DIS | M0));
       /* GPMC_WAIT0 default mux mode is mode0 */
       MUX_VAL(CP(GPMC_WAIT1), (IEN | PTU | EN | M0));

       /* SERIAL INTERFACE */
       MUX_VAL(CP(UART3_CTS_RCTX), (IEN | PTD | EN | M0));
       MUX_VAL(CP(UART3_RTS_SD), (IDIS | PTD | DIS | M0));
       MUX_VAL(CP(UART3_RX_IRRX), (IEN | PTD | DIS | M0));
       MUX_VAL(CP(UART3_TX_IRTX), (IDIS | PTD | DIS | M0));
       MUX_VAL(CP(HSUSB0_CLK), (IEN | PTD | DIS | M0));
       MUX_VAL(CP(HSUSB0_STP), (IDIS | PTU | EN | M0));
       MUX_VAL(CP(HSUSB0_DIR), (IEN | PTD | DIS | M0));
       MUX_VAL(CP(HSUSB0_NXT), (IEN | PTD | DIS | M0));
       MUX_VAL(CP(HSUSB0_DATA0), (IEN | PTD | DIS | M0));
       MUX_VAL(CP(HSUSB0_DATA1), (IEN | PTD | DIS | M0));
       MUX_VAL(CP(HSUSB0_DATA2), (IEN | PTD | DIS | M0));
       MUX_VAL(CP(HSUSB0_DATA3), (IEN | PTD | DIS | M0));
       MUX_VAL(CP(HSUSB0_DATA4), (IEN | PTD | DIS | M0));
       MUX_VAL(CP(HSUSB0_DATA5), (IEN | PTD | DIS | M0));
       MUX_VAL(CP(HSUSB0_DATA6), (IEN | PTD | DIS | M0));
       MUX_VAL(CP(HSUSB0_DATA7), (IEN | PTD | DIS | M0));
       /* I2C1_SCL default mux mode is mode0 */
       /* I2C1_SDA default mux mode is mode0 */
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
void board_init(void)
{
       int in_sdram = running_in_sdram();
       mux_config();
       /* Dont reconfigure SDRAM while running in SDRAM! */
       if (!in_sdram)
               sdrc_init();
}

/******************** Board Run Time *******************/

#ifdef CONFIG_DRIVER_SERIAL_NS16550

static struct NS16550_plat serial_plat = {
       .clock = 48000000,      /* 48MHz (APLL96/2) */
       .f_caps = CONSOLE_STDIN | CONSOLE_STDOUT | CONSOLE_STDERR,
       .reg_read = omap_uart_read,
       .reg_write = omap_uart_write,
};

static struct device_d beagle_serial_device = {
       .name = "serial_ns16550",
       .map_base = OMAP_UART3_BASE,
       .size = 1024,
       .platform_data = (void *)&serial_plat,
};

/**
 * @brief UART serial port initialization - remember to enable COM clocks in
 * arch
 *
 * @return result of device registration
 */
static int beagle_console_init(void)
{
       /* Register the serial port */
       return register_device(&beagle_serial_device);
}
console_initcall(beagle_console_init);
#endif /* CONFIG_DRIVER_SERIAL_NS16550 */

static struct memory_platform_data sram_pdata = {
	.name = "ram0",
	.flags = DEVFS_RDWR,
};

static struct device_d sdram_dev = {
	.name = "mem",
	.map_base = 0x80000000,
	.size = 128 * 1024 * 1024,
	.platform_data = &sram_pdata,
};

static int beagle_devices_init(void)
{
       int ret;
       ret = register_device(&sdram_dev);
       if (ret)
               goto failed;
#ifdef CONFIG_GPMC
	/* WP is made high and WAIT1 active Low */
	gpmc_generic_init(0x10);
#endif
       armlinux_add_dram(&sdram_dev);
failed:
       return ret;
}
device_initcall(beagle_devices_init);
