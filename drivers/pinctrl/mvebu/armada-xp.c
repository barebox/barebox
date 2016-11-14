/*
 * Marvell Armada XP pinctrl driver based on mvebu pinctrl core
 *
 * Copyright (C) 2012 Marvell
 *
 * Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This file supports the three variants of Armada XP SoCs that are
 * available: mv78230, mv78260 and mv78460. From a pin muxing
 * perspective, the mv78230 has 49 MPP pins. The mv78260 and mv78460
 * both have 67 MPP pins (more GPIOs and address lines for the memory
 * bus mainly). The only difference between the mv78260 and the
 * mv78460 in terms of pin muxing is the addition of two functions on
 * pins 43 and 57 to access the VDD of the CPU2 and 3 (mv78260 has two
 * cores, mv78460 has four cores).
 */

#include <common.h>
#include <init.h>
#include <linux/clk.h>
#include <malloc.h>
#include <of.h>
#include <of_address.h>
#include <linux/sizes.h>

#include "common.h"

static void __iomem *mpp_base;

static int armada_xp_mpp_ctrl_get(unsigned pid, unsigned long *config)
{
	return default_mpp_ctrl_get(mpp_base, pid, config);
}

static int armada_xp_mpp_ctrl_set(unsigned pid, unsigned long config)
{
	return default_mpp_ctrl_set(mpp_base, pid, config);
}

enum armada_xp_variant {
	V_MV78230	= BIT(0),
	V_MV78260	= BIT(1),
	V_MV78460	= BIT(2),
	V_MV78230_PLUS	= (V_MV78230 | V_MV78260 | V_MV78460),
	V_MV78260_PLUS	= (V_MV78260 | V_MV78460),
};

static struct mvebu_mpp_mode armada_xp_mpp_modes[] = {
	MPP_MODE(0, "mpp0", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "ge0", "txclkout",   V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "d0",         V_MV78230_PLUS)),
	MPP_MODE(1, "mpp1", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "ge0", "txd0",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "d1",         V_MV78230_PLUS)),
	MPP_MODE(2, "mpp2", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "ge0", "txd1",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "d2",         V_MV78230_PLUS)),
	MPP_MODE(3, "mpp3", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "ge0", "txd2",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "d3",         V_MV78230_PLUS)),
	MPP_MODE(4, "mpp4", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "ge0", "txd3",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "d4",         V_MV78230_PLUS)),
	MPP_MODE(5, "mpp5", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "ge0", "txctl",      V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "d5",         V_MV78230_PLUS)),
	MPP_MODE(6, "mpp6", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "ge0", "rxd0",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "d6",         V_MV78230_PLUS)),
	MPP_MODE(7, "mpp7", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "ge0", "rxd1",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "d7",         V_MV78230_PLUS)),
	MPP_MODE(8, "mpp8", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "ge0", "rxd2",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "d8",         V_MV78230_PLUS)),
	MPP_MODE(9, "mpp9", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "ge0", "rxd3",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "d9",         V_MV78230_PLUS)),
	MPP_MODE(10, "mpp10", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "ge0", "rxctl",      V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "d10",        V_MV78230_PLUS)),
	MPP_MODE(11, "mpp11", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "ge0", "rxclk",      V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "d11",        V_MV78230_PLUS)),
	MPP_MODE(12, "mpp12", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "ge0", "txd4",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x2, "ge1", "txclkout",   V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "d12",        V_MV78230_PLUS)),
	MPP_MODE(13, "mpp13", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "ge0", "txd5",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x2, "ge1", "txd0",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x3, "spi1", "mosi",      V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "d13",        V_MV78230_PLUS)),
	MPP_MODE(14, "mpp14", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "ge0", "txd6",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x2, "ge1", "txd1",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x3, "spi1", "sck",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "d14",        V_MV78230_PLUS)),
	MPP_MODE(15, "mpp15", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "ge0", "txd7",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x2, "ge1", "txd2",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "d15",        V_MV78230_PLUS)),
	MPP_MODE(16, "mpp16", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "ge0", "txclk",      V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x2, "ge1", "txd3",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x3, "spi1", "cs0",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "d16",        V_MV78230_PLUS)),
	MPP_MODE(17, "mpp17", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "ge0", "col",        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x2, "ge1", "txctl",      V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x3, "spi1", "miso",      V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "d17",        V_MV78230_PLUS)),
	MPP_MODE(18, "mpp18", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "ge0", "rxerr",      V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x2, "ge1", "rxd0",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x3, "ptp", "trig",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "d18",        V_MV78230_PLUS)),
	MPP_MODE(19, "mpp19", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "ge0", "crs",        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x2, "ge1", "rxd1",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x3, "ptp", "evreq",      V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "d19",        V_MV78230_PLUS)),
	MPP_MODE(20, "mpp20", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "ge0", "rxd4",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x2, "ge1", "rxd2",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x3, "ptp", "clk",        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "d20",        V_MV78230_PLUS)),
	MPP_MODE(21, "mpp21", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "ge0", "rxd5",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x2, "ge1", "rxd3",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x3, "dram", "bat",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "d21",        V_MV78230_PLUS)),
	MPP_MODE(22, "mpp22", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "ge0", "rxd6",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x2, "ge1", "rxctl",      V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x3, "sata0", "prsnt",    V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "d22",        V_MV78230_PLUS)),
	MPP_MODE(23, "mpp23", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "ge0", "rxd7",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x2, "ge1", "rxclk",      V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x3, "sata1", "prsnt",    V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "d23",        V_MV78230_PLUS)),
	MPP_MODE(24, "mpp24", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "sata1", "prsnt",    V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x3, "tdm", "rst",        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "hsync",      V_MV78230_PLUS)),
	MPP_MODE(25, "mpp25", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "sata0", "prsnt",    V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x3, "tdm", "pclk",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "vsync",      V_MV78230_PLUS)),
	MPP_MODE(26, "mpp26", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x3, "tdm", "fsync",      V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "clk",        V_MV78230_PLUS)),
	MPP_MODE(27, "mpp27", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "ptp", "trig",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x3, "tdm", "dtx",        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "e",          V_MV78230_PLUS)),
	MPP_MODE(28, "mpp28", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "ptp", "evreq",      V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x3, "tdm", "drx",        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "pwm",        V_MV78230_PLUS)),
	MPP_MODE(29, "mpp29", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "ptp", "clk",        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x3, "tdm", "int0",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "ref-clk",    V_MV78230_PLUS)),
	MPP_MODE(30, "mpp30", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "sd0", "clk",        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x3, "tdm", "int1",       V_MV78230_PLUS)),
	MPP_MODE(31, "mpp31", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "sd0", "cmd",        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x3, "tdm", "int2",       V_MV78230_PLUS)),
	MPP_MODE(32, "mpp32", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "sd0", "d0",         V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x3, "tdm", "int3",       V_MV78230_PLUS)),
	MPP_MODE(33, "mpp33", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "sd0", "d1",         V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x3, "tdm", "int4",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "dram", "bat",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x5, "dram", "vttctrl",   V_MV78230_PLUS)),
	MPP_MODE(34, "mpp34", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "sd0", "d2",         V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x2, "sata0", "prsnt",    V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x3, "tdm", "int5",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "dram", "deccerr",   V_MV78230_PLUS)),
	MPP_MODE(35, "mpp35", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "sd0", "d3",         V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x2, "sata1", "prsnt",    V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x3, "tdm", "int6",       V_MV78230_PLUS)),
	MPP_MODE(36, "mpp36", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "spi0", "mosi",      V_MV78230_PLUS)),
	MPP_MODE(37, "mpp37", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "spi0", "miso",      V_MV78230_PLUS)),
	MPP_MODE(38, "mpp38", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "spi0", "sck",       V_MV78230_PLUS)),
	MPP_MODE(39, "mpp39", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "spi0", "cs0",       V_MV78230_PLUS)),
	MPP_MODE(40, "mpp40", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "spi0", "cs1",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x2, "uart2", "cts",      V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "vga-hsync",  V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x5, "pcie", "clkreq0",   V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x6, "spi1", "cs1",       V_MV78230_PLUS)),
	MPP_MODE(41, "mpp41", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "spi0", "cs2",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x2, "uart2", "rts",      V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x3, "sata1", "prsnt",    V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "lcd", "vga-vsync",  V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x5, "pcie", "clkreq1",   V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x6, "spi1", "cs2",       V_MV78230_PLUS)),
	MPP_MODE(42, "mpp42", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "uart2", "rxd",      V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x2, "uart0", "cts",      V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x3, "tdm", "int7",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "tdm", "timer",      V_MV78230_PLUS)),
	MPP_MODE(43, "mpp43", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "uart2", "txd",      V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x2, "uart0", "rts",      V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x3, "spi0", "cs3",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "pcie", "rstout",    V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x6, "spi1", "cs3",       V_MV78230_PLUS)),
	MPP_MODE(44, "mpp44", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "uart2", "cts",      V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x2, "uart3", "rxd",      V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x3, "spi0", "cs4",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "dram", "bat",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x5, "pcie", "clkreq2",   V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x6, "spi1", "cs4",       V_MV78230_PLUS)),
	MPP_MODE(45, "mpp45", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "uart2", "rts",      V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x2, "uart3", "txd",      V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x3, "spi0", "cs5",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "sata1", "prsnt",    V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x5, "dram", "vttctrl",   V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x6, "spi1", "cs5",       V_MV78230_PLUS)),
	MPP_MODE(46, "mpp46", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "uart3", "rts",      V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x2, "uart1", "rts",      V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x3, "spi0", "cs6",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "sata0", "prsnt",    V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x6, "spi1", "cs6",       V_MV78230_PLUS)),
	MPP_MODE(47, "mpp47", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "uart3", "cts",      V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x2, "uart1", "cts",      V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x3, "spi0", "cs7",       V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x4, "ref", "clkout",     V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x5, "pcie", "clkreq3",   V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x6, "spi1", "cs7",       V_MV78230_PLUS)),
	MPP_MODE(48, "mpp48", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x1, "dev", "clkout",     V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x2, "dev", "burst/last", V_MV78230_PLUS),
	   MPP_VAR_FUNCTION(0x3, "nand", "rb",        V_MV78230_PLUS)),
	MPP_MODE(49, "mpp49", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78260_PLUS),
	   MPP_VAR_FUNCTION(0x1, "dev", "we3",        V_MV78260_PLUS)),
	MPP_MODE(50, "mpp50", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78260_PLUS),
	   MPP_VAR_FUNCTION(0x1, "dev", "we2",        V_MV78260_PLUS)),
	MPP_MODE(51, "mpp51", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78260_PLUS),
	   MPP_VAR_FUNCTION(0x1, "dev", "ad16",       V_MV78260_PLUS)),
	MPP_MODE(52, "mpp52", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78260_PLUS),
	   MPP_VAR_FUNCTION(0x1, "dev", "ad17",       V_MV78260_PLUS)),
	MPP_MODE(53, "mpp53", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78260_PLUS),
	   MPP_VAR_FUNCTION(0x1, "dev", "ad18",       V_MV78260_PLUS)),
	MPP_MODE(54, "mpp54", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78260_PLUS),
	   MPP_VAR_FUNCTION(0x1, "dev", "ad19",       V_MV78260_PLUS)),
	MPP_MODE(55, "mpp55", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78260_PLUS),
	   MPP_VAR_FUNCTION(0x1, "dev", "ad20",       V_MV78260_PLUS)),
	MPP_MODE(56, "mpp56", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78260_PLUS),
	   MPP_VAR_FUNCTION(0x1, "dev", "ad21",       V_MV78260_PLUS)),
	MPP_MODE(57, "mpp57", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78260_PLUS),
	   MPP_VAR_FUNCTION(0x1, "dev", "ad22",       V_MV78260_PLUS)),
	MPP_MODE(58, "mpp58", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78260_PLUS),
	   MPP_VAR_FUNCTION(0x1, "dev", "ad23",       V_MV78260_PLUS)),
	MPP_MODE(59, "mpp59", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78260_PLUS),
	   MPP_VAR_FUNCTION(0x1, "dev", "ad24",       V_MV78260_PLUS)),
	MPP_MODE(60, "mpp60", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78260_PLUS),
	   MPP_VAR_FUNCTION(0x1, "dev", "ad25",       V_MV78260_PLUS)),
	MPP_MODE(61, "mpp61", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78260_PLUS),
	   MPP_VAR_FUNCTION(0x1, "dev", "ad26",       V_MV78260_PLUS)),
	MPP_MODE(62, "mpp62", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78260_PLUS),
	   MPP_VAR_FUNCTION(0x1, "dev", "ad27",       V_MV78260_PLUS)),
	MPP_MODE(63, "mpp63", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78260_PLUS),
	   MPP_VAR_FUNCTION(0x1, "dev", "ad28",       V_MV78260_PLUS)),
	MPP_MODE(64, "mpp64", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78260_PLUS),
	   MPP_VAR_FUNCTION(0x1, "dev", "ad29",       V_MV78260_PLUS)),
	MPP_MODE(65, "mpp65", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78260_PLUS),
	   MPP_VAR_FUNCTION(0x1, "dev", "ad30",       V_MV78260_PLUS)),
	MPP_MODE(66, "mpp66", armada_xp_mpp_ctrl,
	   MPP_VAR_FUNCTION(0x0, "gpio", NULL,        V_MV78260_PLUS),
	   MPP_VAR_FUNCTION(0x1, "dev", "ad31",       V_MV78260_PLUS)),
};

static struct mvebu_pinctrl_soc_info armada_xp_pinctrl_info = {
	.modes = armada_xp_mpp_modes,
	.nmodes = ARRAY_SIZE(armada_xp_mpp_modes),
};

static struct of_device_id armada_xp_pinctrl_of_match[] = {
	{ .compatible = "marvell,mv78230-pinctrl", .data = (void *)V_MV78230, },
	{ .compatible = "marvell,mv78260-pinctrl", .data = (void *)V_MV78260, },
	{ .compatible = "marvell,mv78460-pinctrl", .data = (void *)V_MV78460, },
	{ },
};

static int armada_xp_pinctrl_probe(struct device_d *dev)
{
	struct resource *iores;
	const struct of_device_id *match =
		of_match_node(armada_xp_pinctrl_of_match, dev->device_node);
	struct mvebu_pinctrl_soc_info *soc = &armada_xp_pinctrl_info;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	mpp_base = IOMEM(iores->start);

	soc->variant = (enum armada_xp_variant)match->data;

	/*
	 * We don't necessarily want the full list of the armada_xp_mpp_modes,
	 * but only the first 'n' ones that are available on this SoC
	 */
	if (soc->variant == V_MV78230)
		soc->nmodes = 49;

	return mvebu_pinctrl_probe(dev, soc);
}

static struct driver_d armada_xp_pinctrl_driver = {
	.name		= "pinctrl-armada-xp",
	.probe		= armada_xp_pinctrl_probe,
	.of_compatible	= armada_xp_pinctrl_of_match,
};

static int armada_xp_pinctrl_init(void)
{
	return platform_driver_register(&armada_xp_pinctrl_driver);
}
postcore_initcall(armada_xp_pinctrl_init);
