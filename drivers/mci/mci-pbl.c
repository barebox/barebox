// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2025 Ahmad Fatoum
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

static enum pbl_mci_capacity capacity_decode(unsigned csd_struct_ver)
{
	switch (csd_struct_ver) {
	case 0:
		return PBL_MCI_STANDARD_CAPACITY;
	case 1:
		return PBL_MCI_HIGH_CAPACITY;
	case 2:
		return PBL_MCI_ULTRA_CAPACITY;
	case 3:
		return PBL_MCI_RESERVED_CAPACITY;
	}

	return PBL_MCI_UNKNOWN_CAPACITY;
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

/**
 * pbl_mci_check_high_capacity() - Determine what type of card we have
 * @mci: already initialized MCI card
 *
 * Standard capacity cards use a byte offset for the READ_MULTIPLE_BLOCK
 * command argument, which high capacity uses a block offset.
 *
 * As we haven't set up the card ourselves here, we do not have
 * this information directly available.
 *
 * A workaround is reading from address 0 onwards, which works for raw
 * second stage barebox located near start of the card, but is not so
 * good when we need to load barebox from a file system.
 *
 * This function implements the necessary state transitions, so we
 * can reread the CSD and determine, which the card is high capacity
 * or not.
 *
 * Refer to Part1_Physical_Layer_Simplified_Specification_Ver9.00.pdf
 * "Figure 4-13 : SD Memory Card State Diagram (data transfer mode)"
 * for a visualization of how this works.
 *
 * Return: 0 when capacity type could be determined and
 * mci->high_capacity was updated or a negative return code upon error.
 */
static int pbl_mci_check_high_capacity(struct pbl_mci *mci)
{
	struct mci_cmd cmd = {};
	unsigned rca_arg = 0; /* RCA shifted up by 16 bis */
	int ret, ret2;

	/* CMD7: Deselect all cards and move into standby mode */
	mci_setup_cmd(&cmd, MMC_CMD_SELECT_CARD, 0, MMC_RSP_NONE);
	ret = pbl_mci_send_cmd(mci, &cmd, NULL);
	if (ret) {
		pr_debug("deselecting cards failed: %d\n", ret);
		return ret;
	}

	/*
	 * CMD3: Need to get RCA for SDs, so we can use it to communicate
	 * with the correct SD (argument to CMD9)
	 *
	 * TODO: What to do about eMMCs? These would get a
	 * MMC_CMD_SET_RELATIVE_ADDR here
	 *
	 * Figure 27 — e•MMC state diagram (data transfer mode) n
	 * JESD84-B51.pdf doesn't list CMD3 as allowed command while
	 * in stand-by mode
	 */
	mci_setup_cmd(&cmd, SD_CMD_SEND_RELATIVE_ADDR, 0, MMC_RSP_R6);
	ret = pbl_mci_send_cmd(mci, &cmd, NULL);
	if (ret) {
		pr_debug("sending relative address failed: %d\n", ret);
		goto out;
	}

	rca_arg = cmd.response[0] & GENMASK(31, 16);

	/* CMD9: Get the Card-Specific Data */
	mci_setup_cmd(&cmd, MMC_CMD_SEND_CSD, rca_arg, MMC_RSP_R2);
	ret = pbl_mci_send_cmd(mci, &cmd, NULL);
	if (ret) {
		pr_err("sending CSD failed: %d\n", ret);
		goto out;
	}

	/* Check CSD version */
	mci->capacity = capacity_decode(cmd.response[0] >> 30);
	pr_debug("detect %s capacity\n", capacity_tostr(mci->capacity));

out:
	/* CMD7: now let's got get back to transfer mode */
	mci_setup_cmd(&cmd, MMC_CMD_SELECT_CARD, rca_arg, MMC_RSP_R1b);
	ret2 = pbl_mci_send_cmd(mci, &cmd, NULL);
	if (ret2) {
		pr_err("reselecting card with %x failed: %d\n", rca_arg, ret2);
		return ret2;
	}

	return ret;
}

int pbl_mci_bio_init(struct pbl_mci *mci, struct pbl_bio *bio)
{
	/*
	 * There's no useful recovery possible here, so we try to continue
	 * after having printed an error
	 */
	if (mci->capacity == PBL_MCI_UNKNOWN_CAPACITY)
		(void)pbl_mci_check_high_capacity(mci);
	else
		pr_debug("assuming %s capacity card\n",
			 capacity_tostr(mci->capacity));

	bio->priv = mci;
	bio->read = pbl_bio_mci_read;

	return 0;
}
