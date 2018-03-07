/*
 * Copyright (C) 2013-2014 Lucas Stach <l.stach@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
 * @brief Boot informations provided by the Tegra SoC and it's BootROM. All
 * accessor functions are a header only implementations, as they are meant to
 * be used by both the main CPU complex (ARMv7) and the AVP (ARMv4).
 */

#ifndef __TEGRA_LOWLEVEL_H
#define __TEGRA_LOWLEVEL_H

#include <asm/barebox-arm.h>
#include <linux/compiler.h>
#include <linux/sizes.h>
#include <io.h>
#include <mach/iomap.h>

/* Bootinfotable */

/* location of the BCT in IRAM */
#define NV_BIT_BCTPTR_T20	0x3c
#define NV_BIT_BCTPTR_T114	0x4c

/* ODM data */
#define BCT_ODMDATA_OFFSET	12	/* offset from the _end_ of the BCT */

#define T20_ODMDATA_RAMSIZE_SHIFT	28
#define T20_ODMDATA_RAMSIZE_MASK	(3 << T20_ODMDATA_RAMSIZE_SHIFT)
#define T30_ODMDATA_RAMSIZE_MASK	(0xf << T20_ODMDATA_RAMSIZE_SHIFT)
#define T20_ODMDATA_UARTTYPE_SHIFT	18
#define T20_ODMDATA_UARTTYPE_MASK	(3 << T20_ODMDATA_UARTTYPE_SHIFT)
#define T20_ODMDATA_UARTID_SHIFT	15
#define T20_ODMDATA_UARTID_MASK		(7 << T20_ODMDATA_UARTID_SHIFT)

/* chip ID */
#define APB_MISC_HIDREV			0x804
#define HIDREV_CHIPID_SHIFT		8
#define HIDREV_CHIPID_MASK		(0xff << HIDREV_CHIPID_SHIFT)

enum tegra_chiptype {
	TEGRA_UNK_REV = -1,
	TEGRA20 = 0,
	TEGRA30 = 1,
	TEGRA114 = 2,
	TEGRA124 = 3,
};

static __always_inline
u32 tegra_read_chipid(void)
{
	return readl(TEGRA_APB_MISC_BASE + APB_MISC_HIDREV);
}

static __always_inline
enum tegra_chiptype tegra_get_chiptype(void)
{
	u32 hidrev;

	hidrev = readl(TEGRA_APB_MISC_BASE + APB_MISC_HIDREV);

	switch ((hidrev & HIDREV_CHIPID_MASK) >> HIDREV_CHIPID_SHIFT) {
	case 0x20:
		return TEGRA20;
	case 0x30:
		return TEGRA30;
	case 0x40:
		return TEGRA124;
	default:
		return TEGRA_UNK_REV;
	}
}

static __always_inline
u32 tegra_get_odmdata(void)
{
	u32 bctptr_offset, bctptr, odmdata_offset;
	enum tegra_chiptype chiptype = tegra_get_chiptype();

	switch(chiptype) {
	case TEGRA20:
		bctptr_offset = NV_BIT_BCTPTR_T20;
		odmdata_offset = 4068;
		break;
	case TEGRA30:
		bctptr_offset = NV_BIT_BCTPTR_T20;
		odmdata_offset = 6116;
		break;
	case TEGRA114:
		bctptr_offset = NV_BIT_BCTPTR_T114;
		odmdata_offset = 1752;
		break;
	case TEGRA124:
		bctptr_offset = NV_BIT_BCTPTR_T114;
		odmdata_offset = 1704;
		break;
	default:
		return 0;
	}

	bctptr = __raw_readl(TEGRA_IRAM_BASE + bctptr_offset);

	return __raw_readl(bctptr + odmdata_offset);
}

static __always_inline
int tegra_get_num_cores(void)
{
	switch (tegra_get_chiptype()) {
	case TEGRA20:
		return 2;
	case TEGRA30:
	case TEGRA124:
		return 4;
	default:
		return 0;
	}
}

/* Runtime data */
static __always_inline
int tegra_cpu_is_maincomplex(void)
{
	u32 tag0;

	tag0 = readl(TEGRA_UP_TAG_BASE);

	return (tag0 & 0xff) == 0x55;
}

static __always_inline
uint32_t tegra20_get_ramsize(void)
{
	switch ((tegra_get_odmdata() & T20_ODMDATA_RAMSIZE_MASK) >>
		T20_ODMDATA_RAMSIZE_SHIFT) {
	case 1:
		return SZ_256M;
	default:
	case 2:
		return SZ_512M;
	case 3:
		return SZ_1G;
	}
}

static __always_inline
uint32_t tegra30_get_ramsize(void)
{
	switch ((tegra_get_odmdata() & T30_ODMDATA_RAMSIZE_MASK) >>
			T20_ODMDATA_RAMSIZE_SHIFT) {
	case 0:
	case 1:
	default:
		return SZ_256M;
	case 2:
		return SZ_512M;
	case 3:
		return SZ_512M + SZ_256M;
	case 4:
		return SZ_1G;
	case 8:
		return SZ_2G - SZ_1M;
	}
}

#define CRC_OSC_CTRL			0x050
#define CRC_OSC_CTRL_OSC_FREQ_SHIFT	30
#define CRC_OSC_CTRL_OSC_FREQ_MASK	(0x3 << CRC_OSC_CTRL_OSC_FREQ_SHIFT)

static __always_inline
int tegra_get_osc_clock(void)
{
	u32 osc_ctrl = readl(TEGRA_CLK_RESET_BASE + CRC_OSC_CTRL);

	switch ((osc_ctrl & CRC_OSC_CTRL_OSC_FREQ_MASK) >>
		CRC_OSC_CTRL_OSC_FREQ_SHIFT) {
	case 0:
		return 13000000;
	case 1:
		return 19200000;
	case 2:
		return 12000000;
	case 3:
		return 26000000;
	default:
		return 0;
	}
}

#define TIMER_CNTR_1US	0x00
#define TIMER_USEC_CFG	0x04

static __always_inline
void tegra_ll_delay_setup(void)
{
	u32 reg;

	/*
	 * calibrate timer to run at 1MHz
	 * TIMERUS_USEC_CFG selects the scale down factor with bits [0:7]
	 * representing the divisor and bits [8:15] representing the dividend
	 * each in n+1 form.
	 */
	switch (tegra_get_osc_clock()) {
	case 12000000:
		reg = 0x000b;
		break;
	case 13000000:
		reg = 0x000c;
		break;
	case 19200000:
		reg = 0x045f;
		break;
	case 26000000:
		reg = 0x0019;
		break;
	default:
		reg = 0;
		break;
	}

	writel(reg, TEGRA_TMRUS_BASE + TIMER_USEC_CFG);
}

static __always_inline
void tegra_ll_delay_usec(int delay)
{
	int timeout = (int)readl(TEGRA_TMRUS_BASE + TIMER_CNTR_1US) + delay;

	while ((int)readl(TEGRA_TMRUS_BASE + TIMER_CNTR_1US) - timeout < 0);
}

/* reset vector for the AVP, to be called from board reset vector */
void tegra_avp_reset_vector(void);

/* reset vector for the main CPU complex */
void tegra_maincomplex_entry(char *fdt);

static __always_inline
void tegra_cpu_lowlevel_setup(char *fdt)
{
	uint32_t r;

	/* set the cpu to SVC32 mode */
	__asm__ __volatile__("mrs %0, cpsr":"=r"(r));
	r &= ~0x1f;
	r |= 0xd3;
	__asm__ __volatile__("msr cpsr, %0" : : "r"(r));

	arm_setup_stack(TEGRA_IRAM_BASE + SZ_256K - 8);

	if (tegra_cpu_is_maincomplex())
		tegra_maincomplex_entry(fdt + get_runtime_offset());

	tegra_ll_delay_setup();
}

#endif /* __TEGRA_LOWLEVEL_H */
