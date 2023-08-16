// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "skov-imx6: " fmt

#include <common.h>
#include <mach/imx/generic.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <debug_ll.h>
#include <mach/imx/debug_ll.h>
#include <io.h>
#include <mach/imx/imx6-mmdc.h>
#include <mach/imx/imx6-ddr-regs.h>
#include <mach/imx/imx6.h>
#include <mach/imx/xload.h>
#include <mach/imx/esdctl.h>
#include <serial/imx-uart.h>
#include <mach/imx/iomux-mx6.h>
#include <mach/imx/imx-gpio.h>
#include "version.h"

static void __udelay(int us)
{
	volatile int i;

	for (i = 0; i < us * 4; i++);
}

/* ------------------------------------------------------------------------ */

/*
 * Micron MT41K512M16HA-125 IT:E ->  8 GBit = 64 Meg x 16 x 8 banks
 *
 * Speed Grade   Data Rate (MT/s)  tRCD-tRP-CL   tRCD(ns)  tRP(ns)  CL(ns)
 *    -125          1600            11-11-11      13.75     13.75   13.75
 *               (=800 MHz)
 *
 * Memory configuration used by variant:
 * - "Max Performance", 64 bit data bus, 1066 MHz, 4 GiB memory
 */
static const struct mx6_ddr3_cfg skov_imx6_cfg_4x512Mb_1066MHz = {
	.mem_speed = 1066,
	.density = 8, /* GiBit */
	.width = 16, /* 16 bit data per device */
	.banks = 8,
	.rowaddr = 16, /* 64 k */
	.coladdr = 10, /* 1 k */
	.pagesz = 2, /* [kiB] */
	.trcd = 1375, /* 13.75 ns = 11 clocks @ 1.6 GHz */
	.trcmin = 4875, /* 48.75 ns = 39 clocks @ 1.6 GHz */
	.trasmin = 3500, /* 35 ns = 28 clocks @ 1.6 GHz */
	.SRT = 0,
};

static const struct mx6_ddr_sysinfo skov_imx6_sysinfo_4x512Mb_1066MHz = {
	.dsize = 2, /* 64 bit wide = 4 devices, 16 bit each */
	.cs_density = 32, /* four 8 GBit devices connected */
	.ncs = 1, /* one CS line for all devices */
	.cs1_mirror = 1, /* TODO */
	.bi_on = 1, /* TODO */
	.rtt_nom = 1, /* MX6_MMDC_P0_MPODTCTRL -> 0x00022227 */
	.rtt_wr = 0, /* is LW_EN is 0 in their code */
	.ralat = 5, /* TODO */
	.walat = 1, /* TODO */
	.mif3_mode = 3, /* TODO */
	.rst_to_cke = 0x23, /* used in their code as well */
	.sde_to_rst = 0x10, /* used in their code as well */
	.pd_fast_exit = 0, /* TODO */
};

static const struct mx6_mmdc_calibration skov_imx6_calib_4x512Mb_1066MHz = {
	.p0_mpwldectrl0 = 0x001a0017,
	.p0_mpwldectrl1 = 0x001F001F,
	.p0_mpdgctrl0 = 0x43040319,
	.p0_mpdgctrl1 = 0x03040279,
	.p0_mprddlctl = 0x4d434248,
	.p0_mpwrdlctl = 0x34424543,

	.p1_mpwldectrl0 = 0x00170027,
	.p1_mpwldectrl1 = 0x000a001f,
	.p1_mpdgctrl0 = 0x43040321,
	.p1_mpdgctrl1 = 0x03030251,
	.p1_mprddlctl = 0x42413c4d,
	.p1_mpwrdlctl = 0x49324933,
};

/* ------------------------------------------------------------------------ */

/*
 * Micron MT41K256M16HA-125 IT:E ->  4 GBit = 32 Meg x 16 x 8 banks
 *
 * Speed Grade   Data Rate (MT/s)  tRCD-tRP-CL   tRCD(ns)  tRP(ns)  CL(ns)
 *    -125          1600            11-11-11      13.75     13.75   13.75
 *               (=800 MHz)
 *
 * Memory configuration used by variant:
 * - "Max Performance", 64 bit data bus, 1066 MHz, 2 GiB memory
 */
static const struct mx6_ddr3_cfg skov_imx6_cfg_4x256Mb_1066MHz = {
	.mem_speed = 1066,
	.density = 4, /* GiBit */
	.width = 16, /* 16 bit data per device */
	.banks = 8,
	.rowaddr = 15, /* 32 k */
	.coladdr = 10, /* 1 k */
	.pagesz = 2, /* [kiB] */
	.trcd = 1375, /* 13.75 ns = 11 clocks @ 1.6 GHz */
	.trcmin = 4875, /* 48.75 ns = 39 clocks @ 1.6 GHz */
	.trasmin = 3500, /* 35 ns = 28 clocks @ 1.6 GHz */
	.SRT = 0,
};

static const struct mx6_ddr_sysinfo skov_imx6_sysinfo_4x256Mb_1066MHz = {
	.dsize = 2, /* 64 bit wide = 4 devices, 16 bit each */
	.cs_density = 16, /* four 4 GBit devices connected */
	.ncs = 1, /* one CS line for all devices */
	.cs1_mirror = 1, /* TODO */
	.bi_on = 1, /* TODO */
	.rtt_nom = 1, /* MX6_MMDC_P0_MPODTCTRL -> 0x00022227 */
	.rtt_wr = 0, /* is LW_EN is 0 in their code */
	.ralat = 5, /* TODO */
	.walat = 1, /* TODO */
	.mif3_mode = 3, /* TODO */
	.rst_to_cke = 0x23, /* used in their code as well */
	.sde_to_rst = 0x10, /* used in their code as well */
	.pd_fast_exit = 0, /* TODO */
};

static const struct mx6_mmdc_calibration skov_imx6_calib_4x256Mb_1066MHz = {
	.p0_mpwldectrl0 = 0x001a0017,
	.p0_mpwldectrl1 = 0x001F001F,
	.p0_mpdgctrl0 = 0x43040319,
	.p0_mpdgctrl1 = 0x03040279,
	.p0_mprddlctl = 0x4d434248,
	.p0_mpwrdlctl = 0x34424543,

	.p1_mpwldectrl0 = 0x00170027,
	.p1_mpwldectrl1 = 0x000a001f,
	.p1_mpdgctrl0 = 0x43040321,
	.p1_mpdgctrl1 = 0x03030251,
	.p1_mprddlctl = 0x42413c4d,
	.p1_mpwrdlctl = 0x49324933,
};

/* ------------------------------------------------------------------------ */

/*
 * Micron MT41K128M16JT-125 IT:K ->  2 GBit = 16 Meg x 16 x 8 banks
 *
 * Speed Grade   Data Rate (MT/s)  tRCD-tRP-CL   tRCD(ns)  tRP(ns)  CL(ns)
 *    -125 ¹²       1600            11-11-11      13.75     13.75   13.75
 *               (=800 MHz)
 *
 * ¹ Backward compatible to 1066 (=533 MHz), CL = 7
 * ² Backward compatible to 1333 (=667 MHz), CL = 9
 *
 * Memory configuration used by variant
 * - "High Performance", 64 bit data bus, 1066 MHz, 1 GiB memory
 */
static const struct mx6_ddr3_cfg skov_imx6_cfg_4x128Mb_1066MHz = {
	.mem_speed = 1066,
	.density = 2, /* GiBit */
	.width = 16, /* 16 bit data per device */
	.banks = 8,
	.rowaddr = 14, /* 16 k */
	.coladdr = 10, /* 1 k */
	.pagesz = 2, /* [kiB] */
	.trcd = 1375, /* 13.75 ns = 11 clocks @ 1.6 GHz */
	.trcmin = 4875, /* 48.75 ns = 39 clocks @ 1.6 GHz */
	.trasmin = 3500, /* 35 ns = 28 clocks @ 1.6 GHz */
	.SRT = 0,
};

static const struct mx6_ddr_sysinfo skov_imx6_sysinfo_4x128Mb_1066MHz = {
	.dsize = 2, /* 64 bit wide = 4 devices, 16 bit each */
	.cs_density = 8, /* four 2 GBit devices connected */
	.ncs = 1, /* one CS line for all devices */
	.cs1_mirror = 1,
	.bi_on = 1,
	.rtt_nom = 1, /* MX6_MMDC_P0_MPODTCTRL -> 0x00022227 */
	.rtt_wr = 0, /* is LW_EN is 0 in their code */
	.ralat = 5,
	.walat = 0,
	.mif3_mode = 3,
	.rst_to_cke = 0x23,
	.sde_to_rst = 0x10,
	.pd_fast_exit = 1,
};

/* calibration info for the "max performance" and "high performance" */
static const struct mx6_mmdc_calibration skov_imx6_calib_4x128Mb_1066MHz = {
	.p0_mpwldectrl0 = 0x00230023,
	.p0_mpwldectrl1 = 0x0029001E,
	.p0_mpdgctrl0 = 0x43400350,
	.p0_mpdgctrl1 = 0x03380330,
	.p0_mprddlctl = 0x3E323638,
	.p0_mpwrdlctl = 0x383A3E3A,

	.p1_mpwldectrl0 = 0x001F002A,
	.p1_mpwldectrl1 = 0x001A0028,
	.p1_mpdgctrl0 = 0x43300340,
	.p1_mpdgctrl1 = 0x03340300,
	.p1_mprddlctl = 0x383A3242,
	.p1_mpwrdlctl = 0x4232463A,
};

/* ------------------------------------------------------------------------ */

static struct mx6dq_iomux_ddr_regs ddr_iomux_q = {
	.dram_sdqs0 = 0x00000030,
	.dram_sdqs1 = 0x00000030,
	.dram_sdqs2 = 0x00000030,
	.dram_sdqs3 = 0x00000030,
	.dram_sdqs4 = 0x00000030,
	.dram_sdqs5 = 0x00000030,
	.dram_sdqs6 = 0x00000030,
	.dram_sdqs7 = 0x00000030,
	.dram_dqm0 = 0x00000030,
	.dram_dqm1 = 0x00000030,
	.dram_dqm2 = 0x00000030,
	.dram_dqm3 = 0x00000030,
	.dram_dqm4 = 0x00000030,
	.dram_dqm5 = 0x00000030,
	.dram_dqm6 = 0x00000030,
	.dram_dqm7 = 0x00000030,
	.dram_cas = 0x00000030,
	.dram_ras = 0x00000030,
	.dram_sdclk_0 = 0x00000030,
	.dram_sdclk_1 = 0x00000030,
	.dram_sdcke0 = 0x00003000,
	.dram_sdcke1 = 0x00003000,
	.dram_reset = 0x00000030,
	.dram_sdba2 = 0x00000000,
	.dram_sdodt0 = 0x00003030,
	.dram_sdodt1 = 0x00003030,
};

static struct mx6dq_iomux_grp_regs grp_iomux_q = {
	.grp_b0ds = 0x00000030,
	.grp_b1ds = 0x00000030,
	.grp_b2ds = 0x00000030,
	.grp_b3ds = 0x00000030,
	.grp_b4ds = 0x00000030,
	.grp_b5ds = 0x00000030,
	.grp_b6ds = 0x00000030,
	.grp_b7ds = 0x00000030,
	.grp_addds = 0x00000030,
	.grp_ddrmode_ctl = 0x00020000,
	.grp_ddrpke = 0x00000000,
	.grp_ddrmode = 0x00020000,
	.grp_ctlds = 0x00000030,
	.grp_ddr_type = 0x000C0000,
};

static void spl_imx6q_dram_init(const struct mx6_ddr_sysinfo *si,
				const struct mx6_mmdc_calibration *cb,
				const struct mx6_ddr3_cfg *cfg)
{
	mx6dq_dram_iocfg(64, &ddr_iomux_q, &grp_iomux_q);
	mx6_dram_cfg(si, cb, cfg);
	__udelay(100);
}

/* ------------------------------------------------------------------------ */
/*
 * Device Information: Varies per DDR3 part number and speed grade
 * Note: this SDRAM type is used on the "Low Cost" variant
 *
 * Micron MT41K128M16JT-125 IT:K ->  2 GBit = 16 Meg x 16 x 8 banks
 *
 * Speed Grade   Data Rate (MT/s)  tRCD-tRP-CL   tRCD(ns)  tRP(ns)  CL(ns)
 *    -125 ¹²       1600            11-11-11      13.75     13.75   13.75
 *               (=800 MHz)
 *
 * ¹ Backward compatible to 1066 (=533 MHz), CL = 7
 * ² Backward compatible to 1333 (=667 MHz), CL = 9
 *
 * Memory configuration used by variant
 * - "Low Cost", 32 bit data bus, 800 MHz, 512 MiB memory
 */
static const struct mx6_ddr3_cfg skov_imx6_cfg_2x128Mb_800MHz = {
	.mem_speed = 800,
	.density = 2, /* GiBit */
	.width = 16, /* 16 bit data per device */
	.banks = 8,
	.rowaddr = 14, /* 16 k */
	.coladdr = 10, /* 1 k */
	.pagesz = 2, /* [kiB] */
	.trcd = 1375, /* 13.75 ns = 11 clocks @ 1.6 GHz */
	.trcmin = 4875, /* 48.75 ns = 39 clocks @ 1.6 GHz */
	.trasmin = 3500, /* 35 ns = 28 clocks @ 1.6 GHz */
	.SRT = 0,
};

static const struct mx6_ddr_sysinfo skov_imx6_sysinfo_2x128Mb_800MHz = {
	.dsize = 1, /* 32 bit wide = 2 devices, 16 bit each */
	.cs_density = 4, /* two 2 GBit devices connected */
	.ncs = 1, /* one CS line for all devices */
	.cs1_mirror = 1,
	.bi_on = 1,
	.rtt_nom = 1, /* MX6_MMDC_P0_MPODTCTRL -> 0x00022227 */
	.rtt_wr = 0, /* is LW_EN is 0 in their code */
	.ralat = 5,
	.walat = 0,
	.mif3_mode = 3,
	.rst_to_cke = 0x23,
	.sde_to_rst = 0x10,
	.pd_fast_exit = 1,
};

static const struct mx6_mmdc_calibration skov_imx6_calib_2x128Mb_800MHz = {
	.p0_mpwldectrl0 = 0x004A004B,
	.p0_mpwldectrl1 = 0x00420046,
	.p0_mpdgctrl0 = 0x42400240,
	.p0_mpdgctrl1 = 0x02300230,
	.p0_mprddlctl = 0x464A4A4A,
	.p0_mpwrdlctl = 0x32342A32,
};

/* ------------------------------------------------------------------------ */

static const struct mx6sdl_iomux_ddr_regs ddr_iomux_s = {
	.dram_sdqs0 = 0x00000030,
	.dram_sdqs1 = 0x00000030,
	.dram_sdqs2 = 0x00000030,
	.dram_sdqs3 = 0x00000030,
	.dram_sdqs4 = 0x00000030,
	.dram_sdqs5 = 0x00000030,
	.dram_sdqs6 = 0x00000030,
	.dram_sdqs7 = 0x00000030,
	.dram_dqm0 = 0x00000030,
	.dram_dqm1 = 0x00000030,
	.dram_dqm2 = 0x00000030,
	.dram_dqm3 = 0x00000030,
	.dram_dqm4 = 0x00000030,
	.dram_dqm5 = 0x00000030,
	.dram_dqm6 = 0x00000030,
	.dram_dqm7 = 0x00000030,
	.dram_cas = 0x00000030,
	.dram_ras = 0x00000030,
	.dram_sdclk_0 = 0x00000030,
	.dram_sdclk_1 = 0x0000030,
	.dram_sdcke0 = 0x00003000,
	.dram_sdcke1 = 0x00003000,
	.dram_reset = 0x00000030,
	.dram_sdba2 = 0x00000000,
	.dram_sdodt0 = 0x00003030,
	.dram_sdodt1 = 0x00003030,
};

static const struct mx6sdl_iomux_grp_regs grp_iomux_s = { /* TODO */
	.grp_b0ds = 0x00000030,
	.grp_b1ds = 0x00000030,
	.grp_b2ds = 0x00000030,
	.grp_b3ds = 0x00000030,
	.grp_b4ds = 0x00000030,
	.grp_b5ds = 0x00000030,
	.grp_b6ds = 0x00000030,
	.grp_b7ds = 0x00000030,
	.grp_addds = 0x00000030,
	.grp_ddrmode_ctl = 0x00020000,
	.grp_ddrpke = 0x00000000,
	.grp_ddrmode = 0x00020000,
	.grp_ctlds = 0x00000030,
	.grp_ddr_type = 0x000C0000,
};

static void spl_imx6sdl_dram_init(const struct mx6_ddr_sysinfo *si,
				const struct mx6_mmdc_calibration *cb,
				const struct mx6_ddr3_cfg *cfg)
{
	mx6sdl_dram_iocfg(64, &ddr_iomux_s, &grp_iomux_s);
	mx6_dram_cfg(si, cb, cfg);
	__udelay(100);
}

/* ------------------------------------------------------------------------ */

#define BKLGT_PWR_PAD_CTRL MX6_PAD_CTL_SPEED_LOW | MX6_PAD_CTL_DSE_80ohm | MX6_PAD_CTL_SRE_SLOW

static inline void init_backlight_gpios(int cpu_type, unsigned board_variant)
{
	void __iomem *iomuxbase = IOMEM(MX6_IOMUXC_BASE_ADDR);
	void __iomem *gpio6base = IOMEM(MX6_GPIO6_BASE_ADDR);
	void __iomem *gpio1base = IOMEM(MX6_GPIO1_BASE_ADDR);

	/*
	 * since revision B a backlight switch is present which can help to
	 * prevent any kind of flicker when switching on the board. Use it.
	 * GPIO6/23 controls the backlight. High switches off the backlight.
	 */
	switch (board_variant) {
	case 0 ... 8:
		break;
	default:
		imx6_gpio_direction_output(gpio6base, 23, 1);

		switch (cpu_type) {
		case IMX6_CPUTYPE_IMX6S:
		case IMX6_CPUTYPE_IMX6DL:
			writel(IOMUX_CONFIG_SION | 0x05, iomuxbase + 0x2D0);
			writel(BKLGT_PWR_PAD_CTRL, iomuxbase + 0x6B8);
			break;
		case IMX6_CPUTYPE_IMX6D:
		case IMX6_CPUTYPE_IMX6Q:
			writel(IOMUX_CONFIG_SION | 0x05, iomuxbase + 0x068);
			writel(BKLGT_PWR_PAD_CTRL, iomuxbase + 0x37C);
			break;
		}
	}

	/*
	 * switch brightness to the lowest available value. This is what we
	 * can do for revision A boards
	 * GPIO1/1 controls (via PWM) the brightness. A static low means
	 * a very dark backlight
	 */
	imx6_gpio_direction_output(gpio1base, 1, 0);

	switch (cpu_type) {
	case IMX6_CPUTYPE_IMX6S:
	case IMX6_CPUTYPE_IMX6DL:
		writel(IOMUX_CONFIG_SION | 0x05, iomuxbase + 0x210);
		writel(BKLGT_PWR_PAD_CTRL, iomuxbase + 0x5E0);
		break;
	case IMX6_CPUTYPE_IMX6D:
	case IMX6_CPUTYPE_IMX6Q:
		writel(IOMUX_CONFIG_SION | 0x05, iomuxbase + 0x224);
		writel(BKLGT_PWR_PAD_CTRL, iomuxbase + 0x5F4);
		break;
	}
}

#define LED_PAD_CTRL MX6_PAD_CTL_SPEED_LOW | MX6_PAD_CTL_DSE_240ohm | MX6_PAD_CTL_SRE_SLOW

static inline void setup_leds(int cpu_type)
{
	void __iomem *iomuxbase = IOMEM(MX6_IOMUXC_BASE_ADDR);
	void __iomem *gpiobase = IOMEM(MX6_GPIO1_BASE_ADDR);

	switch (cpu_type) {
	case IMX6_CPUTYPE_IMX6S:
	case IMX6_CPUTYPE_IMX6DL:
		writel(0x05, iomuxbase + 0x20C); /* LED1 (GPIO0) */
		writel(LED_PAD_CTRL, iomuxbase + 0x5DC);
		writel(0x05, iomuxbase + 0x224); /* LED2 (GPIO2) */
		writel(LED_PAD_CTRL, iomuxbase + 0x5F4);
		writel(0x05, iomuxbase + 0x22C); /* LED3 (GPIO4) */
		writel(LED_PAD_CTRL, iomuxbase + 0x5FC);
		break;
	case IMX6_CPUTYPE_IMX6D:
	case IMX6_CPUTYPE_IMX6Q:
		writel(0x05, iomuxbase + 0x220); /* LED1 (GPIO0) */
		writel(LED_PAD_CTRL, iomuxbase + 0x5f0);
		writel(0x05, iomuxbase + 0x234); /* LED2 (GPIO2) */
		writel(LED_PAD_CTRL, iomuxbase + 0x604);
		writel(0x05, iomuxbase + 0x238); /* LED3 (GPIO4) */
		writel(LED_PAD_CTRL, iomuxbase + 0x608);
		break;
	}

	/* Turn off all LEDS */
	imx6_gpio_direction_output(gpiobase, 1, 0);
	imx6_gpio_direction_output(gpiobase, 4, 0);
	imx6_gpio_direction_output(gpiobase, 16, 0);
}

static inline void setup_uart(int cpu_type)
{
	void __iomem *iomuxbase = IOMEM(MX6_IOMUXC_BASE_ADDR);

	/* UART TxD output is pin EIM/D26, e.g. UART is in DTE mode */
	switch (cpu_type) {
	case IMX6_CPUTYPE_IMX6S:
	case IMX6_CPUTYPE_IMX6DL:
		writel(0x0, iomuxbase + 0x904); /*  IOMUXC_UART2_UART_RX_DATA_SELECT_INPUT */
		writel(0x4, iomuxbase + 0x16c); /*  IOMUXC_SW_MUX_CTL_PAD_EIM_DATA26 */
		break;
	case IMX6_CPUTYPE_IMX6D:
	case IMX6_CPUTYPE_IMX6Q:
		writel(0x0, iomuxbase + 0x928); /*  IOMUXC_UART2_UART_RX_DATA_SELECT_INPUT */
		writel(0x4, iomuxbase + 0x0bc); /*  IOMUXC_SW_MUX_CTL_PAD_EIM_DATA26 */
		break;
	}

	imx6_ungate_all_peripherals();
	imx6_uart_setup(IOMEM(MX6_UART2_BASE_ADDR));
	pbl_set_putc(imx_uart_putc, IOMEM(MX6_UART2_BASE_ADDR));

	pr_debug("\n");
}

/*
 * Hardware marked board revisions and deployments
 *
 *   count      board    ram       flash       CPU
 *               rev.
 * 00000000       A    1024 MiB   1024 MiB     i.MX6Q
 * 00000001       A     512 MiB    256 MiB     i.MX6S
 * 00000010       A    1024 MiB    512 MiB     i.MX6Q
 * 00000011       ---- not defined ----
 * 00000100       A     512 MiB    256 MiB     i.MX6S
 * 00000101       ---- not defined ----
 * 00000110       ---- not defined ----
 * 00000111       ---- not defined ----
 * 00001000       A    1024 MiB    512 MiB     i.MX6Q
 * 00001001       B     256 MiB     16 MiB     i.MX6S
 * 00001010       B     256 MiB    256 MiB     i.MX6S
 * 00001011       B    1024 MiB    256 MiB     i.MX6Q
 * 00001100       B    2048 MiB      8 GiB     i.MX6Q
 * 00001101       C     256 MiB    256 MiB     i.MX6S
 * 00001110       C    1024 MiB    256 MiB     i.MX6Q
 * 00001111       C     512 MiB    256 MiB     i.MX6S
 * 00010000       C     512 MiB      4 GiB     i.MX6S
 * 00010001       C    2048 MiB      8 GiB     i.MX6Q
 * 00010010       C    4096 MiB     16 GiB     i.MX6Q+
 * 00010011       C     512 MiB      2 GiB     i.MX6S
 * 00010100       C    1024 MiB      4 GiB     i.MX6Q
 * 00010101       D     512 MiB      2 GIB     i.MX6S
 * 00010110       D    1024 MiB      4 GIB     i.MX6Q
 * 00010111       ---- not defined ----
 * 00011000       E    1024 MiB      4 GIB     i.MX6Q
 *
 * This routine does not return if starting the image from SD card or NOR
 * was successful. It restarts skov_imx6_start() instead
 */
static void skov_imx6_init(int cpu_type, unsigned board_variant)
{
	enum bootsource bootsrc;
	int instance;

	switch (board_variant) {
	case 12: /* P2 i.MX6Q, max performance */
		if (cpu_type != IMX6_CPUTYPE_IMX6Q) {
			pr_err("Invalid SoC! i.MX6Q expected\n");
			return;
		}
		pr_debug("Initializing a P2 max performance system...\n");
		spl_imx6q_dram_init(&skov_imx6_sysinfo_4x256Mb_1066MHz,
					&skov_imx6_calib_4x256Mb_1066MHz,
					&skov_imx6_cfg_4x256Mb_1066MHz);
		break;
	case 18: /* i.MX6Q+ */
		if (cpu_type != IMX6_CPUTYPE_IMX6Q) {
			pr_err("Invalid SoC! i.MX6Q expected\n");
			return;
		}
		pr_debug("Initializing board variant 18\n");
		spl_imx6q_dram_init(&skov_imx6_sysinfo_4x512Mb_1066MHz,
					&skov_imx6_calib_4x512Mb_1066MHz,
					&skov_imx6_cfg_4x512Mb_1066MHz);
		break;
	case 19: /* i.MX6S "Solo_R512M_F2G" */
		if (cpu_type != IMX6_CPUTYPE_IMX6S) {
			pr_err("Invalid SoC! i.MX6S expected\n");
			return;
		}
		pr_debug("Initializing board variant 19\n");
		spl_imx6sdl_dram_init(&skov_imx6_sysinfo_2x128Mb_800MHz,
					&skov_imx6_calib_2x128Mb_800MHz,
					&skov_imx6_cfg_2x128Mb_800MHz);
		break;
	case 20: /* i.MX6Q, "Quad_R1G_F2G" */
		if (cpu_type != IMX6_CPUTYPE_IMX6Q) {
			pr_err("Invalid SoC! i.MX6Q expected\n");
			return;
		}
		pr_debug("Initializing board variant 20\n");
		spl_imx6q_dram_init(&skov_imx6_sysinfo_4x128Mb_1066MHz,
					&skov_imx6_calib_4x128Mb_1066MHz,
					&skov_imx6_cfg_4x128Mb_1066MHz);
		break;
	case 21: /* i.MX6S "Solo_R512M_F2G" */
		if (cpu_type != IMX6_CPUTYPE_IMX6S) {
			pr_err("Invalid SoC! i.MX6S expected\n");
			return;
		}
		pr_debug("Initializing board variant 21\n");
		spl_imx6sdl_dram_init(&skov_imx6_sysinfo_2x128Mb_800MHz,
					&skov_imx6_calib_2x128Mb_800MHz,
					&skov_imx6_cfg_2x128Mb_800MHz);
		break;
	case 22: /* i.MX6Q, "Quad_R1G_F4G" */
		if (cpu_type != IMX6_CPUTYPE_IMX6Q) {
			pr_err("Invalid SoC! i.MX6Q expected\n");
			return;
		}
		pr_debug("Initializing board variant 22\n");
		spl_imx6q_dram_init(&skov_imx6_sysinfo_4x128Mb_1066MHz,
					&skov_imx6_calib_4x128Mb_1066MHz,
					&skov_imx6_cfg_4x128Mb_1066MHz);
		break;
	case 24: /* i.MX6Q, "Quad_R1G_F4G" */
		if (cpu_type != IMX6_CPUTYPE_IMX6Q) {
			pr_err("Invalid SoC! i.MX6Q expected\n");
			return;
		}
		pr_debug("Initializing board variant 24\n");
		spl_imx6q_dram_init(&skov_imx6_sysinfo_4x128Mb_1066MHz,
					&skov_imx6_calib_4x128Mb_1066MHz,
					&skov_imx6_cfg_4x128Mb_1066MHz);
		break;
	default:
		pr_err("Unsupported board variant: 0x%x\n", board_variant);
		/* don't continue */
		while(1);
		break;
	}

	imx6_get_boot_source(&bootsrc, &instance);
	if (bootsrc == BOOTSOURCE_SPI_NOR) {
		pr_info("Loading bootloader image from SPI flash...");
		imx6_spi_start_image(0);
	} else {
		pr_info("Loading bootloader image from SD card...");
		imx6_esdhc_start_image(instance);
	}
}

extern char __dtb_z_imx6q_skov_imx6_start[];
extern char __dtb_z_imx6dl_skov_imx6_start[];
extern char __dtb_z_imx6s_skov_imx6_start[];

/* called twice: once for SDRAM setup only, second for devicetree setup */
static noinline void skov_imx6_start(void)
{
	int cpu_type = __imx6_cpu_type();
	unsigned board_variant = skov_imx6_get_version();

	setup_uart(cpu_type);

	if (get_pc() <= MX6_MMDC_PORT01_BASE_ADDR) {
		/* first call: do the lowlevel things first */
		init_backlight_gpios(cpu_type, board_variant);
		setup_leds(cpu_type);
		pr_info("Starting to init IMX6 system...\n");
		skov_imx6_init(cpu_type, board_variant);
		pr_err("Unable to start bootloader\n");
		while (1);
	}

	/* boot this platform (second call) */
	switch (cpu_type) {
	case IMX6_CPUTYPE_IMX6S:
		pr_debug("Startup i.MX6S based system...\n");
		imx6q_barebox_entry(__dtb_z_imx6s_skov_imx6_start);
		break;
	case IMX6_CPUTYPE_IMX6DL:
		pr_debug("Startup i.MX6DL based system...\n");
		imx6q_barebox_entry(__dtb_z_imx6dl_skov_imx6_start);
		break;
	case IMX6_CPUTYPE_IMX6D:
	case IMX6_CPUTYPE_IMX6Q:
		pr_debug("Startup i.MX6Q based system...\n");
		imx6q_barebox_entry(__dtb_z_imx6q_skov_imx6_start);
		break;
	}
}

ENTRY_FUNCTION(start_imx6_skov_imx6, r0, r1, r2)
{
	imx6_cpu_lowlevel_init();

	relocate_to_current_adr();
	setup_c();
	barrier();

	skov_imx6_start();
}
