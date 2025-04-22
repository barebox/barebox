// SPDX-License-Identifier: GPL-2.0-only
/*
 * This code assumes that the SD/eMMC is already in transfer mode,
 * because it card initialization has been done by the BootROM.
 *
 * In interest of reducing complexity, code size and boot time, we
 * don't want to reset the card, but reuse it as-is.
 *
 * Full reinitialization of the card has to wait until we are
 * in barebox proper (see mci-core.c).
 */

#define pr_fmt(fmt) "mci-pbl: " fmt

#include <mci.h>
#include <pbl/mci.h>
#include <pbl/bio.h>
#include <linux/minmax.h>
#include <linux/bug.h>

#define MCI_PBL_BLOCK_LEN		512

static int pbl_mci_read_blocks(struct pbl_mci *mci, void *dst,
			       off_t start, unsigned int nblocks)
{
	struct mci_cmd cmd = {};
	struct mci_data data;
	int ret;

	if (mci->capacity == PBL_MCI_STANDARD_CAPACITY)
		start *= MCI_PBL_BLOCK_LEN;

	mci_setup_cmd(&cmd, MMC_CMD_READ_SINGLE_BLOCK,
		      start, MMC_RSP_R1);
	if (nblocks > 1)
		cmd.cmdidx = MMC_CMD_READ_MULTIPLE_BLOCK;

	data.dest = dst;
	data.flags = MMC_DATA_READ;
	data.blocksize = MCI_PBL_BLOCK_LEN;
	data.blocks = nblocks;

	ret = pbl_mci_send_cmd(mci, &cmd, &data);
	if (ret || nblocks > 1) {
		mci_setup_cmd(&cmd, MMC_CMD_STOP_TRANSMISSION, 0,
			      MMC_RSP_R1b);
		pbl_mci_send_cmd(mci, &cmd, NULL);
	}

	return ret;
}

static int pbl_bio_mci_read(struct pbl_bio *bio, off_t start,
			    void *buf, unsigned int nblocks)
{
	struct pbl_mci *mci = bio->priv;
	unsigned int count = 0;
	unsigned int block_len = MCI_PBL_BLOCK_LEN;
	int ret;

	while (count < nblocks) {
		unsigned n = nblocks - count;

		if (mci->max_blocks_per_read)
			n = min(n, mci->max_blocks_per_read);

		ret = pbl_mci_read_blocks(mci, buf, start, n);
		if (ret < 0)
			return ret;

		count += n;
		start += n;
		buf += n *block_len;
	}

	return count;
}

static const char *capacity_tostr(enum pbl_mci_capacity capacity)
{
	switch (capacity) {
	case PBL_MCI_STANDARD_CAPACITY:
		return "standard";
	case PBL_MCI_HIGH_CAPACITY:
		return "high/extended";
	case PBL_MCI_ULTRA_CAPACITY:
		return "ultra";
	case PBL_MCI_RESERVED_CAPACITY:
		return "reserved";
	case PBL_MCI_UNKNOWN_CAPACITY:
		break;
	}

	return "unknown";
}

int pbl_mci_bio_init(struct pbl_mci *mci, struct pbl_bio *bio)
{

	if (mci->capacity == PBL_MCI_UNKNOWN_CAPACITY)
		BUG();
	else
		pr_debug("assuming %s capacity card\n",
			 capacity_tostr(mci->capacity));

	bio->priv = mci;
	bio->read = pbl_bio_mci_read;

	return 0;
}
