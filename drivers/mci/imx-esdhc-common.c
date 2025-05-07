// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <io.h>
#include <mci.h>
#include <pbl.h>

#include "sdhci.h"
#include "imx-esdhc.h"

#define PRSSTAT_DAT0  0x01000000

static u32 esdhc_op_read32_be(struct sdhci *sdhci, int reg)
{
	struct fsl_esdhc_host *host = sdhci_to_esdhc(sdhci);

	return in_be32(host->sdhci.base + reg);
}

static void esdhc_op_write32_be(struct sdhci *sdhci, int reg, u32 val)
{
	struct fsl_esdhc_host *host = sdhci_to_esdhc(sdhci);

	out_be32(host->sdhci.base + reg, val);
}

static u16 esdhc_op_read16_be(struct sdhci *sdhci, int reg)
{
	struct fsl_esdhc_host *host = sdhci_to_esdhc(sdhci);

	return in_be16(host->sdhci.base + reg);
}

static void esdhc_op_write16_be(struct sdhci *sdhci, int reg, u16 val)
{
	struct fsl_esdhc_host *host = sdhci_to_esdhc(sdhci);

	out_be16(host->sdhci.base + reg, val);
}

void esdhc_populate_sdhci(struct fsl_esdhc_host *host)
{
	if (host->socdata->flags & ESDHC_FLAG_BIGENDIAN) {
		host->sdhci.read16 = esdhc_op_read16_be;
		host->sdhci.write16 = esdhc_op_write16_be;
		host->sdhci.read32 = esdhc_op_read32_be;
		host->sdhci.write32 = esdhc_op_write32_be;
	}
}

static bool esdhc_use_pio_mode(void)
{
	return IN_PBL || IS_ENABLED(CONFIG_MCI_IMX_ESDHC_PIO);
}

static int esdhc_setup_data(struct fsl_esdhc_host *host, struct mci_data *data,
			    dma_addr_t *dma)
{
	u32 wml_value;

	wml_value = data->blocksize / 4;
	if (wml_value > 0x80)
		wml_value = 0x80;

	if (data->flags & MMC_DATA_READ)
		esdhc_clrsetbits32(host, IMX_SDHCI_WML, WML_RD_WML_MASK, wml_value);
	else
		esdhc_clrsetbits32(host, IMX_SDHCI_WML, WML_WR_WML_MASK,
					wml_value << 16);

	host->sdhci.sdma_boundary = 0;

	if (esdhc_use_pio_mode())
		sdhci_setup_data_pio(&host->sdhci, data);
	else
		sdhci_setup_data_dma(&host->sdhci, data, dma);

	return 0;
}

int __esdhc_send_cmd(struct fsl_esdhc_host *host, struct mci_cmd *cmd,
		     struct mci_data *data)
{
	u32	xfertyp, mixctrl, command;
	u32	val, irqstat;
	dma_addr_t dma = SDHCI_NO_DMA;
	int ret;

	sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, -1);

	/* Wait at least 8 SD clock cycles before the next command */
	udelay(1);

	/* Set up for a data transfer if we have one */
	if (data) {
		ret = esdhc_setup_data(host, data, &dma);
		if (ret)
			return ret;
	}

	sdhci_set_cmd_xfer_mode(&host->sdhci, cmd, data,
				dma != SDHCI_NO_DMA, &command, &xfertyp);

	if ((host->socdata->flags & ESDHC_FLAG_MULTIBLK_NO_INT) &&
	    (cmd->cmdidx == MMC_CMD_STOP_TRANSMISSION))
		command |= SDHCI_COMMAND_CMDTYP_ABORT;

	/* Send the command */
	sdhci_write32(&host->sdhci, SDHCI_ARGUMENT, cmd->cmdarg);

	if (esdhc_is_usdhc(host)) {
		/* write lower-half of xfertyp to mixctrl */
		mixctrl = xfertyp;
		/* Keep the bits 22-25 of the register as is */
		mixctrl |= (sdhci_read32(&host->sdhci, IMX_SDHCI_MIXCTRL) & (0xF << 22));
		mixctrl |= mci_timing_is_ddr(host->sdhci.timing) ? MIX_CTRL_DDREN : 0;
		sdhci_write32(&host->sdhci, IMX_SDHCI_MIXCTRL, mixctrl);
	}

	sdhci_write32(&host->sdhci, SDHCI_TRANSFER_MODE__COMMAND,
		      command << 16 | xfertyp);

	/* Wait for the command to complete */
	ret = esdhc_poll(host, SDHCI_INT_STATUS, val,
			 val & SDHCI_INT_CMD_COMPLETE,
			 100 * MSECOND);
	if (ret) {
		dev_dbg(host->dev, "timeout 1\n");
		goto undo_setup_data;
	}

	irqstat = sdhci_read32(&host->sdhci, SDHCI_INT_STATUS);

	if (irqstat & CMD_ERR) {
		ret = -EIO;
		goto undo_setup_data;
	}

	if (irqstat & SDHCI_INT_TIMEOUT) {
		ret = -ETIMEDOUT;
		goto undo_setup_data;
	}

	/* Workaround for ESDHC errata ENGcm03648 / ENGcm12360 */
	if (!data && (cmd->resp_type & MMC_RSP_BUSY)) {
		/*
		 * Poll on DATA0 line for cmd with busy signal for
		 * timout / 10 usec since DLA polling can be insecure.
		 */
		ret = esdhc_poll(host, SDHCI_PRESENT_STATE, val,
				 val & PRSSTAT_DAT0,
				 sdhci_compute_timeout(cmd, NULL, 2500 * MSECOND));
		if (ret) {
			dev_err(host->dev, "timeout PRSSTAT_DAT0\n");
			goto undo_setup_data;
		}
	}

	sdhci_read_response(&host->sdhci, cmd);

	/* Wait until all of the blocks are transferred */
	if (data) {
		if (esdhc_use_pio_mode())
			ret = sdhci_transfer_data_pio(&host->sdhci, data);
		else
			ret = sdhci_transfer_data_dma(&host->sdhci, data, dma);

		if (ret)
			return ret;
	}

	sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, -1);

	/* Wait for the bus to be idle */
	ret = esdhc_poll(host, SDHCI_PRESENT_STATE, val,
			 (val & (SDHCI_CMD_INHIBIT_CMD | SDHCI_CMD_INHIBIT_DATA)) == 0,
			 sdhci_compute_timeout(cmd, data, SECOND));
	if (ret) {
		dev_err(host->dev, "timeout 2\n");
		return -ETIMEDOUT;
	}

	ret = esdhc_poll(host, SDHCI_PRESENT_STATE, val,
			 (val & SDHCI_DATA_LINE_ACTIVE) == 0,
			 sdhci_compute_timeout(cmd, NULL, 100 * MSECOND));
	if (ret) {
		dev_err(host->dev, "timeout 3\n");
		return -ETIMEDOUT;
	}

	return 0;
undo_setup_data:
	sdhci_teardown_data(&host->sdhci, data, dma);
	return ret;
}

