/**
 * @file
 * @brief Board Initialization routines for the phyCARD-A-L1
 *
 * FileName: arch/arm/boards/phycard-a-l1/pca-a-l1.c
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
 * Copyright (C) 2011 Phytec Messtechnik GmbH - http://www.phytec.de/
 * Juergen Kilb <j.kilb@phytec.de>
 *
 * based on code from Texas Instruments / board-beagle.c
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <console.h>
#include <driver.h>
#include <errno.h>
#include <init.h>
#include <nand.h>
#include <ns16550.h>
#include <partition.h>
#include <sizes.h>
#include <asm/armlinux.h>
#include <asm/io.h>
#include <generated/mach-types.h>
#include <linux/err.h>
#include <mach/control.h>
#include <mach/gpmc.h>
#include <mach/gpmc_nand.h>
#include <mach/omap_hsmmc.h>
#include <mach/xload.h>
#include <mach/omap3-mux.h>
#include <mach/sdrc.h>
#include <mach/silicon.h>
#include <mach/sys_info.h>
#include <mach/syslib.h>

#define SMC911X_BASE 0x2c000000

/* Slower full frequency range default timings for x32 operation */
#define SDP_SDRC_SHARING	0x00000100
/* Diabling power down mode using CKE pin */
#define SDP_SDRC_POWER_POP	0x00000081
/* rkw - need to find of 90/72 degree recommendation for speed like before. */
#define SDP_SDRC_DLLAB_CTRL ((DLL_ENADLL << 3) | \
	(DLL_LOCKDLL << 2) | (DLL_DLLPHASE_90 << 1))

/* used to create an array of memory configuartions. */
struct sdrc_config {
	u32	cs_cfg;
	u32	mcfg;
	u32	mr;
	u32	actim_ctrla;
	u32	actim_ctrlb;
	u32	rfr_ctrl;
} const sdrc_config[] = {
/* max cs_size for autodetection, common timing */
/* 2x256MByte, 14 Rows, 10 Columns , RBC (BAL=2) */
{ 0x00000004, 0x03590099, 0x00000032, 0x9A9DB4C6, 0x00011216, 0x0004e201},
/* MT46H32M32LF 2x128MByte, 13 Rows, 10 Columns */
{ 0x00000001, 0x02584099, 0x00000032, 0x9A9DB4C6, 0x00011216, 0x0004e201},
/* MT46H64M32LF 1x256MByte, 14 Rows, 10 Columns */
{ 0x00000002, 0x03588099, 0x00000032, 0x629DB4C6, 0x00011113, 0x0004e201},
/* MT64H128M32L2 2x256MByte, 14 Rows, 10 Columns */
{ 0x00000002, 0x03588099, 0x00000032, 0x629DB4C6, 0x00011113, 0x0004e201},
};

/*
 * Boot-time initialization(s)
 */

/*********************************************************************
 * init_sdram_ddr() - Init DDR controller.
 *********************************************************************/
void init_sdram_ddr(void)
{
	/* reset sdrc controller */
	writel(SOFTRESET, SDRC_REG(SYSCONFIG));
	wait_on_value(1<<0, 1<<0, SDRC_REG(STATUS), 12000000);
	writel(0, SDRC_REG(SYSCONFIG));

	/* setup sdrc to ball mux */
	writel(SDP_SDRC_SHARING, SDRC_REG(SHARING));
	writel(SDP_SDRC_POWER_POP, SDRC_REG(POWER));

	/* set up dll */
	writel(SDP_SDRC_DLLAB_CTRL, SDRC_REG(DLLA_CTRL));
	sdelay(0x2000);	/* give time to lock */

}
/*********************************************************************
 * config_sdram_ddr() - Init DDR on dev board.
 *********************************************************************/
void config_sdram_ddr(u8 cs, u8 cfg)
{

	writel(sdrc_config[cfg].mcfg, SDRC_REG(MCFG_0) + (0x30 * cs));
	writel(sdrc_config[cfg].actim_ctrla, SDRC_REG(ACTIM_CTRLA_0) + (0x28 * cs));
	writel(sdrc_config[cfg].actim_ctrlb, SDRC_REG(ACTIM_CTRLB_0) + (0x28 * cs));
	writel(sdrc_config[cfg].rfr_ctrl, SDRC_REG(RFR_CTRL_0) + (0x30 * cs));

	writel(CMD_NOP, SDRC_REG(MANUAL_0) + (0x30 * cs));

	sdelay(5000);

	writel(CMD_PRECHARGE, SDRC_REG(MANUAL_0) + (0x30 * cs));
	writel(CMD_AUTOREFRESH, SDRC_REG(MANUAL_0) + (0x30 * cs));
	writel(CMD_AUTOREFRESH, SDRC_REG(MANUAL_0) + (0x30 * cs));

	/* set mr0 */
	writel(sdrc_config[cfg].mr, SDRC_REG(MR_0) + (0x30 * cs));

	sdelay(2000);
}

/**
 * @brief Initialize the SDRC module
 * Initialisation for 1x256MByte but normally
 * done by x-loader.
 * @return void
 */
static void pcaal1_sdrc_init(void)
{
	u32 test0, test1;
	char cfg;

	init_sdram_ddr();

	config_sdram_ddr(0, 0); /* 256MByte at CS0 */
	config_sdram_ddr(1, 0); /* 256MByte at CS1 */

	test0 = get_ram_size((long *) 0x80000000, SZ_256M);
	test1 = get_ram_size((long *) 0xA0000000, SZ_256M);

	/* mask out lower nible, its not tested with
	in common/memsize.c */
	test1 &= 0xfffffff0;

	if ((test1 > 0) && (test1 != test0))
		hang();

	cfg = -1; /* illegal configuration found */

	if (test1 == 0) {
		init_sdram_ddr();
		writel((sdrc_config[(uchar) cfg].mcfg & 0xfffc00ff), SDRC_REG(MCFG_1));

		/* 1 x 256MByte */
		if (test0 == SZ_256M)
			cfg = 2;

		if (cfg != -1) {
			config_sdram_ddr(0, cfg);
			writel(sdrc_config[(uchar) cfg].cs_cfg, SDRC_REG(CS_CFG));
		}
		return;
	}

	/* reinit both cs with correct size */
	/* 2 x 128MByte */
	if (test0 == SZ_128M)
		cfg = 1;
	/* 2 x 256MByte */
	if (test0 == SZ_256M)
		cfg = 3;

	if (cfg != -1) {
		init_sdram_ddr();
		writel(sdrc_config[(uchar) cfg].cs_cfg, SDRC_REG(CS_CFG));
		config_sdram_ddr(0, cfg);
		config_sdram_ddr(1, cfg);
	}
}

/**
 * @brief Do the necessary pin muxing required for phyCARD-A-L1.
 * Some pins in OMAP3 do not have alternate modes.
 * We don't program these pins.
 *
 * See @ref MUX_VAL for description of the muxing mode.
 *
 * @return void
 */
static void pcaal1_mux_config(void)
{
	/*
	 * Serial Interface
	 */
	MUX_VAL(CP(UART3_CTS_RCTX),	(IEN  | PTD | EN  | M0));
	MUX_VAL(CP(UART3_RTS_SD),	(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(UART3_RX_IRRX),	(IEN  | PTD | EN  | M0));
	MUX_VAL(CP(UART3_TX_IRTX),	(IDIS | PTD | DIS | M0));

	/* GPMC */
	MUX_VAL(CP(GPMC_A1),		(IDIS | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_A2),		(IDIS | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_A3),		(IDIS | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_A4),		(IDIS | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_A5),		(IDIS | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_A6),		(IDIS | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_A7),		(IDIS | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_A8),		(IDIS | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_A9),		(IDIS | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_A10),		(IDIS | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D0),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D1),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D2),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D3),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D4),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D5),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D6),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D7),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D8),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D9),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D10),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D11),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D12),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D13),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D14),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D15),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_NCS0),		(IDIS | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_NADV_ALE),	(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_NOE),		(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_NWE),		(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_NBE0_CLE),	(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_NWP),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_WAIT0),		(IEN  | PTU | EN  | M0));

	/* ETH_PME (GPIO_55) */
	MUX_VAL(CP(GPMC_NCS4),		(IDIS | PTU | EN  | M4));
	/* #CS5 (Ethernet) */
	MUX_VAL(CP(GPMC_NCS5),		(IDIS | PTU | EN  | M0));
	/* ETH_FIFO_SEL (GPIO_57) */
	MUX_VAL(CP(GPMC_NCS6),		(IDIS | PTD | EN  | M4));
	/* ETH_AMDIX_EN (GPIO_58) */
	MUX_VAL(CP(GPMC_NCS7),		(IDIS | PTU | EN  | M4));
	/* ETH_nRST (GPIO_64) */
	MUX_VAL(CP(GPMC_WAIT2),		(IDIS | PTU | EN  | M4));

	/* HSMMC1 */
	MUX_VAL(CP(MMC1_CLK),		(IDIS | PTU | EN  | M0));
	MUX_VAL(CP(MMC1_CMD),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(MMC1_DAT0),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(MMC1_DAT1),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(MMC1_DAT2),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(MMC1_DAT3),		(IEN  | PTU | EN  | M0));

	/* USBOTG_nRST (GPIO_63) */
	MUX_VAL(CP(GPMC_WAIT1),		(IDIS | PTU | EN  | M4));

	/* USBH_nRST (GPIO_65) */
	MUX_VAL(CP(GPMC_WAIT3),		(IDIS | PTU | EN  | M4));
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
static int pcaal1_board_init(void)
{
	int in_sdram = running_in_sdram();

	omap3_core_init();

	pcaal1_mux_config();
	/* Dont reconfigure SDRAM while running in SDRAM! */
	if (!in_sdram)
		pcaal1_sdrc_init();

	return 0;
}
pure_initcall(pcaal1_board_init);

/*
 * Run-time initialization(s)
 */
static struct NS16550_plat serial_plat = {
	.clock	= 48000000,      /* 48MHz (APLL96/2) */
	.shift	= 2,
};

/**
 * @brief Initialize the serial port to be used as console.
 *
 * @return result of device registration
 */
static int pcaal1_init_console(void)
{
	add_ns16550_device(-1, OMAP_UART3_BASE, 1024, IORESOURCE_MEM_8BIT,
			   &serial_plat);

	return 0;
}
console_initcall(pcaal1_init_console);

#ifdef CONFIG_DRIVER_NET_SMC911X
/** GPMC timing for our SMSC9221 device */
static struct gpmc_config smsc_cfg = {
	.cfg = {
		0x41001000,	/*CONF1 */
		0x00040500,	/*CONF2 */
		0x00000000,	/*CONF3 */
		0x04000500,	/*CONF4 */
		0x05050505,	/*CONF5 */
		0x000002c1,	/*CONF6 */
	},
	.base = SMC911X_BASE,
	/* GPMC address map as small as possible */
	.size = GPMC_SIZE_16M,
};

/*
 * Routine: setup_net_chip
 * Description: Setting up the configuration GPMC registers specific to the
 *            Ethernet hardware.
 */
static void pcaal1_setup_net_chip(void)
{
	gpmc_cs_config(5, &smsc_cfg);
}
#endif

static int pcaal1_mem_init(void)
{

#ifdef CONFIG_OMAP_GPMC
	/*
	 * WP is made high and WAIT1 active Low
	 */
	gpmc_generic_init(0x10);
#endif
	add_mem_device("sram0", OMAP_SRAM_BASE, 60 * SZ_1K,
				   IORESOURCE_MEM_WRITEABLE);

	arm_add_mem_device("ram0", OMAP_SDRC_CS0, get_sdr_cs_size(SDRC_CS0_OSET));
	printf("found %s at SDCS0\n", size_human_readable(get_sdr_cs_size(SDRC_CS0_OSET)));

	if ((get_sdr_cs_size(SDRC_CS1_OSET) != 0) && (get_sdr_cs1_base() != OMAP_SDRC_CS0)) {
		arm_add_mem_device("ram1", get_sdr_cs1_base(), get_sdr_cs_size(SDRC_CS1_OSET));
		printf("found %s at SDCS1\n", size_human_readable(get_sdr_cs_size(SDRC_CS1_OSET)));
	}

	return 0;
}
mem_initcall(pcaal1_mem_init);

#ifdef CONFIG_MCI_OMAP_HSMMC
struct omap_hsmmc_platform_data pcaal1_hsmmc_plat = {
	.f_max = 26000000,
};
#endif

static int pcaal1_init_devices(void)
{
	gpmc_generic_nand_devices_init(0, 16, OMAP_ECC_BCH8_CODE_HW, &omap3_nand_cfg);
#ifdef CONFIG_MCI_OMAP_HSMMC
	add_generic_device("omap-hsmmc", DEVICE_ID_DYNAMIC, NULL, OMAP_MMC1_BASE, SZ_4K,
			   IORESOURCE_MEM, &pcaal1_hsmmc_plat);
#endif

#ifdef CONFIG_DRIVER_NET_SMC911X
	pcaal1_setup_net_chip();
	add_generic_device("smc911x", DEVICE_ID_DYNAMIC, NULL, SMC911X_BASE, SZ_4K,
			   IORESOURCE_MEM, NULL);
#endif

	armlinux_set_bootparams((void *)0x80000100);
	armlinux_set_architecture(MACH_TYPE_PCAAL1);

	return 0;
}
device_initcall(pcaal1_init_devices);

static int pcaal1_late_init(void)
{
#ifdef CONFIG_PARTITION
	devfs_add_partition("nand0", 0x00000, 0x80000, DEVFS_PARTITION_FIXED, "x-loader");
	dev_add_bb_dev("self_raw", "x_loader0");

	devfs_add_partition("nand0", 0x80000, 0x1e0000, DEVFS_PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");

	devfs_add_partition("nand0", 0x260000, 0x20000, DEVFS_PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");
#endif
	return 0;
}
late_initcall(pcaal1_late_init);
