// SPDX-License-Identifier: BSD-1-Clause
/*
 * Copyright (c) 2015, Atmel Corporation
 * Copyright (c) 2019, Ahmad Fatoum, Pengutronix
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 */

#include <common.h>
#include <pbl.h>
#include <mci.h>
#include <debug_ll.h>
#include <mach/xload.h>
#include "atmel-sdhci.h"

#include <mach/early_udelay.h>

#ifdef __PBL__
#define udelay early_udelay
#endif

#define SECTOR_SIZE			512
#define SUPPORT_MAX_BLOCKS		16U

struct at91_sdhci_priv {
	struct at91_sdhci host;
	bool highcapacity_card;
};

static int sd_cmd_stop_transmission(struct at91_sdhci_priv *priv)
{
	struct mci_cmd cmd = {
		.cmdidx = MMC_CMD_STOP_TRANSMISSION,
		.resp_type = MMC_RSP_R1b,
	};

	return at91_sdhci_send_command(&priv->host, &cmd, NULL);
}

static int sd_cmd_read_multiple_block(struct at91_sdhci_priv *priv,
				      void *buf,
				      unsigned int start,
				      unsigned int block_count)
{
	u16 block_len = SECTOR_SIZE;
	struct mci_data data;
	struct mci_cmd cmd = {
		.cmdidx = MMC_CMD_READ_MULTIPLE_BLOCK,
		.resp_type = MMC_RSP_R1,
		.cmdarg = start,
	};

	if (!priv->highcapacity_card)
		cmd.cmdarg *= block_len;

	data.dest = buf;
	data.flags = MMC_DATA_READ;
	data.blocksize = block_len;
	data.blocks = block_count;

	return at91_sdhci_send_command(&priv->host, &cmd, &data);
}

static int at91_sdhci_bio_read(struct pbl_bio *bio, off_t start,
				void *buf, unsigned int nblocks)
{
	struct at91_sdhci_priv *priv = bio->priv;
	unsigned int blocks_done = 0;
	unsigned int blocks;
	unsigned int block_len = SECTOR_SIZE;
	unsigned int blocks_read;
	int ret;

	/*
	 * Refer to the at91sam9g20 datasheet:
	 * Figure 35-10. Read Function Flow Diagram
	*/

	while (blocks_done < nblocks) {
		blocks = min(nblocks - blocks_done, SUPPORT_MAX_BLOCKS);

		blocks_read = sd_cmd_read_multiple_block(priv, buf,
							 start + blocks_done,
							 blocks);

		ret = sd_cmd_stop_transmission(priv);
		if (ret)
			return ret;

		blocks_done += blocks_read;

		if (blocks_read != blocks)
			break;

		buf += blocks * block_len;
	}

	return blocks_done;
}

static struct at91_sdhci_priv atmel_sdcard;

int at91_sdhci_bio_init(struct pbl_bio *bio, void __iomem *base)
{
	struct at91_sdhci_priv *priv = &atmel_sdcard;
	struct at91_sdhci *host = &priv->host;
	struct mci_ios ios = { .bus_width = MMC_BUS_WIDTH_1, .clock = 25000000 };
	int ret;

	bio->priv = priv;
	bio->read = at91_sdhci_bio_read;

	at91_sdhci_mmio_init(host, base);

	sdhci_reset(&host->sdhci, SDHCI_RESET_CMD | SDHCI_RESET_DATA);

	ret = at91_sdhci_init(host, 240000000, true, true);
	if (ret)
		return ret;

	ret = at91_sdhci_set_ios(host, &ios);

	 // FIXME can we determine this without leaving SD transfer mode?
	priv->highcapacity_card = 1;

	return 0;
}
