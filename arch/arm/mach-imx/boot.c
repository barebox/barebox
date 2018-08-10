/*
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

#include <common.h>
#include <bootsource.h>
#include <environment.h>
#include <init.h>
#include <linux/bitfield.h>
#include <magicvar.h>

#include <io.h>
#include <mach/generic.h>
#include <mach/imx25-regs.h>
#include <mach/imx27-regs.h>
#include <mach/imx35-regs.h>
#include <mach/imx51-regs.h>
#include <mach/imx53-regs.h>
#include <mach/imx6-regs.h>
#include <mach/imx7-regs.h>
#include <mach/vf610-regs.h>
#include <mach/imx8mq.h>


static void
imx_boot_save_loc(void (*get_boot_source)(enum bootsource *, int *))
{
	enum bootsource src = BOOTSOURCE_UNKNOWN;
	int instance = BOOTSOURCE_INSTANCE_UNKNOWN;

	get_boot_source(&src, &instance);

	bootsource_set(src);
	bootsource_set_instance(instance);
}


/* [CTRL][TYPE] */
static const enum bootsource locations[4][4] = {
	{ /* CTRL = WEIM */
		BOOTSOURCE_NOR,
		BOOTSOURCE_UNKNOWN,
		BOOTSOURCE_ONENAND,
		BOOTSOURCE_UNKNOWN,
	}, { /* CTRL == NAND */
		BOOTSOURCE_NAND,
		BOOTSOURCE_NAND,
		BOOTSOURCE_NAND,
		BOOTSOURCE_NAND,
	}, { /* CTRL == ATA, (imx35 only) */
		BOOTSOURCE_UNKNOWN,
		BOOTSOURCE_UNKNOWN, /* might be p-ata */
		BOOTSOURCE_UNKNOWN,
		BOOTSOURCE_UNKNOWN,
	}, { /* CTRL == expansion */
		BOOTSOURCE_MMC, /* note imx25 could also be: movinand, ce-ata */
		BOOTSOURCE_UNKNOWN,
		BOOTSOURCE_I2C,
		BOOTSOURCE_SPI,
	}
};

/*
 * Saves the boot source media into the $bootsource environment variable
 *
 * This information is useful for barebox init scripts as we can then easily
 * use a kernel image stored on the same media that we launch barebox with
 * (for example).
 *
 * imx25 and imx35 can boot into barebox from several media such as
 * nand, nor, mmc/sd cards, serial roms. "mmc" is used to represent several
 * sources as its impossible to distinguish between them.
 *
 * Some sources such as serial roms can themselves have 3 different boot
 * possibilities (i2c1, i2c2 etc). It is assumed that any board will
 * only be using one of these at any one time.
 *
 * Note also that I suspect that the boot source pins are only sampled at
 * power up.
 */
static enum bootsource imx25_35_boot_source(unsigned int ctrl, unsigned int type)
{
	enum bootsource src;

	src = locations[ctrl][type];

	return src;
}

void imx25_get_boot_source(enum bootsource *src, int *instance)
{
	void __iomem *ccm_base = IOMEM(MX25_CCM_BASE_ADDR);
	uint32_t val;

	val = readl(ccm_base + MX25_CCM_RCSR);
	*src = imx25_35_boot_source((val >> MX25_CCM_RCSR_MEM_CTRL_SHIFT) & 0x3,
				    (val >> MX25_CCM_RCSR_MEM_TYPE_SHIFT) & 0x3);
}

void imx25_boot_save_loc(void)
{
	imx_boot_save_loc(imx25_get_boot_source);
}

void imx35_get_boot_source(enum bootsource *src, int *instance)
{
	void __iomem *ccm_base = IOMEM(MX35_CCM_BASE_ADDR);
	uint32_t val;

	val = readl(ccm_base + MX35_CCM_RCSR);
	*src = imx25_35_boot_source((val >> MX35_CCM_RCSR_MEM_CTRL_SHIFT) & 0x3,
				    (val >> MX35_CCM_RCSR_MEM_TYPE_SHIFT) & 0x3);
}

void imx35_boot_save_loc(void)
{
	imx_boot_save_loc(imx35_get_boot_source);
}

#define IMX27_SYSCTRL_GPCR	0x18
#define IMX27_GPCR_BOOT_SHIFT			16
#define IMX27_GPCR_BOOT_MASK			(0xf << IMX27_GPCR_BOOT_SHIFT)
#define IMX27_GPCR_BOOT_UART_USB		0
#define IMX27_GPCR_BOOT_8BIT_NAND_2k		2
#define IMX27_GPCR_BOOT_16BIT_NAND_2k		3
#define IMX27_GPCR_BOOT_16BIT_NAND_512		4
#define IMX27_GPCR_BOOT_16BIT_CS0		5
#define IMX27_GPCR_BOOT_32BIT_CS0		6
#define IMX27_GPCR_BOOT_8BIT_NAND_512		7

void imx27_get_boot_source(enum bootsource *src, int *instance)
{
	void __iomem *sysctrl_base = IOMEM(MX27_SYSCTRL_BASE_ADDR);
	uint32_t val;

	val = readl(sysctrl_base + IMX27_SYSCTRL_GPCR);
	val &= IMX27_GPCR_BOOT_MASK;
	val >>= IMX27_GPCR_BOOT_SHIFT;

	switch (val) {
	case IMX27_GPCR_BOOT_UART_USB:
		*src = BOOTSOURCE_SERIAL;
		break;
	case IMX27_GPCR_BOOT_8BIT_NAND_2k:
	case IMX27_GPCR_BOOT_16BIT_NAND_2k:
	case IMX27_GPCR_BOOT_16BIT_NAND_512:
	case IMX27_GPCR_BOOT_8BIT_NAND_512:
		*src = BOOTSOURCE_NAND;
		break;
	default:
		*src = BOOTSOURCE_NOR;
		break;
	}
}

void imx27_boot_save_loc(void)
{
	imx_boot_save_loc(imx27_get_boot_source);
}

#define IMX51_SRC_SBMR			0x4
#define IMX51_SBMR_BT_MEM_TYPE_SHIFT	7
#define IMX51_SBMR_BT_MEM_CTL_SHIFT	0
#define IMX51_SBMR_BMOD_SHIFT		14

void imx51_get_boot_source(enum bootsource *src, int *instance)
{
	void __iomem *src_base = IOMEM(MX51_SRC_BASE_ADDR);
	uint32_t reg;
	unsigned int ctrl, type;

	reg = readl(src_base + IMX51_SRC_SBMR);

	switch ((reg >> IMX51_SBMR_BMOD_SHIFT) & 0x3) {
	case 0:
	case 2:
		/* internal boot */
		ctrl = (reg >> IMX51_SBMR_BT_MEM_CTL_SHIFT) & 0x3;
		type = (reg >> IMX51_SBMR_BT_MEM_TYPE_SHIFT) & 0x3;

		*src = locations[ctrl][type];
		break;
	case 1:
		/* reserved */
		*src = BOOTSOURCE_UNKNOWN;
		break;
	case 3:
		*src = BOOTSOURCE_SERIAL;
		break;

	}
}

void imx51_boot_save_loc(void)
{
	imx_boot_save_loc(imx51_get_boot_source);
}

#define IMX53_SRC_SBMR	0x4
#define SRC_SBMR_BMOD	GENMASK(25, 24)
#define IMX53_BMOD_SERIAL	0b11

#define __BOOT_CFG(n, m, l)	GENMASK((m) + ((n) - 1) * 8, \
					(l) + ((n) - 1) * 8)
#define BOOT_CFG1(m, l)		__BOOT_CFG(1, m, l)
#define BOOT_CFG2(m, l)		__BOOT_CFG(2, m, l)
#define BOOT_CFG3(m, l)		__BOOT_CFG(3, m, l)
#define BOOT_CFG4(m, l)		__BOOT_CFG(4, m, l)

#define ___BOOT_CFG(n, i)	__BOOT_CFG(n, i, i)
#define __MAKE_BOOT_CFG_BITS(idx)					\
	enum {								\
		BOOT_CFG##idx##_0 = ___BOOT_CFG(idx, 0),		\
		BOOT_CFG##idx##_1 = ___BOOT_CFG(idx, 1),		\
		BOOT_CFG##idx##_2 = ___BOOT_CFG(idx, 2),		\
		BOOT_CFG##idx##_3 = ___BOOT_CFG(idx, 3),		\
		BOOT_CFG##idx##_4 = ___BOOT_CFG(idx, 4),		\
		BOOT_CFG##idx##_5 = ___BOOT_CFG(idx, 5),		\
		BOOT_CFG##idx##_6 = ___BOOT_CFG(idx, 6),		\
		BOOT_CFG##idx##_7 = ___BOOT_CFG(idx, 7),		\
	};

__MAKE_BOOT_CFG_BITS(1)
__MAKE_BOOT_CFG_BITS(2)
__MAKE_BOOT_CFG_BITS(4)
#undef __MAKE_BOOT_CFG
#undef ___BOOT_CFG


static unsigned int imx53_get_bmod(uint32_t r)
{
	return FIELD_GET(SRC_SBMR_BMOD, r);
}

static int imx53_bootsource_internal(uint32_t r)
{
	return FIELD_GET(BOOT_CFG1(7, 4), r);
}

static int imx53_port_select(uint32_t r)
{
	return FIELD_GET(BOOT_CFG3(5, 4), r);
}

static bool imx53_bootsource_nand(uint32_t r)
{
	return FIELD_GET(BOOT_CFG1_7, r);
}

static enum bootsource imx53_bootsource_serial_rom(uint32_t r)
{
	return BOOT_CFG1(r, 3) ? BOOTSOURCE_SPI : BOOTSOURCE_I2C;
}

void imx53_get_boot_source(enum bootsource *src, int *instance)
{
	void __iomem *src_base = IOMEM(MX53_SRC_BASE_ADDR);
	uint32_t cfg1 = readl(src_base + IMX53_SRC_SBMR);

	if (imx53_get_bmod(cfg1) == IMX53_BMOD_SERIAL) {
		*src = BOOTSOURCE_USB;
		*instance = 0;
		return;
	}

	switch (imx53_bootsource_internal(cfg1)) {
	case 2:
		*src = BOOTSOURCE_HD;
		break;
	case 3:
		*src = imx53_bootsource_serial_rom(cfg1);
		break;
	case 4:
	case 5:
	case 6:
	case 7:
		*src = BOOTSOURCE_MMC;
		break;
	default:
		if (imx53_bootsource_nand(cfg1))
			*src = BOOTSOURCE_NAND;
		break;
	}

	switch (*src) {
	case BOOTSOURCE_MMC:
	case BOOTSOURCE_SPI:
	case BOOTSOURCE_I2C:
		*instance = imx53_port_select(cfg1);
		break;
	default:
		*instance = 0;
		break;
	}
}

void imx53_boot_save_loc(void)
{
	enum bootsource src = BOOTSOURCE_UNKNOWN;
	int instance = BOOTSOURCE_INSTANCE_UNKNOWN;

	imx53_get_boot_source(&src, &instance);

	bootsource_set(src);
	bootsource_set_instance(instance);
}

#define IMX6_SRC_SBMR1	0x04
#define IMX6_SRC_SBMR2	0x1c
#define IMX6_BMOD_SERIAL	0b01
#define IMX6_BMOD_RESERVED	0b11
#define IMX6_BMOD_FUSES		0b00
#define BT_FUSE_SEL		BIT(4)

static bool imx6_bootsource_reserved(uint32_t sbmr2)
{
	return imx53_get_bmod(sbmr2) == IMX6_BMOD_RESERVED;
}

static bool imx6_bootsource_serial(uint32_t sbmr2)
{
	return imx53_get_bmod(sbmr2) == IMX6_BMOD_SERIAL ||
		/*
		 * If boot from fuses is selected and fuses are not
		 * programmed by setting BT_FUSE_SEL, ROM code will
		 * fallback to serial mode
		 */
	       (imx53_get_bmod(sbmr2) == IMX6_BMOD_FUSES &&
		!(sbmr2 & BT_FUSE_SEL));
}

static int __imx6_bootsource_serial_rom(uint32_t r)
{
	return FIELD_GET(BOOT_CFG4(2, 0), r);
}

/*
 * Serial ROM bootsource on i.MX6 are as follows:
 *
 *	000 - ECSPI-1
 *	001 - ECSPI-2
 *	010 - ECSPI-3
 *	011 - ECSPI-4
 *	100 - ECSPI-5
 *	101 - I2C1
 *	110 - I2C2
 *	111 - I2C3
 *
 * There's no single bit that would tell us we are booting from I2C or
 * SPI, so we just have to compare the "source" agains the value for
 * I2C1 for both: calculating bootsource and boot instance.
 */
#define IMX6_BOOTSOURCE_SERIAL_ROM_I2C1	0b101

static enum bootsource imx6_bootsource_serial_rom(uint32_t sbmr)
{
	const int source = __imx6_bootsource_serial_rom(sbmr);

	return source < IMX6_BOOTSOURCE_SERIAL_ROM_I2C1 ?
		BOOTSOURCE_SPI_NOR : BOOTSOURCE_I2C;
}

static int imx6_boot_instance_serial_rom(uint32_t sbmr)
{
	const int source = __imx6_bootsource_serial_rom(sbmr);

	if (source < IMX6_BOOTSOURCE_SERIAL_ROM_I2C1)
		return source;

	return source - IMX6_BOOTSOURCE_SERIAL_ROM_I2C1;
}

static int imx6_boot_instance_mmc(uint32_t r)
{
	return FIELD_GET(BOOT_CFG2(4, 3), r);
}

void imx6_get_boot_source(enum bootsource *src, int *instance)
{
	void __iomem *src_base = IOMEM(MX6_SRC_BASE_ADDR);
	uint32_t sbmr1 = readl(src_base + IMX6_SRC_SBMR1);
	uint32_t sbmr2 = readl(src_base + IMX6_SRC_SBMR2);

	if (imx6_bootsource_reserved(sbmr2))
		return;

	if (imx6_bootsource_serial(sbmr2)) {
		*src = BOOTSOURCE_SERIAL;
		return;
	}

	switch (imx53_bootsource_internal(sbmr1)) {
	case 2:
		*src = BOOTSOURCE_HD;
		break;
	case 3:
		*src = imx6_bootsource_serial_rom(sbmr1);
		*instance = imx6_boot_instance_serial_rom(sbmr1);
		break;
	case 4:
	case 5:
	case 6:
	case 7:
		*src = BOOTSOURCE_MMC;
		*instance = imx6_boot_instance_mmc(sbmr1);
		break;
	default:
		if (imx53_bootsource_nand(sbmr1))
			*src = BOOTSOURCE_NAND;
		break;
	}
}

void imx6_boot_save_loc(void)
{
	imx_boot_save_loc(imx6_get_boot_source);
}

#define IMX7_BOOT_SW_INFO_POINTER_ADDR		0x000001E8
#define IMX8M_BOOT_SW_INFO_POINTER_ADDR_A0	0x000009e8
#define IMX8M_BOOT_SW_INFO_POINTER_ADDR_B0	0x00000968

#define IMX_BOOT_SW_INFO_BDT_SD		0x1

struct imx_boot_sw_info {
	uint8_t  reserved_1;
	uint8_t  boot_device_instance;
	uint8_t  boot_device_type;
	uint8_t  reserved_2;
	uint32_t frequency_hz[4]; /* Various frequencies (ARM, AXI,
				   * DDR, etc.). Not used */
	uint32_t reserved_3[3];
} __packed;

static void __imx7_get_boot_source(enum bootsource *src, int *instance,
				   unsigned long boot_sw_info_pointer_addr)
{
	const struct imx_boot_sw_info *info;

	info = (const void *)(unsigned long)
		readl(boot_sw_info_pointer_addr);

	switch (info->boot_device_type) {
	case 1:
	case 2:
		*src = BOOTSOURCE_MMC;
		*instance = info->boot_device_instance;
		break;
	case 3:
		*src = BOOTSOURCE_NAND;
		break;
	case 6:
		*src = BOOTSOURCE_SPI_NOR;
		*instance = info->boot_device_instance;
		break;
	case 4:
		*src = BOOTSOURCE_SPI; /* Really: qspi */
		break;
	case 5:
		*src = BOOTSOURCE_NOR;
		break;
	default:
		break;
	}
}

void imx7_get_boot_source(enum bootsource *src, int *instance)
{
	__imx7_get_boot_source(src, instance, IMX7_BOOT_SW_INFO_POINTER_ADDR);
}

void imx7_boot_save_loc(void)
{
	imx_boot_save_loc(imx7_get_boot_source);
}

static int vf610_boot_instance_spi(uint32_t r)
{
	return FIELD_GET(BOOT_CFG1_1, r);
}

static int vf610_boot_instance_nor(uint32_t r)
{
	return FIELD_GET(BOOT_CFG1_3, r);
}

/*
 * Vybrid's Serial ROM boot sources (BOOT_CFG4[2:0]) are as follows:
 *
 *	000 - SPI0
 *	001 - SPI1
 *	010 - SPI2
 *	011 - SPI3
 *	100 - I2C0
 *	101 - I2C1
 *	110 - I2C2
 *	111 - I2C3
 *
 * Which we can neatly divide in two halves and use MSb to detect if
 * bootsource is I2C or SPI EEPROM and 2 LSbs directly as boot
 * insance.
 */
static enum bootsource vf610_bootsource_serial_rom(uint32_t r)
{
	return FIELD_GET(BOOT_CFG4_2, r) ? BOOTSOURCE_I2C : BOOTSOURCE_SPI_NOR;
}

static int vf610_boot_instance_serial_rom(uint32_t r)
{
	return __imx6_bootsource_serial_rom(r) & 0b11;
}

static int vf610_boot_instance_can(uint32_t r)
{
	return FIELD_GET(BOOT_CFG1_0, r);
}

static int vf610_boot_instance_mmc(uint32_t r)
{
	return FIELD_GET(BOOT_CFG2_3, r);
}

void vf610_get_boot_source(enum bootsource *src, int *instance)
{
	void __iomem *src_base = IOMEM(VF610_SRC_BASE_ADDR);
	uint32_t sbmr1 = readl(src_base + IMX6_SRC_SBMR1);
	uint32_t sbmr2 = readl(src_base + IMX6_SRC_SBMR2);

	if (imx6_bootsource_reserved(sbmr2))
		return;

	if (imx6_bootsource_serial(sbmr2)) {
		*src = BOOTSOURCE_SERIAL;
		return;
	}

	switch (imx53_bootsource_internal(sbmr1)) {
	case 0:
		*src = BOOTSOURCE_SPI; /* Really: qspi */
		*instance = vf610_boot_instance_spi(sbmr1);
		break;
	case 1:
		*src = BOOTSOURCE_NOR;
		*instance = vf610_boot_instance_nor(sbmr1);
		break;
	case 2:
		*src = vf610_bootsource_serial_rom(sbmr1);
		*instance = vf610_boot_instance_serial_rom(sbmr1);
		break;
	case 3:
		*src = BOOTSOURCE_CAN;
		*instance = vf610_boot_instance_can(sbmr1);
		break;
	case 6:
	case 7:
		*src = BOOTSOURCE_MMC;
		*instance = vf610_boot_instance_mmc(sbmr1);
		break;
	default:
		if (imx53_bootsource_nand(sbmr1))
			*src = BOOTSOURCE_NAND;
		break;
	}
}

void vf610_boot_save_loc(void)
{
	imx_boot_save_loc(vf610_get_boot_source);
}

void imx8_get_boot_source(enum bootsource *src, int *instance)
{
	unsigned long addr;

	addr = (imx8mq_cpu_revision() == IMX_CHIP_REV_1_0) ?
		IMX8M_BOOT_SW_INFO_POINTER_ADDR_A0 :
		IMX8M_BOOT_SW_INFO_POINTER_ADDR_B0;

	__imx7_get_boot_source(src, instance, addr);
}

void imx8_boot_save_loc(void)
{
	imx_boot_save_loc(imx8_get_boot_source);
}
