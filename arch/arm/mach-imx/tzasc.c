// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "tzc380: " fmt

#include <common.h>
#include <mach/imx/generic.h>
#include <mach/imx/tzasc.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/log2.h>
#include <linux/sizes.h>
#include <mach/imx/imx8m-regs.h>
#include <io.h>

/*******************************************************************************
 *                         TZC380 defines
 ******************************************************************************/

#define TZC380_BUILD_CONFIG		0x000
#define   TZC380_BUILD_CONFIG_AW	GENMASK(13, 8)
#define   TZC380_BUILD_CONFIG_NR	GENMASK(4, 0)

/*
 * All TZC region configuration registers are placed one after another. It
 * depicts size of block of registers for programming each region.
 */
#define TZC380_REGION_REG_SIZE		0x10

#define TZC380_REGION_SETUP_LOW_0	0x100
#define TZC380_REGION_SETUP_HIGH_0	0x104
#define TZC380_REGION_ATTR_0		0x108
#define   TZC380_REGION_SP		GENMASK(31, 28)
#define   TZC380_SUBREGION_DIS_MASK	GENMASK(15, 8)
#define   TZC380_REGION_SIZE		GENMASK(6, 1)
#define   TZC380_REGION_EN		BIT(0)

/* ID Registers */
#define TZC380_PID0_OFF			0xfe0
#define TZC380_PID1_OFF			0xfe4
#define   TZC380_PERIPHERAL_ID		0x380
#define TZC380_PID2_OFF			0xfe8
#define TZC380_PID3_OFF			0xfec
#define TZC380_PID4_OFF			0xfd0
#define TZC380_CID0_OFF			0xff0
#define TZC380_CID1_OFF			0xff4
#define TZC380_CID2_OFF			0xff8

#define TZC380_REGION_OFFSET(region_no)		\
		(TZC380_REGION_REG_SIZE * (region_no))
#define TZC380_REGION_SETUP_LOW(region_no)	\
		(TZC380_REGION_OFFSET(region_no) + TZC380_REGION_SETUP_LOW_0)
#define TZC380_REGION_SETUP_HIGH(region_no)	\
		(TZC380_REGION_OFFSET(region_no) + TZC380_REGION_SETUP_HIGH_0)
#define TZC380_REGION_ATTR(region_no)		\
		(TZC380_REGION_OFFSET(region_no) + TZC380_REGION_ATTR_0)

#define TZC380_REGION_SP_NS_W		FIELD_PREP(TZC380_REGION_SP, BIT(0))
#define TZC380_REGION_SP_NS_R		FIELD_PREP(TZC380_REGION_SP, BIT(1))
#define TZC380_REGION_SP_S_W		FIELD_PREP(TZC380_REGION_SP, BIT(2))
#define TZC380_REGION_SP_S_R		FIELD_PREP(TZC380_REGION_SP, BIT(3))

#define TZC380_REGION_SP_ALL		\
		(TZC380_REGION_SP_NS_W | TZC380_REGION_SP_NS_R | \
		 TZC380_REGION_SP_S_W | TZC380_REGION_SP_S_R)
#define TZC380_REGION_SP_S_RW		\
		(TZC380_REGION_SP_S_W | TZC380_REGION_SP_S_R)
#define TZC380_REGION_SP_NS_RW		\
		(TZC380_REGION_SP_NS_W | TZC380_REGION_SP_NS_R)

/*******************************************************************************
 *                         SoC specific defines
 ******************************************************************************/

#define GPR_TZASC_EN					BIT(0)
#define GPR_TZASC_ID_SWAP_BYPASS		BIT(1)
#define GPR_TZASC_EN_LOCK				BIT(16)
#define GPR_TZASC_ID_SWAP_BYPASS_LOCK	BIT(17)

#define MX8M_TZASC_REGION_ATTRIBUTES_0		(MX8M_TZASC_BASE_ADDR + 0x108)
#define MX8M_TZASC_REGION_ATTRIBUTES_0_SP	GENMASK(31, 28)

/*
 * Implementation defined values used to validate inputs later.
 * Filters : max of 4 ; 0 to 3
 * Regions : max of 9 ; 0 to 8
 * Address width : Values between 32 to 64
 */
struct tzc380_instance {
	void __iomem *base;
	uint8_t addr_width;
	uint8_t num_regions;
};

/* Some platforms like i.MX6 does have two tzc380 controllers */
static struct tzc380_instance tzc380_inst[2];

static inline unsigned int tzc_read_peripheral_id(void __iomem *base)
{
	unsigned int id;

	id = in_le32(base + TZC380_PID0_OFF);
	/* Masks DESC part in PID1 */
	id |= ((in_le32(base + TZC380_PID1_OFF) & 0xFU) << 8U);

	return id;
}

static struct tzc380_instance *tzc380_init(void __iomem *base)
{
	struct tzc380_instance *tzc380 = &tzc380_inst[0];
	unsigned int tzc380_id;
	unsigned int tzc380_build;

	if (tzc380->base)
		tzc380 = &tzc380_inst[1];

	if (tzc380->base)
		panic("TZC-380: No free memory\n");

	tzc380->base = base;

	tzc380_id = tzc_read_peripheral_id(base);
	if (tzc380_id != TZC380_PERIPHERAL_ID)
		panic("TZC-380 : Wrong device ID (0x%x).\n", tzc380_id);

	/* Save values we will use later. */
	tzc380_build = in_le32(base + TZC380_BUILD_CONFIG);
	tzc380->addr_width  = FIELD_GET(TZC380_BUILD_CONFIG_AW, tzc380_build) + 1;
	tzc380->num_regions = FIELD_GET(TZC380_BUILD_CONFIG_NR, tzc380_build) + 1;

	return tzc380;
}

static void
tzc380_configure_region(struct tzc380_instance *tzc380, unsigned int region,
			uint64_t region_base, unsigned int region_attr)
{
	void __iomem *base = tzc380->base;

	/* Do range checks on regions */
	ASSERT((region < tzc380->num_regions));

	pr_debug("Configuring region %u\n", region);
	pr_debug("... base = %#llx\n", region_base);
	pr_debug("... sp = %#x\n",
		 (unsigned int)FIELD_GET(TZC380_REGION_SP, region_attr));
	pr_debug("... subregion dis-mask = %#x\n",
		 (unsigned int)FIELD_GET(TZC380_SUBREGION_DIS_MASK, region_attr));
	pr_debug("... size = %#x\n",
		 (unsigned int)FIELD_GET(TZC380_REGION_SIZE, region_attr));
	pr_debug("... enable = %#x\n",
		 (unsigned int)FIELD_GET(TZC380_REGION_EN, region_attr));

	/***************************************************/
	/* Inputs look ok, start programming registers.    */
	/* The address registers are 32 bits wide and      */
	/* have a LOW and HIGH				   */
	/* component used to construct an address up to a  */
	/* 64bit.					   */
	/***************************************************/
	out_le32(base + TZC380_REGION_SETUP_LOW(region), (uint32_t)region_base);
	out_le32(base + TZC380_REGION_SETUP_HIGH(region), (uint32_t)(region_base >> 32));

	/* Set region attributes */
	out_le32(base + TZC380_REGION_ATTR(region), region_attr);
}

static int
tzc380_auto_configure(struct tzc380_instance *tzc380, unsigned int region,
		      uint64_t base, uint64_t size,
		      unsigned int attr)
{
	uint64_t sub_region_size = 0;
	uint64_t area = 0;
	uint8_t lregion = region;
	uint64_t region_size = 0;
	uint64_t sub_address = 0;
	uint64_t address = base;
	uint64_t lsize = size;
	unsigned int lattr;
	uint32_t mask = 0;
	uint64_t reminder;
	int i = 0;
	uint8_t pow = 0;

	ASSERT(tzc380->base);

	/*
	 * TZC380 RM
	 * For region_attributes_<n> registers, region_size:
	 * Note: The AXI address width, that is AXI_ADDRESS_MSB+1, controls the
	 * upper limit value of the field.
	 */
	pow = tzc380->addr_width;

	while (lsize != 0 && pow >= 15) {
		region_size = 1ULL << pow;

		/* Case region fits alignment and covers requested area */
		if ((address % region_size == 0) &&
		    ((address + lsize) % region_size == 0)) {
			lattr = attr;
			lattr |= FIELD_PREP(TZC380_REGION_SIZE, (pow - 1));
			lattr |= TZC380_REGION_EN;

			tzc380_configure_region(tzc380, lregion, address, lattr);

			lregion++;
			address += region_size;
			lsize -= region_size;
			pow = tzc380->addr_width;
			continue;
		}

		/* Cover area using several subregions */
		sub_region_size = div_u64(region_size, 8);
		div64_u64_rem(address, sub_region_size, &reminder);
		if (reminder == 0 && lsize > 2 * sub_region_size) {
			sub_address = div64_u64(address, region_size) * region_size;
			mask = 0;
			for (i = 0; i < 8; i++) {
				area = (i + 1) * sub_region_size;
				if (sub_address + area <= address ||
				    sub_address + area > address + lsize) {
					mask |= FIELD_PREP(TZC380_SUBREGION_DIS_MASK, BIT(i));
				} else {
					address += sub_region_size;
					lsize -= sub_region_size;
				}
			}

			lattr = mask | attr;
			lattr |= FIELD_PREP(TZC380_REGION_SIZE, (pow - 1));
			lattr |= TZC380_REGION_EN;

			tzc380_configure_region(tzc380, lregion, sub_address, lattr);

			lregion++;
			pow = tzc380->addr_width;
			continue;
		}
		pow--;
	}
	ASSERT(lsize == 0);
	ASSERT(address == base + size);
	return lregion;
}

/******************************************************************************
 *                          SoC specific helpers
 ******************************************************************************/

void imx8m_tzc380_init(void)
{
	u32 __iomem *gpr = IOMEM(MX8M_IOMUXC_GPR_BASE_ADDR);

	/* Enable TZASC and lock setting */
	setbits_le32(&gpr[10], GPR_TZASC_EN);
	setbits_le32(&gpr[10], GPR_TZASC_EN_LOCK);

	/*
	 * According to TRM, TZASC_ID_SWAP_BYPASS should be set in
	 * order to avoid AXI Bus errors when GPU is in use
	 */
	if (cpu_is_mx8mm() || cpu_is_mx8mn() || cpu_is_mx8mp())
		setbits_le32(&gpr[10], GPR_TZASC_ID_SWAP_BYPASS);

	/*
	 * imx8mn and imx8mp implements the lock bit for
	 * TZASC_ID_SWAP_BYPASS, enable it to lock settings
	 */
	if (cpu_is_mx8mn() || cpu_is_mx8mp())
		setbits_le32(&gpr[10], GPR_TZASC_ID_SWAP_BYPASS_LOCK);

	/*
	 * set Region 0 attribute to allow secure and non-secure
	 * read/write permission. Found some masters like usb dwc3
	 * controllers can't work with secure memory.
	 */
	writel(MX8M_TZASC_REGION_ATTRIBUTES_0_SP,
		   MX8M_TZASC_REGION_ATTRIBUTES_0);
}

bool imx8m_tzc380_is_enabled(void)
{
	u32 __iomem *gpr = IOMEM(MX8M_IOMUXC_GPR_BASE_ADDR);

	return (readl(&gpr[10]) & (GPR_TZASC_EN | GPR_TZASC_EN_LOCK))
		== (GPR_TZASC_EN | GPR_TZASC_EN_LOCK);
}
