/*
 * Marvell Kirkwood pinctrl driver based on mvebu pinctrl core
 *
 * Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <common.h>
#include <init.h>
#include <malloc.h>
#include <of.h>
#include <linux/err.h>

#include "common.h"

static void __iomem *mpp_base;

static int kirkwood_mpp_ctrl_get(unsigned pid, unsigned long *config)
{
	return default_mpp_ctrl_get(mpp_base, pid, config);
}

static int kirkwood_mpp_ctrl_set(unsigned pid, unsigned long config)
{
	return default_mpp_ctrl_set(mpp_base, pid, config);
}

#define V(f6180, f6190, f6192, f6281, f6282, dx4122)	\
	((f6180 << 0) | (f6190 << 1) | (f6192 << 2) |	\
	 (f6281 << 3) | (f6282 << 4) | (dx4122 << 5))

enum kirkwood_variant {
	VARIANT_MV88F6180	= V(1, 0, 0, 0, 0, 0),
	VARIANT_MV88F6190	= V(0, 1, 0, 0, 0, 0),
	VARIANT_MV88F6192	= V(0, 0, 1, 0, 0, 0),
	VARIANT_MV88F6281	= V(0, 0, 0, 1, 0, 0),
	VARIANT_MV88F6282	= V(0, 0, 0, 0, 1, 0),
	VARIANT_MV98DX4122	= V(0, 0, 0, 0, 0, 1),
};

static struct mvebu_mpp_mode mv88f6xxx_mpp_modes[] = {
	MPP_MODE(0, "mpp0", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x1, "nand", "io2",     V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x2, "spi", "cs",       V(1, 1, 1, 1, 1, 1))),
	MPP_MODE(1, "mpp1", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpo", NULL,       V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x1, "nand", "io3",     V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x2, "spi", "mosi",     V(1, 1, 1, 1, 1, 1))),
	MPP_MODE(2, "mpp2", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpo", NULL,       V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x1, "nand", "io4",     V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x2, "spi", "sck",      V(1, 1, 1, 1, 1, 1))),
	MPP_MODE(3, "mpp3", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpo", NULL,       V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x1, "nand", "io5",     V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x2, "spi", "miso",     V(1, 1, 1, 1, 1, 1))),
	MPP_MODE(4, "mpp4", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x1, "nand", "io6",     V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x2, "uart0", "rxd",    V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x5, "sata1", "act",    V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "hsync",    V(0, 0, 0, 0, 1, 0)),
		MPP_VAR_FUNCTION(0xd, "ptp", "clk",      V(1, 1, 1, 1, 0, 0))),
	MPP_MODE(5, "mpp5", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpo", NULL,       V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x1, "nand", "io7",     V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x2, "uart0", "txd",    V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x4, "ptp", "trig",     V(1, 1, 1, 1, 0, 0)),
		MPP_VAR_FUNCTION(0x5, "sata0", "act",    V(0, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "vsync",    V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(6, "mpp6", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x1, "sysrst", "out",   V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x2, "spi", "mosi",     V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x3, "ptp", "trig",     V(1, 1, 1, 1, 0, 0))),
	MPP_MODE(7, "mpp7", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpo", NULL,       V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x1, "pex", "rsto",     V(1, 1, 1, 1, 0, 1)),
		MPP_VAR_FUNCTION(0x2, "spi", "cs",       V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x3, "ptp", "trig",     V(1, 1, 1, 1, 0, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "pwm",      V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(8, "mpp8", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x1, "twsi0", "sda",    V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x2, "uart0", "rts",    V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x3, "uart1", "rts",    V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x4, "mii-1", "rxerr",  V(0, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x5, "sata1", "prsnt",  V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xc, "ptp", "clk",      V(1, 1, 1, 1, 0, 0)),
		MPP_VAR_FUNCTION(0xd, "mii", "col",      V(1, 1, 1, 1, 1, 0))),
	MPP_MODE(9, "mpp9", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x1, "twsi0", "sck",    V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x2, "uart0", "cts",    V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x3, "uart1", "cts",    V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x5, "sata0", "prsnt",  V(0, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xc, "ptp", "evreq",    V(1, 1, 1, 1, 0, 0)),
		MPP_VAR_FUNCTION(0xd, "mii", "crs",      V(1, 1, 1, 1, 1, 0))),
	MPP_MODE(10, "mpp10", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpo", NULL,       V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x2, "spi", "sck",      V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0X3, "uart0", "txd",    V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x5, "sata1", "act",    V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xc, "ptp", "trig",     V(1, 1, 1, 1, 0, 0))),
	MPP_MODE(11, "mpp11", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x2, "spi", "miso",     V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x3, "uart0", "rxd",    V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x4, "ptp-1", "evreq",  V(1, 1, 1, 1, 0, 0)),
		MPP_VAR_FUNCTION(0xc, "ptp-2", "trig",   V(1, 1, 1, 1, 0, 0)),
		MPP_VAR_FUNCTION(0xd, "ptp", "clk",      V(1, 1, 1, 1, 0, 0)),
		MPP_VAR_FUNCTION(0x5, "sata0", "act",    V(0, 1, 1, 1, 1, 0))),
	MPP_MODE(12, "mpp12", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpo", NULL,       V(1, 1, 1, 0, 1, 0)),
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(0, 0, 0, 1, 0, 0)),
		MPP_VAR_FUNCTION(0x1, "sdio", "clk",     V(1, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xa, "audio", "spdifo", V(0, 0, 0, 0, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "spi", "mosi",     V(0, 0, 0, 0, 1, 0)),
		MPP_VAR_FUNCTION(0xd, "twsi1", "sda",    V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(13, "mpp13", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x1, "sdio", "cmd",     V(1, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x3, "uart1", "txd",    V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0xa, "audio", "rmclk",  V(0, 0, 0, 0, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "pwm",      V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(14, "mpp14", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x1, "sdio", "d0",      V(1, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x3, "uart1", "rxd",    V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x4, "sata1", "prsnt",  V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xa, "audio", "spdifi", V(0, 0, 0, 0, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "audio-1", "sdi",  V(0, 0, 0, 0, 1, 0)),
		MPP_VAR_FUNCTION(0xd, "mii", "col",      V(1, 1, 1, 1, 1, 0))),
	MPP_MODE(15, "mpp15", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x1, "sdio", "d1",      V(1, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x2, "uart0", "rts",    V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x3, "uart1", "txd",    V(1, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x4, "sata0", "act",    V(0, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "spi", "cs",       V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(16, "mpp16", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x1, "sdio", "d2",      V(1, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x2, "uart0", "cts",    V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x3, "uart1", "rxd",    V(1, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x4, "sata1", "act",    V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "extclk",   V(0, 0, 0, 0, 1, 0)),
		MPP_VAR_FUNCTION(0xd, "mii", "crs",      V(1, 1, 1, 1, 1, 0))),
	MPP_MODE(17, "mpp17", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(1, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x1, "sdio", "d3",      V(1, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x4, "sata0", "prsnt",  V(0, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xa, "sata1", "act",    V(0, 0, 0, 0, 1, 0)),
		MPP_VAR_FUNCTION(0xd, "twsi1", "sck",    V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(18, "mpp18", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpo", NULL,       V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x1, "nand", "io0",     V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x2, "pex", "clkreq",   V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(19, "mpp19", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpo", NULL,       V(1, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x1, "nand", "io1",     V(1, 1, 1, 1, 1, 1))),
	MPP_MODE(20, "mpp20", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(1, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x1, "ts", "mp0",       V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x2, "tdm", "tx0ql",    V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x3, "ge1", "txd0",     V(0, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x4, "audio", "spdifi", V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x5, "sata1", "act",    V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "d0",       V(0, 0, 0, 0, 1, 0)),
		MPP_VAR_FUNCTION(0xc, "mii", "rxerr",    V(1, 0, 0, 0, 0, 0))),
	MPP_MODE(21, "mpp21", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(1, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x1, "ts", "mp1",       V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x2, "tdm", "rx0ql",    V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x3, "ge1", "txd1",     V(0, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x4, "audio", "spdifi", V(1, 0, 0, 0, 0, 0)),
		MPP_VAR_FUNCTION(0x4, "audio", "spdifo", V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x5, "sata0", "act",    V(0, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "d1",       V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(22, "mpp22", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(1, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x1, "ts", "mp2",       V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x2, "tdm", "tx2ql",    V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x3, "ge1", "txd2",     V(0, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x4, "audio", "spdifo", V(1, 0, 0, 0, 0, 0)),
		MPP_VAR_FUNCTION(0x4, "audio", "rmclk",  V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x5, "sata1", "prsnt",  V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "d2",       V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(23, "mpp23", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(1, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x1, "ts", "mp3",       V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x2, "tdm", "rx2ql",    V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x3, "ge1", "txd3",     V(0, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x4, "audio", "rmclk",  V(1, 0, 0, 0, 0, 0)),
		MPP_VAR_FUNCTION(0x4, "audio", "bclk",   V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x5, "sata0", "prsnt",  V(0, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "d3",       V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(24, "mpp24", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(1, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x1, "ts", "mp4",       V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x2, "tdm", "spi-cs0",  V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x3, "ge1", "rxd0",     V(0, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x4, "audio", "bclk",   V(1, 0, 0, 0, 0, 0)),
		MPP_VAR_FUNCTION(0x4, "audio", "sdo",    V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "d4",       V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(25, "mpp25", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(1, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x1, "ts", "mp5",       V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x2, "tdm", "spi-sck",  V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x3, "ge1", "rxd1",     V(0, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x4, "audio", "sdo",    V(1, 0, 0, 0, 0, 0)),
		MPP_VAR_FUNCTION(0x4, "audio", "lrclk",  V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "d5",       V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(26, "mpp26", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(1, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x1, "ts", "mp6",       V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x2, "tdm", "spi-miso", V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x3, "ge1", "rxd2",     V(0, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x4, "audio", "lrclk",  V(1, 0, 0, 0, 0, 0)),
		MPP_VAR_FUNCTION(0x4, "audio", "mclk",   V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "d6",       V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(27, "mpp27", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(1, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x1, "ts", "mp7",       V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x2, "tdm", "spi-mosi", V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x3, "ge1", "rxd3",     V(0, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x4, "audio", "mclk",   V(1, 0, 0, 0, 0, 0)),
		MPP_VAR_FUNCTION(0x4, "audio", "sdi",    V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "d7",       V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(28, "mpp28", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(1, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x1, "ts", "mp8",       V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x2, "tdm", "int",      V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x3, "ge1", "col",      V(0, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x4, "audio", "sdi",    V(1, 0, 0, 0, 0, 0)),
		MPP_VAR_FUNCTION(0x4, "audio", "extclk", V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "d8",       V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(29, "mpp29", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(1, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x1, "ts", "mp9",       V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x2, "tdm", "rst",      V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x3, "ge1", "txclk",    V(0, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x4, "audio", "extclk", V(1, 0, 0, 0, 0, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "d9",       V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(30, "mpp30", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(0, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x1, "ts", "mp10",      V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x2, "tdm", "pclk",     V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x3, "ge1", "rxctl",    V(0, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "d10",      V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(31, "mpp31", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(0, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x1, "ts", "mp11",      V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x2, "tdm", "fs",       V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x3, "ge1", "rxclk",    V(0, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "d11",      V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(32, "mpp32", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(0, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x1, "ts", "mp12",      V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x2, "tdm", "drx",      V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x3, "ge1", "txclko",   V(0, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "d12",      V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(33, "mpp33", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpo", NULL,       V(0, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x2, "tdm", "dtx",      V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x3, "ge1", "txctl",    V(0, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "d13",      V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(34, "mpp34", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(0, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x2, "tdm", "spi-cs1",  V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x3, "ge1", "txen",     V(0, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x5, "sata1", "act",    V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "d14",      V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(35, "mpp35", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(0, 1, 1, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x2, "tdm", "tx0ql",    V(0, 0, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x3, "ge1", "rxerr",    V(0, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x5, "sata0", "act",    V(0, 1, 1, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "d15",      V(0, 0, 0, 0, 1, 0)),
		MPP_VAR_FUNCTION(0xc, "mii", "rxerr",    V(0, 1, 1, 1, 1, 0))),
	MPP_MODE(36, "mpp36", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(0, 0, 0, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x1, "ts", "mp0",       V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x2, "tdm", "spi-cs1",  V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x4, "audio", "spdifi", V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "twsi1", "sda",    V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(37, "mpp37", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(0, 0, 0, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x1, "ts", "mp1",       V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x2, "tdm", "tx2ql",    V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x4, "audio", "spdifo", V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "twsi1", "sck",    V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(38, "mpp38", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(0, 0, 0, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x1, "ts", "mp2",       V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x2, "tdm", "rx2ql",    V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x4, "audio", "rmclk",  V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "d18",      V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(39, "mpp39", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(0, 0, 0, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x1, "ts", "mp3",       V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x2, "tdm", "spi-cs0",  V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x4, "audio", "bclk",   V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "d19",      V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(40, "mpp40", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(0, 0, 0, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x1, "ts", "mp4",       V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x2, "tdm", "spi-sck",  V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x4, "audio", "sdo",    V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "d20",      V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(41, "mpp41", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(0, 0, 0, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x1, "ts", "mp5",       V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x2, "tdm", "spi-miso", V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x4, "audio", "lrclk",  V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "d21",      V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(42, "mpp42", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(0, 0, 0, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x1, "ts", "mp6",       V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x2, "tdm", "spi-mosi", V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x4, "audio", "mclk",   V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "d22",      V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(43, "mpp43", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(0, 0, 0, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x1, "ts", "mp7",       V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x2, "tdm", "int",      V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x4, "audio", "sdi",    V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "d23",      V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(44, "mpp44", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(0, 0, 0, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x1, "ts", "mp8",       V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x2, "tdm", "rst",      V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x4, "audio", "extclk", V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "clk",      V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(45, "mpp45", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(0, 0, 0, 1, 1, 1)),
		MPP_VAR_FUNCTION(0x1, "ts", "mp9",       V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x2, "tdm", "pclk",     V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "e",        V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(46, "mpp46", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x1, "ts", "mp10",      V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x2, "tdm", "fs",       V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "hsync",    V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(47, "mpp47", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x1, "ts", "mp11",      V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x2, "tdm", "drx",      V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "vsync",    V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(48, "mpp48", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x1, "ts", "mp12",      V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x2, "tdm", "dtx",      V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "d16",      V(0, 0, 0, 0, 1, 0))),
	MPP_MODE(49, "mpp49", kirkwood_mpp_ctrl,
		MPP_VAR_FUNCTION(0x0, "gpio", NULL,      V(0, 0, 0, 1, 0, 1)),
		MPP_VAR_FUNCTION(0x0, "gpo", NULL,       V(0, 0, 0, 0, 1, 0)),
		MPP_VAR_FUNCTION(0x1, "ts", "mp9",       V(0, 0, 0, 1, 0, 0)),
		MPP_VAR_FUNCTION(0x2, "tdm", "rx0ql",    V(0, 0, 0, 1, 1, 0)),
		MPP_VAR_FUNCTION(0x5, "ptp", "clk",      V(0, 0, 0, 1, 0, 0)),
		MPP_VAR_FUNCTION(0xa, "pex", "clkreq",   V(0, 0, 0, 0, 1, 0)),
		MPP_VAR_FUNCTION(0xb, "lcd", "d17",      V(0, 0, 0, 0, 1, 0))),
};

static struct mvebu_pinctrl_soc_info mv88f6180_info = {
	.variant = VARIANT_MV88F6180,
	.modes = mv88f6xxx_mpp_modes,
	.nmodes = ARRAY_SIZE(mv88f6xxx_mpp_modes),
};

static struct mvebu_pinctrl_soc_info mv88f6190_info = {
	.variant = VARIANT_MV88F6190,
	.modes = mv88f6xxx_mpp_modes,
	.nmodes = ARRAY_SIZE(mv88f6xxx_mpp_modes),
};

static struct mvebu_pinctrl_soc_info mv88f6192_info = {
	.variant = VARIANT_MV88F6192,
	.modes = mv88f6xxx_mpp_modes,
	.nmodes = ARRAY_SIZE(mv88f6xxx_mpp_modes),
};

static struct mvebu_pinctrl_soc_info mv88f6281_info = {
	.variant = VARIANT_MV88F6281,
	.modes = mv88f6xxx_mpp_modes,
	.nmodes = ARRAY_SIZE(mv88f6xxx_mpp_modes),
};

static struct mvebu_pinctrl_soc_info mv88f6282_info = {
	.variant = VARIANT_MV88F6282,
	.modes = mv88f6xxx_mpp_modes,
	.nmodes = ARRAY_SIZE(mv88f6xxx_mpp_modes),
};

static struct mvebu_pinctrl_soc_info mv98dx4122_info = {
	.variant = VARIANT_MV98DX4122,
	.modes = mv88f6xxx_mpp_modes,
	.nmodes = ARRAY_SIZE(mv88f6xxx_mpp_modes),
};

static struct of_device_id kirkwood_pinctrl_of_match[] = {
	{
		.compatible = "marvell,88f6180-pinctrl",
		.data = &mv88f6180_info
	},
	{
		.compatible = "marvell,88f6190-pinctrl",
		.data = &mv88f6190_info
	},
	{
		.compatible = "marvell,88f6192-pinctrl",
		.data = &mv88f6192_info
	},
	{
		.compatible = "marvell,88f6281-pinctrl",
		.data = &mv88f6281_info
	},
	{
		.compatible = "marvell,88f6282-pinctrl",
		.data = &mv88f6282_info
	},
	{
		.compatible = "marvell,98dx4122-pinctrl",
		.data = &mv98dx4122_info
	},
	{ }
};

static int kirkwood_pinctrl_probe(struct device_d *dev)
{
	struct resource *iores;
	const struct of_device_id *match =
		of_match_node(kirkwood_pinctrl_of_match, dev->device_node);
	struct mvebu_pinctrl_soc_info *soc =
		(struct mvebu_pinctrl_soc_info *)match->data;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	mpp_base = IOMEM(iores->start);

	return mvebu_pinctrl_probe(dev, soc);
}

static struct driver_d kirkwood_pinctrl_driver = {
	.name		= "pinctrl-kirkwood",
	.probe		= kirkwood_pinctrl_probe,
	.of_compatible	= kirkwood_pinctrl_of_match,
};

static int kirkwood_pinctrl_init(void)
{
	return platform_driver_register(&kirkwood_pinctrl_driver);
}
core_initcall(kirkwood_pinctrl_init);
