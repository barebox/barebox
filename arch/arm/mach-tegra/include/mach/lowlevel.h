/*
 * Copyright (C) 2013 Lucas Stach <l.stach@pengutronix.de>
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

#include <sizes.h>
#include <io.h>
#include <mach/iomap.h>

/* Bootinfotable */

#define NV_BIT_BCTSIZE		0x38	/* size of the BCT in IRAM */
#define NV_BIT_BCTPTR		0x3C	/* location of the BCT in IRAM */

/* ODM data */
#define BCT_ODMDATA_OFFSET	12	/* offset from the _end_ of the BCT */

#define T20_ODMDATA_RAMSIZE_SHIFT	28
#define T20_ODMDATA_RAMSIZE_MASK	(3 << T20_ODMDATA_RAMSIZE_SHIFT)
#define T20_ODMDATA_UARTTYPE_SHIFT	18
#define T20_ODMDATA_UARTTYPE_MASK	(3 << T20_ODMDATA_UARTTYPE_SHIFT)
#define T20_ODMDATA_UARTID_SHIFT	15
#define T20_ODMDATA_UARTID_MASK		(7 << T20_ODMDATA_UARTID_SHIFT)

static inline __attribute__((always_inline))
u32 tegra_get_odmdata(void)
{
	u32 bctsize, bctptr, odmdata;

	bctsize = cpu_readl(TEGRA_IRAM_BASE + NV_BIT_BCTSIZE);
	bctptr = cpu_readl(TEGRA_IRAM_BASE + NV_BIT_BCTPTR);

	odmdata = cpu_readl(bctptr + bctsize - BCT_ODMDATA_OFFSET);

	return odmdata;
}

/* chip ID */
#define APB_MISC_HIDREV			0x804
#define HIDREV_CHIPID_SHIFT		8
#define HIDREV_CHIPID_MASK		(0xff << HIDREV_CHIPID_SHIFT)

enum tegra_chiptype {
	TEGRA_UNK_REV = -1,
	TEGRA20 = 0,
};

static inline __attribute__((always_inline))
enum tegra_chiptype tegra_get_chiptype(void)
{
	u32 hidrev;

	hidrev = readl(TEGRA_APB_MISC_BASE + APB_MISC_HIDREV);

	switch ((hidrev & HIDREV_CHIPID_MASK) >> HIDREV_CHIPID_SHIFT) {
	case 0x20:
		return TEGRA20;
	default:
		return TEGRA_UNK_REV;
	}
}

static inline __attribute__((always_inline))
int tegra_get_num_cores(void)
{
	switch (tegra_get_chiptype()) {
	case TEGRA20:
		return 2;
		break;
	default:
		return 0;
		break;
	}
}

/* Runtime data */
static inline __attribute__((always_inline))
int tegra_cpu_is_maincomplex(void)
{
	u32 tag0;

	tag0 = readl(TEGRA_UP_TAG_BASE);

	return (tag0 & 0xff) == 0x55;
}

static inline __attribute__((always_inline))
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

static long uart_id_to_base[] = {
	TEGRA_UARTA_BASE,
	TEGRA_UARTB_BASE,
	TEGRA_UARTC_BASE,
	TEGRA_UARTD_BASE,
	TEGRA_UARTE_BASE,
};

static inline __attribute__((always_inline))
long tegra20_get_debuguart_base(void)
{
	u32 odmdata;
	int id;

	odmdata = tegra_get_odmdata();

	/*
	 * Get type, we accept both "2" and "3", as they both demark a UART,
	 * depending on the board type.
	 */
	if (!(((odmdata & T20_ODMDATA_UARTTYPE_MASK) >>
	      T20_ODMDATA_UARTTYPE_SHIFT) & 0x2))
		return 0;

	id = (odmdata & T20_ODMDATA_UARTID_MASK) >> T20_ODMDATA_UARTID_SHIFT;
	if (id > ARRAY_SIZE(uart_id_to_base))
		return 0;

	return uart_id_to_base[id];
}

#define CRC_OSC_CTRL			0x050
#define CRC_OSC_CTRL_OSC_FREQ_SHIFT	30
#define CRC_OSC_CTRL_OSC_FREQ_MASK	(0x3 << CRC_OSC_CTRL_OSC_FREQ_SHIFT)

static inline unsigned __attribute__((always_inline))
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

static inline __attribute__((always_inline))
void tegra_cpu_lowlevel_setup(void)
{
	uint32_t r;

	/* set the cpu to SVC32 mode */
	__asm__ __volatile__("mrs %0, cpsr":"=r"(r));
	r &= ~0x1f;
	r |= 0xd3;
	__asm__ __volatile__("msr cpsr, %0" : : "r"(r));
}

/* reset vector for the AVP, to be called from board reset vector */
void tegra_avp_reset_vector(uint32_t boarddata);

/* reset vector for the main CPU complex */
void tegra_maincomplex_entry(void);
