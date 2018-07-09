#include <common.h>
#include <linux/iopoll.h>
#include <mach/imx8-ddrc.h>
#include <debug_ll.h>

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
	 * 10ms seems not enough for poll message, so use 1s here.
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

	putc_ll('|');
	puthex_ll(index);

	for (i = 0; i < index; i++) {
		const u32 arg = ddrc_phy_get_message(phy, PMC_MESSAGE_STREAM);

		putc_ll('|');
		puthex_ll(arg);
	}
}

void ddrc_phy_wait_training_complete(void __iomem *phy)
{
	for (;;) {
		const u32 m = ddrc_phy_get_message(phy, PMC_MESSAGE_ID);

		puthex_ll(m);

		switch (m) {
		case PMC_TRAIN_STREAM_START:
			ddrc_phy_fetch_streaming_message(phy);
			break;
		case PMC_TRAIN_SUCCESS:
			putc_ll('P');
			putc_ll('\r');
			putc_ll('\n');
			return;
		case PMC_TRAIN_FAIL:
			putc_ll('F');
			hang();
		}

		putc_ll('\r');
		putc_ll('\n');
	}
}