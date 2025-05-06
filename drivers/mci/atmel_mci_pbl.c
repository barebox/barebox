// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <mach/at91/xload.h>
#include <mci.h>

#include "atmel-mci-regs.h"

#define SECTOR_SIZE			512
#define SUPPORT_MAX_BLOCKS		16U

struct atmel_mci_priv {
	struct atmel_mci host;
	bool highcapacity_card;
};

static struct atmel_mci_priv atmci_sdcard;

static int atmel_mci_pbl_stop_transmission(struct atmel_mci_priv *priv)
{
	struct mci_cmd cmd = {
		.cmdidx = MMC_CMD_STOP_TRANSMISSION,
		.resp_type = MMC_RSP_R1b,
	};

	return atmci_common_request(&priv->host, &cmd, NULL);
}

static int at91_mci_sd_cmd_read_multiple_block(struct atmel_mci_priv *priv,
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

	return atmci_common_request(&priv->host, &cmd, &data);
}

static int at91_mci_bio_read(struct pbl_bio *bio, off_t start,
				void *buf, unsigned int nblocks)
{
	struct atmel_mci_priv *priv = bio->priv;
	unsigned int blocks_done = 0;
	unsigned int blocks;
	unsigned int block_len = SECTOR_SIZE;
	unsigned int blocks_read;
	int ret;

	while (blocks_done < nblocks) {
		blocks = min(nblocks - blocks_done, SUPPORT_MAX_BLOCKS);

		blocks_read = at91_mci_sd_cmd_read_multiple_block(priv, buf,
							 start + blocks_done,
							 blocks);

		ret = atmel_mci_pbl_stop_transmission(priv);
		if (ret)
			return ret;

		blocks_done += blocks_read;

		if (blocks_read != blocks)
			break;

		buf += blocks * block_len;
	}

	return blocks_done;
}

int at91_mci_bio_init(struct pbl_bio *bio, void __iomem *base,
		      unsigned int clock, unsigned int slot,
		      enum pbl_mci_capacity capacity)
{
	struct atmel_mci_priv *priv = &atmci_sdcard;
	struct atmel_mci *host = &priv->host;
	struct mci_ios ios = { .bus_width = MMC_BUS_WIDTH_4, .clock = 25000000 };

	/* PBL will get MCI controller in disabled state. We need to reconfigure
	 * it. */
	bio->priv = priv;
	bio->read = at91_mci_bio_read;

	host->regs = base;

	atmci_get_cap(host);

	host->bus_hz = clock;

	host->slot_b = slot;
	if (host->slot_b)
		host->sdc_reg = ATMCI_SDCSEL_SLOT_B;
	else
		host->sdc_reg = ATMCI_SDCSEL_SLOT_A;

	atmci_writel(host, ATMCI_CR, ATMCI_CR_PWSDIS);
	atmci_writel(host, ATMCI_DTOR, 0x7f);

	atmci_common_set_ios(host, &ios);

	if (capacity == PBL_MCI_STANDARD_CAPACITY)
		atmci_sdcard.highcapacity_card = false;
	else
		atmci_sdcard.highcapacity_card = true;

	return 0;
}
