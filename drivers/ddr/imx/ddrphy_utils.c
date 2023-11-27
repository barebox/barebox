// SPDX-License-Identifier: GPL-2.0-or-later
/*
* Copyright 2018 NXP
*/

#define pr_fmt(fmt) "imx-ddr: " fmt

#include <common.h>
#include <errno.h>
#include <io.h>
#include <linux/iopoll.h>
#include <soc/imx8m/ddr.h>

void ddrc_phy_load_firmware(struct dram_controller *dram,
			    enum ddrc_phy_firmware_offset offset,
			    const u16 *blob, size_t size)
{
	while (size) {
		writew(*blob++, dwc_ddrphy_apb_addr(dram, offset));
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

static u32 ddrc_phy_get_message(struct dram_controller *dram, int type)
{
	u32 message;

	/*
	 * When BIT0 set to 0, the PMU has a message for the user
	 * Wait for it indefinitely.
	 */
	while (dwc_ddrphy_apb_rd(dram, 0xd0004) & BIT(0));

	switch (type) {
	case PMC_MESSAGE_ID:
		/*
		 * Get the major message ID
		 */
		message = dwc_ddrphy_apb_rd(dram, 0xd0032);
		break;
	case PMC_MESSAGE_STREAM:
		message = dwc_ddrphy_apb_rd(dram, 0xd0034);
		message <<= 16;
		message |= dwc_ddrphy_apb_rd(dram, 0xd0032);
		break;
	}

	/*
	 * By setting this register to 0, the user acknowledges the
	 * receipt of the message.
	 */
	dwc_ddrphy_apb_wr(dram, 0xd0031, 0x00000000);
	/*
	 * When BIT0 set to 0, the PMU has a message for the user
	 */
	while (!(dwc_ddrphy_apb_rd(dram, 0xd0004) & BIT(0)));

	dwc_ddrphy_apb_wr(dram, 0xd0031, 0x00000001);

	return message;
}

static void ddrc_phy_fetch_streaming_message(struct dram_controller *dram)
{
	const u16 index = ddrc_phy_get_message(dram, PMC_MESSAGE_STREAM);
	u16 i;

	for (i = 0; i < index; i++)
		ddrc_phy_get_message(dram, PMC_MESSAGE_STREAM);
}

int wait_ddrphy_training_complete(struct dram_controller *dram)
{
	for (;;) {
		const u32 m = ddrc_phy_get_message(dram, PMC_MESSAGE_ID);

		switch (m) {
		case PMC_TRAIN_STREAM_START:
			ddrc_phy_fetch_streaming_message(dram);
			break;
		case PMC_TRAIN_SUCCESS:
			return 0;
		case PMC_TRAIN_FAIL:
			hang();
		}
	}
}
