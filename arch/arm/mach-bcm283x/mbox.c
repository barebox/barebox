/*
 * based on U-Boot code
 *
 * (C) Copyright 2012 Stephen Warren
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#define pr_fmt(fmt) "rpi-mbox: " fmt

#include <clock.h>
#include <common.h>
#include <dma.h>
#include <init.h>
#include <io.h>
#include <of_address.h>
#include <pbl.h>

#include <mach/bcm283x/mbox.h>
#include <mach/bcm283x/core.h>

#define TIMEOUT (MSECOND * 1000)

static void __iomem *mbox_base;

#ifdef __PBL__
#define is_timeout_non_interruptible(start, timeout) ((void)start, 0)
#define get_time_ns() 0
#endif

static int bcm2835_mbox_call_raw(u32 chan, struct bcm2835_mbox_hdr *buffer,
					u32 *recv)
{
	uint64_t starttime = get_time_ns();
	u32 send = virt_to_phys(buffer);
	u32 val;

	if (send & BCM2835_CHAN_MASK) {
		pr_err("mbox: Illegal mbox data 0x%08x\n", send);
		return -EINVAL;
	}

	/* Drain any stale responses */
	for (;;) {
		val = readl(mbox_base + MAIL0_STA);
		if (val & BCM2835_MBOX_STATUS_RD_EMPTY)
			break;
		if (is_timeout_non_interruptible(starttime, TIMEOUT)) {
			pr_err("mbox: Timeout draining stale responses\n");
			return -ETIMEDOUT;
		}
		val = readl(mbox_base + MAIL0_RD);
	}

	/* Wait for space to send */
	for (;;) {
		val = readl(mbox_base + MAIL0_STA);
		if (!(val & BCM2835_MBOX_STATUS_WR_FULL))
			break;
		if (is_timeout_non_interruptible(starttime, TIMEOUT)) {
			pr_err("mbox: Timeout waiting for send space\n");
			return -ETIMEDOUT;
		}
	}

	/* Send the request */
	val = BCM2835_MBOX_PACK(chan, send);
	pr_debug("mbox: TX raw: 0x%08x\n", val);
	dma_sync_single_for_device(NULL, (unsigned long)send, buffer->buf_size,
				   DMA_BIDIRECTIONAL);
	writel(val, mbox_base + MAIL1_WRT);

	/* Wait for the response */
	for (;;) {
		val = readl(mbox_base + MAIL0_STA);
		if (!(val & BCM2835_MBOX_STATUS_RD_EMPTY))
			break;
		if (is_timeout_non_interruptible(starttime, TIMEOUT)) {
			pr_err("mbox: Timeout waiting for response\n");
			return -ETIMEDOUT;
		}
	}

	/* Read the response */
	val = readl(mbox_base + MAIL0_RD);
	pr_debug("mbox: RX raw: 0x%08x\n", val);
	dma_sync_single_for_cpu(NULL, (unsigned long)send, buffer->buf_size,
				DMA_BIDIRECTIONAL);

	/* Validate the response */
	if (BCM2835_MBOX_UNPACK_CHAN(val) != chan) {
		pr_err("mbox: Response channel mismatch\n");
		return -EIO;
	}

	*recv = BCM2835_MBOX_UNPACK_DATA(val);

	return 0;
}

#ifdef DEBUG
static void dump_buf(struct bcm2835_mbox_hdr *buffer)
{
	u32 *p;
	u32 words;
	int i;

	p = (u32 *)buffer;
	words = buffer->buf_size / 4;
	for (i = 0; i < words; i++)
		pr_debug("    0x%04x: 0x%08x\n", i * 4, p[i]);
}
#else
static void dump_buf(struct bcm2835_mbox_hdr *buffer)
{
}
#endif

static void __iomem *bcm2835_mbox_probe(void)
{
	struct device_node *mbox_node;

	if (IN_PBL)
		return bcm2835_get_mmio_base_by_cpuid() + 0xb880;

	mbox_node = of_find_compatible_node(NULL, NULL, "brcm,bcm2835-mbox");
	if (!mbox_node) {
		pr_err("Missing mbox node\n");
		return NULL;
	}

	return of_iomap(mbox_node, 0);
}

int bcm2835_mbox_call_prop(u32 chan, struct bcm2835_mbox_hdr *buffer)
{
	int ret;
	u32 rbuffer;
	struct bcm2835_mbox_tag_hdr *tag;
	int tag_index;

	if (!mbox_base) {
		mbox_base = bcm2835_mbox_probe();
		if (!mbox_base)
			return -ENOENT;
	}

	pr_debug("mbox: TX buffer\n");
	dump_buf(buffer);

	ret = bcm2835_mbox_call_raw(chan, buffer, &rbuffer);
	if (ret)
		return ret;
	if (rbuffer != (uintptr_t)buffer) {
		pr_err("mbox: Response buffer mismatch\n");
		return -EIO;
	}

	pr_debug("mbox: RX buffer\n");
	dump_buf(buffer);

	/* Validate overall response status */
	if (buffer->code != BCM2835_MBOX_RESP_CODE_SUCCESS) {
		pr_err("mbox: Header response code invalid\n");
		return -EIO;
	}

	/* Validate each tag's response status */
	tag = (void *)(buffer + 1);
	tag_index = 0;
	while (tag->tag) {
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
