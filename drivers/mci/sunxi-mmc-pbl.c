// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2024 Jules Maselbas
#include <common.h>

#include <mach/sunxi/xload.h>
#include "sunxi-mmc.h"
#include "sunxi-mmc-common.c"

#define SECTOR_SIZE			512
#define SUPPORT_MAX_BLOCKS		16U

static int sunxi_mmc_read_block(struct sunxi_mmc_host *host,
				void *dst, unsigned int blocknum,
				unsigned int blocks)
{
	struct mci_data data;
	struct mci_cmd cmd = {
		.cmdidx = (blocks > 1) ? MMC_CMD_READ_MULTIPLE_BLOCK : MMC_CMD_READ_SINGLE_BLOCK,
		 /* mci->high_capacity ? blocknum : blocknum * mci->read_bl_len, */
		 /* TODO: detect if card is high-capacity */
		.cmdarg = blocknum,
		.resp_type = MMC_RSP_R1,
	};
	int ret;

	data.dest = dst;
	data.blocks = blocks;
	data.blocksize = SECTOR_SIZE; /* compat with MMC/SD */
	data.flags = MMC_DATA_READ;

	ret = sunxi_mmc_send_cmd(host, &cmd, &data, NULL);

	if (ret || blocks > 1) {
		cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
		cmd.cmdarg = 0;
		cmd.resp_type = MMC_RSP_R1b;
		sunxi_mmc_send_cmd(host, &cmd, NULL, NULL);
	}

	return ret;
}

static int sunxi_mmc_bio_read(struct pbl_bio *bio, off_t start,
				void *buf, unsigned int nblocks)
{
	struct sunxi_mmc_host *host = bio->priv;
	unsigned int count = 0;
	unsigned int block_len = SECTOR_SIZE;
	int ret;

	while (count < nblocks) {
		unsigned int n = min_t(unsigned int, nblocks - count, SUPPORT_MAX_BLOCKS);

		ret = sunxi_mmc_read_block(host, buf, start, n);
		if (ret < 0)
			return ret;

		count += n;
		start += n;
		buf += n * block_len;
	}

	return count;
}

int sunxi_mmc_bio_init(struct pbl_bio *bio, void __iomem *base,
		       unsigned int clock, unsigned int slot)
{
	static struct sunxi_mmc_host host;
	struct mci_ios ios = { .bus_width = MMC_BUS_WIDTH_4, .clock = 400000 };

	host.base = base;
	host.clkrate = clock;
	bio->priv = &host;
	bio->read = sunxi_mmc_bio_read;

	sunxi_mmc_init(&host);
	sunxi_mmc_set_ios(&host, &ios);

	return 0;
}
