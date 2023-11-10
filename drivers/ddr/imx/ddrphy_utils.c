// SPDX-License-Identifier: GPL-2.0-or-later
/*
* Copyright 2018 NXP
*/

#define pr_fmt(fmt) "imx8m-ddr: " fmt

#include <common.h>
#include <errno.h>
#include <io.h>
#include <linux/iopoll.h>
#include <soc/imx8m/ddr.h>
#include <mach/imx/imx8m-regs.h>
#include <mach/imx/imx8m-ccm-regs.h>

void ddrc_phy_load_firmware(void __iomem *phy,
			    enum ddrc_phy_firmware_offset offset,
			    const u16 *blob, size_t size)
{
	while (size) {
		writew(*blob++, phy + DDRC_PHY_REG(offset));
		offset++;
		size -= sizeof(*blob);
	}
}

enum pmc_constants {
	PMC_MESSAGE_ID,
	PMC_MESSAGE_STREAM,

	PMC_TRAIN_SUCCESS	= 0x07,
	PMC_TRAIN_STREAM_START	= 0x08,
	PMC_TRAIN_FAIL		= 0xff,
};

static u32 ddrc_phy_get_message(void __iomem *phy, int type)
{
	u32 r, message;

	/*
	 * When BIT0 set to 0, the PMU has a message for the user
	 * Wait for it indefinitely.
	 */
	readl_poll_timeout(phy + DDRC_PHY_REG(0xd0004),
			   r, !(r & BIT(0)), 0);

	switch (type) {
	case PMC_MESSAGE_ID:
		/*
		 * Get the major message ID
		 */
		message = readl(phy + DDRC_PHY_REG(0xd0032));
		break;
	case PMC_MESSAGE_STREAM:
		message = readl(phy + DDRC_PHY_REG(0xd0034));
		message <<= 16;
		message |= readl(phy + DDRC_PHY_REG(0xd0032));
		break;
	}

	/*
	 * By setting this register to 0, the user acknowledges the
	 * receipt of the message.
	 */
	writel(0x00000000, phy + DDRC_PHY_REG(0xd0031));
	/*
	 * When BIT0 set to 0, the PMU has a message for the user
	 */
	readl_poll_timeout(phy + DDRC_PHY_REG(0xd0004),
			   r, r & BIT(0), 0);

	writel(0x00000001, phy + DDRC_PHY_REG(0xd0031));

	return message;
}

static void ddrc_phy_fetch_streaming_message(void __iomem *phy)
{
	const u16 index = ddrc_phy_get_message(phy, PMC_MESSAGE_STREAM);
	u16 i;

	for (i = 0; i < index; i++)
		ddrc_phy_get_message(phy, PMC_MESSAGE_STREAM);
}

int wait_ddrphy_training_complete(void)
{
	void __iomem *phy = IOMEM(MX8M_DDRC_PHY_BASE_ADDR);

	for (;;) {
		const u32 m = ddrc_phy_get_message(phy, PMC_MESSAGE_ID);

		switch (m) {
		case PMC_TRAIN_STREAM_START:
			ddrc_phy_fetch_streaming_message(phy);
			break;
		case PMC_TRAIN_SUCCESS:
			return 0;
		case PMC_TRAIN_FAIL:
			hang();
		}
	}
}
