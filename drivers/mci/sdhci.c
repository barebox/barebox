// SPDX-License-Identifier: GPL-2.0

#include <common.h>
#include <driver.h>
#include <mci.h>
#include <io.h>

#include "sdhci.h"

void sdhci_read_response(struct sdhci *sdhci, struct mci_cmd *cmd)
{
	if (cmd->resp_type & MMC_RSP_136) {
		u32 cmdrsp3, cmdrsp2, cmdrsp1, cmdrsp0;

		cmdrsp3 = sdhci_read32(sdhci, SDHCI_RESPONSE_3);
		cmdrsp2 = sdhci_read32(sdhci, SDHCI_RESPONSE_2);
		cmdrsp1 = sdhci_read32(sdhci, SDHCI_RESPONSE_1);
		cmdrsp0 = sdhci_read32(sdhci, SDHCI_RESPONSE_0);
		cmd->response[0] = (cmdrsp3 << 8) | (cmdrsp2 >> 24);
		cmd->response[1] = (cmdrsp2 << 8) | (cmdrsp1 >> 24);
		cmd->response[2] = (cmdrsp1 << 8) | (cmdrsp0 >> 24);
		cmd->response[3] = (cmdrsp0 << 8);
	} else {
		cmd->response[0] = sdhci_read32(sdhci, SDHCI_RESPONSE_0);
	}
}

void sdhci_set_cmd_xfer_mode(struct sdhci *host, struct mci_cmd *cmd,
			     struct mci_data *data, bool dma, u32 *command,
			     u32 *xfer)
{
	*command = 0;
	*xfer = 0;

	if (!(cmd->resp_type & MMC_RSP_PRESENT))
		*command |= SDHCI_RESP_NONE;
	else if (cmd->resp_type & MMC_RSP_136)
		*command |= SDHCI_RESP_TYPE_136;
	else if (cmd->resp_type & MMC_RSP_BUSY)
		*command |= SDHCI_RESP_TYPE_48_BUSY;
	else
		*command |= SDHCI_RESP_TYPE_48;

	if (cmd->resp_type & MMC_RSP_CRC)
		*command |= SDHCI_CMD_CRC_CHECK_EN;
	if (cmd->resp_type & MMC_RSP_OPCODE)
		*command |= SDHCI_CMD_INDEX_CHECK_EN;

	*command |= SDHCI_CMD_INDEX(cmd->cmdidx);

	if (data) {
		*command |= SDHCI_DATA_PRESENT;

		*xfer |= SDHCI_BLOCK_COUNT_EN;

		if (data->blocks > 1)
			*xfer |= SDHCI_MULTIPLE_BLOCKS;

		if (data->flags & MMC_DATA_READ)
			*xfer |= SDHCI_DATA_TO_HOST;

		if (dma)
			*xfer |= SDHCI_DMA_EN;
	}
}

static void sdhci_rx_pio(struct sdhci *sdhci, struct mci_data *data,
			 unsigned int block)
{
	u32 *buf = (u32 *)data->dest;
	int i;

	buf += block * data->blocksize / sizeof(u32);

	for (i = 0; i < data->blocksize / sizeof(u32); i++)
		buf[i] = sdhci_read32(sdhci, SDHCI_BUFFER);
}

static void sdhci_tx_pio(struct sdhci *sdhci, struct mci_data *data,
			 unsigned int block)
{
	const u32 *buf = (const u32 *)data->src;
	int i;

	buf += block * data->blocksize / sizeof(u32);

	for (i = 0; i < data->blocksize / sizeof(u32); i++)
		sdhci_write32(sdhci, SDHCI_BUFFER, buf[i]);
}

int sdhci_transfer_data(struct sdhci *sdhci, struct mci_data *data)
{
	unsigned int block = 0;
	u32 stat, prs;
	uint64_t start = get_time_ns();

	do {
		stat = sdhci_read32(sdhci, SDHCI_INT_STATUS);
		if (stat & SDHCI_INT_ERROR)
			return -EIO;

		if (block >= data->blocks)
			continue;

		prs = sdhci_read32(sdhci, SDHCI_PRESENT_STATE);

		if (prs & SDHCI_BUFFER_READ_ENABLE &&
		    data->flags & MMC_DATA_READ) {
			sdhci_rx_pio(sdhci, data, block);
			block++;
			start = get_time_ns();
		}

		if (prs & SDHCI_BUFFER_WRITE_ENABLE &&
		    !(data->flags & MMC_DATA_READ)) {
			sdhci_tx_pio(sdhci, data, block);
			block++;
			start = get_time_ns();
		}

		if (is_timeout(start, 10 * SECOND))
			return -ETIMEDOUT;

	} while (!(stat & SDHCI_INT_XFER_COMPLETE));

	return 0;
}
