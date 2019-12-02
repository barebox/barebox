// SPDX-License-Identifier: GPL-2.0

#include <common.h>
#include <io.h>
#include <mci.h>
#include <pbl.h>

#include "sdhci.h"
#include "imx-esdhc.h"

#define PRSSTAT_DAT0  0x01000000

struct fsl_esdhc_dma_transfer {
	dma_addr_t dma;
	unsigned int size;
	enum dma_data_direction dir;
};

static u32 esdhc_op_read32_le(struct sdhci *sdhci, int reg)
{
	struct fsl_esdhc_host *host = sdhci_to_esdhc(sdhci);

	return readl(host->regs + reg);
}

static u32 esdhc_op_read32_be(struct sdhci *sdhci, int reg)
{
	struct fsl_esdhc_host *host = sdhci_to_esdhc(sdhci);

	return in_be32(host->regs + reg);
}

static void esdhc_op_write32_le(struct sdhci *sdhci, int reg, u32 val)
{
	struct fsl_esdhc_host *host = sdhci_to_esdhc(sdhci);

	writel(val, host->regs + reg);
}

static void esdhc_op_write32_be(struct sdhci *sdhci, int reg, u32 val)
{
	struct fsl_esdhc_host *host = sdhci_to_esdhc(sdhci);

	out_be32(host->regs + reg, val);
}

void esdhc_populate_sdhci(struct fsl_esdhc_host *host)
{
	if (host->socdata->flags & ESDHC_FLAG_BIGENDIAN) {
		host->sdhci.read32 = esdhc_op_read32_be;
		host->sdhci.write32 = esdhc_op_write32_be;
	} else {
		host->sdhci.read32 = esdhc_op_read32_le;
		host->sdhci.write32 = esdhc_op_write32_le;
	}
}

static bool esdhc_use_pio_mode(void)
{
	return IN_PBL || IS_ENABLED(CONFIG_MCI_IMX_ESDHC_PIO);
}
static int esdhc_setup_data(struct fsl_esdhc_host *host, struct mci_data *data,
			    struct fsl_esdhc_dma_transfer *tr)
{
	u32 wml_value;
	void *ptr;

	if (!esdhc_use_pio_mode()) {
		wml_value = data->blocksize/4;

		if (data->flags & MMC_DATA_READ) {
			if (wml_value > 0x10)
				wml_value = 0x10;

			esdhc_clrsetbits32(host, IMX_SDHCI_WML, WML_RD_WML_MASK, wml_value);
		} else {
			if (wml_value > 0x80)
				wml_value = 0x80;

			esdhc_clrsetbits32(host, IMX_SDHCI_WML, WML_WR_WML_MASK,
						wml_value << 16);
		}

		tr->size = data->blocks * data->blocksize;

		if (data->flags & MMC_DATA_WRITE) {
			ptr = (void *)data->src;
			tr->dir = DMA_TO_DEVICE;
		} else {
			ptr = data->dest;
			tr->dir = DMA_FROM_DEVICE;
		}

		tr->dma = dma_map_single(host->dev, ptr, tr->size, tr->dir);
		if (dma_mapping_error(host->dev, tr->dma))
			return -EFAULT;


		sdhci_write32(&host->sdhci, SDHCI_DMA_ADDRESS, tr->dma);
	}

	sdhci_write32(&host->sdhci, SDHCI_BLOCK_SIZE__BLOCK_COUNT, data->blocks << 16 | data->blocksize);

	return 0;
}

static int esdhc_do_data(struct fsl_esdhc_host *host, struct mci_data *data,
		  struct fsl_esdhc_dma_transfer *tr)
{
	u32 irqstat;

	if (esdhc_use_pio_mode())
		return sdhci_transfer_data(&host->sdhci, data);

	do {
		irqstat = sdhci_read32(&host->sdhci, SDHCI_INT_STATUS);

		if (irqstat & DATA_ERR)
			return -EIO;

		if (irqstat & SDHCI_INT_DATA_TIMEOUT)
			return -ETIMEDOUT;
	} while (!(irqstat & SDHCI_INT_XFER_COMPLETE) &&
		(sdhci_read32(&host->sdhci, SDHCI_PRESENT_STATE) & SDHCI_DATA_LINE_ACTIVE));

	dma_unmap_single(host->dev, tr->dma, tr->size, tr->dir);

	return 0;
}

static bool esdhc_match32(struct fsl_esdhc_host *host, unsigned int off,
			  unsigned int mask, unsigned int val)
{
	const unsigned int reg = sdhci_read32(&host->sdhci, off) & mask;

	return reg == val;
}

#ifdef __PBL__
/*
 * Stubs to make timeout logic below work in PBL
 */

#define get_time_ns()		0
/*
 * Use time in us (approx) as a busy counter timeout value
 */
#define is_timeout(s, t)	((s)++ > ((t) / 1024))

static void __udelay(int us)
{
	volatile int i;

	for (i = 0; i < us * 4; i++);
}

#define udelay(n)	__udelay(n)
#undef  dev_err
#define dev_err(d, ...)	pr_err(__VA_ARGS__)

#endif

int esdhc_poll(struct fsl_esdhc_host *host, unsigned int off,
	       unsigned int mask, unsigned int val,
	       uint64_t timeout)
{
	return wait_on_timeout(timeout,
			       esdhc_match32(host, off, mask, val));
}

int __esdhc_send_cmd(struct fsl_esdhc_host *host, struct mci_cmd *cmd,
		     struct mci_data *data)
{
	u32	xfertyp, mixctrl, command;
	u32	irqstat;
	struct fsl_esdhc_dma_transfer tr = { 0 };
	int ret;

	sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, -1);

	/* Wait at least 8 SD clock cycles before the next command */
	udelay(1);

	/* Set up for a data transfer if we have one */
	if (data) {
		ret = esdhc_setup_data(host, data, &tr);
		if (ret)
			return ret;
	}

	sdhci_set_cmd_xfer_mode(&host->sdhci, cmd, data,
				!esdhc_use_pio_mode(), &command, &xfertyp);

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
		sdhci_write32(&host->sdhci, IMX_SDHCI_MIXCTRL, mixctrl);
	}

	sdhci_write32(&host->sdhci, SDHCI_TRANSFER_MODE__COMMAND,
		      command << 16 | xfertyp);

	/* Wait for the command to complete */
	ret = esdhc_poll(host, SDHCI_INT_STATUS,
			 SDHCI_INT_CMD_COMPLETE, SDHCI_INT_CMD_COMPLETE,
			 100 * MSECOND);
	if (ret) {
		dev_err(host->dev, "timeout 1\n");
		return -ETIMEDOUT;
	}

	irqstat = sdhci_read32(&host->sdhci, SDHCI_INT_STATUS);
	sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, irqstat);

	if (irqstat & CMD_ERR)
		return -EIO;

	if (irqstat & SDHCI_INT_TIMEOUT)
		return -ETIMEDOUT;

	/* Workaround for ESDHC errata ENGcm03648 / ENGcm12360 */
	if (!data && (cmd->resp_type & MMC_RSP_BUSY)) {
		/*
		 * Poll on DATA0 line for cmd with busy signal for
		 * timout / 10 usec since DLA polling can be insecure.
		 */
		ret = esdhc_poll(host, SDHCI_PRESENT_STATE,
				 PRSSTAT_DAT0, PRSSTAT_DAT0,
				 2500 * MSECOND);
		if (ret) {
			dev_err(host->dev, "timeout PRSSTAT_DAT0\n");
			return -ETIMEDOUT;
		}
	}

	sdhci_read_response(&host->sdhci, cmd);

	/* Wait until all of the blocks are transferred */
	if (data) {
		ret = esdhc_do_data(host, data, &tr);
		if (ret)
			return ret;
	}

	sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, -1);

	/* Wait for the bus to be idle */
	ret = esdhc_poll(host, SDHCI_PRESENT_STATE,
			 SDHCI_CMD_INHIBIT_CMD | SDHCI_CMD_INHIBIT_DATA, 0,
			 SECOND);
	if (ret) {
		dev_err(host->dev, "timeout 2\n");
		return -ETIMEDOUT;
	}

	ret = esdhc_poll(host, SDHCI_PRESENT_STATE,
			 SDHCI_DATA_LINE_ACTIVE, 0,
			 100 * MSECOND);
	if (ret) {
		dev_err(host->dev, "timeout 3\n");
		return -ETIMEDOUT;
	}

	return 0;
}

