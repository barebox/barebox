/*
 * based on U-Boot code
 *
 * (C) Copyright 2012 Stephen Warren
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <clock.h>
#include <common.h>
#include <dma.h>
#include <init.h>
#include <io.h>

#include <mach/mbox.h>

#define TIMEOUT (MSECOND * 1000)

static void __iomem *mbox_base;

static int bcm2835_mbox_call_raw(u32 chan, struct bcm2835_mbox_hdr *buffer,
					u32 *recv)
{
	uint64_t starttime = get_time_ns();
	u32 send = virt_to_phys(buffer);
	u32 val;

	if (send & BCM2835_CHAN_MASK) {
		printf("mbox: Illegal mbox data 0x%08x\n", send);
		return -EINVAL;
	}

	/* Drain any stale responses */
	for (;;) {
		val = readl(mbox_base + MAIL0_STA);
		if (val & BCM2835_MBOX_STATUS_RD_EMPTY)
			break;
		if (is_timeout(starttime, TIMEOUT)) {
			printf("mbox: Timeout draining stale responses\n");
			return -ETIMEDOUT;
		}
		val = readl(mbox_base + MAIL0_RD);
	}

	/* Wait for space to send */
	for (;;) {
		val = readl(mbox_base + MAIL0_STA);
		if (!(val & BCM2835_MBOX_STATUS_WR_FULL))
			break;
		if (is_timeout(starttime, TIMEOUT)) {
			printf("mbox: Timeout waiting for send space\n");
			return -ETIMEDOUT;
		}
	}

	/* Send the request */
	val = BCM2835_MBOX_PACK(chan, send);
	debug("mbox: TX raw: 0x%08x\n", val);
	dma_sync_single_for_device((unsigned long)send, buffer->buf_size,
				   DMA_BIDIRECTIONAL);
	writel(val, mbox_base + MAIL1_WRT);

	/* Wait for the response */
	for (;;) {
		val = readl(mbox_base + MAIL0_STA);
		if (!(val & BCM2835_MBOX_STATUS_RD_EMPTY))
			break;
		if (is_timeout(starttime, TIMEOUT)) {
			printf("mbox: Timeout waiting for response\n");
			return -ETIMEDOUT;
		}
	}

	/* Read the response */
	val = readl(mbox_base + MAIL0_RD);
	debug("mbox: RX raw: 0x%08x\n", val);
	dma_sync_single_for_cpu((unsigned long)send, buffer->buf_size,
				DMA_BIDIRECTIONAL);

	/* Validate the response */
	if (BCM2835_MBOX_UNPACK_CHAN(val) != chan) {
		printf("mbox: Response channel mismatch\n");
		return -EIO;
	}

	*recv = BCM2835_MBOX_UNPACK_DATA(val);

	return 0;
}

#ifdef DEBUG
void dump_buf(struct bcm2835_mbox_hdr *buffer)
{
	u32 *p;
	u32 words;
	int i;

	p = (u32 *)buffer;
	words = buffer->buf_size / 4;
	for (i = 0; i < words; i++)
		printf("    0x%04x: 0x%08x\n", i * 4, p[i]);
}
#endif

int bcm2835_mbox_call_prop(u32 chan, struct bcm2835_mbox_hdr *buffer)
{
	int ret;
	u32 rbuffer;
	struct bcm2835_mbox_tag_hdr *tag;
	int tag_index;

#ifdef DEBUG
	printf("mbox: TX buffer\n");
	dump_buf(buffer);
#endif

	ret = bcm2835_mbox_call_raw(chan, buffer, &rbuffer);
	if (ret)
		return ret;
	if (rbuffer != (u32)buffer) {
		printf("mbox: Response buffer mismatch\n");
		return -EIO;
	}

#ifdef DEBUG
	printf("mbox: RX buffer\n");
	dump_buf(buffer);
#endif

	/* Validate overall response status */
	if (buffer->code != BCM2835_MBOX_RESP_CODE_SUCCESS) {
		printf("mbox: Header response code invalid\n");
		return -EIO;
	}

	/* Validate each tag's response status */
	tag = (void *)(buffer + 1);
	tag_index = 0;
	while (tag->tag) {
		if (!(tag->val_len & BCM2835_MBOX_TAG_VAL_LEN_RESPONSE)) {
			printf("mbox: Tag %d missing val_len response bit\n",
				tag_index);
			return -EIO;
		}
		/*
		 * Clear the reponse bit so clients can just look right at the
		 * length field without extra processing
		 */
		tag->val_len &= ~BCM2835_MBOX_TAG_VAL_LEN_RESPONSE;
		tag = (void *)(((u8 *)tag) + sizeof(*tag) + tag->val_buf_size);
		tag_index++;
	}

	return 0;
}

static int bcm2835_mbox_probe(struct device_d *dev)
{
	struct resource *iores;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "could not get memory region\n");
		return PTR_ERR(iores);
	}
	mbox_base = IOMEM(iores->start);

	return 0;
}

static __maybe_unused struct of_device_id bcm2835_mbox_dt_ids[] = {
	{
		.compatible = "brcm,bcm2835-mbox",
	}, {
		/* sentinel */
	},
};

static struct driver_d bcm2835_mbox_driver = {
	.name		= "bcm2835_mbox",
	.of_compatible	= DRV_OF_COMPAT(bcm2835_mbox_dt_ids),
	.probe		= bcm2835_mbox_probe,
};

static int __init bcm2835_mbox_init(void)
{
	return platform_driver_register(&bcm2835_mbox_driver);
}
core_initcall(bcm2835_mbox_init);
