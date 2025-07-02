// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <io.h>
#include <mci.h>
#include <pbl.h>

#include "sdhci.h"
#include "imx-esdhc.h"

#define PRSSTAT_DAT0  0x01000000

#define	ESDHC_CTRL_D3CD			0x08

#define  SDHCI_CTRL_VDD_180         0x0008

#define ESDHC_MIX_CTRL			0x48
#define  ESDHC_MIX_CTRL_DDREN		(1 << 3)
#define  ESDHC_MIX_CTRL_AC23EN		(1 << 7)
#define  ESDHC_MIX_CTRL_EXE_TUNE	(1 << 22)
#define  ESDHC_MIX_CTRL_SMPCLK_SEL	(1 << 23)
#define  ESDHC_MIX_CTRL_AUTO_TUNE_EN	(1 << 24)
#define  ESDHC_MIX_CTRL_FBCLK_SEL	(1 << 25)
#define  ESDHC_MIX_CTRL_HS400_EN	(1 << 26)
#define  ESDHC_MIX_CTRL_HS400_ES_EN	(1 << 27)
/* Bits 3 and 6 are not SDHCI standard definitions */
#define  ESDHC_MIX_CTRL_SDHCI_MASK	0xb7
/* Tuning bits */
#define  ESDHC_MIX_CTRL_TUNING_MASK	0x03c00000

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

static u16 esdhc_op_read16_le_tuning(struct sdhci *sdhci, int reg)
{
	struct fsl_esdhc_host *host = sdhci_to_esdhc(sdhci);
	u16 ret = 0;
	u32 val;

	if (unlikely(reg == SDHCI_HOST_VERSION)) {
		/*
		 * The usdhc register returns a wrong host version.
		 * Correct it here.
		 */
		return SDHCI_SPEC_300;
	}

	if (unlikely(reg == SDHCI_HOST_CONTROL2)) {
		val = readl(sdhci->base + ESDHC_VENDOR_SPEC);
		if (val & ESDHC_VENDOR_SPEC_VSELECT)
			ret |= SDHCI_CTRL_VDD_180;

		if (host->socdata->flags & ESDHC_FLAG_MAN_TUNING)
			val = readl(sdhci->base + ESDHC_MIX_CTRL);
		else if (host->socdata->flags & ESDHC_FLAG_STD_TUNING)
			/* the std tuning bits is in ACMD12_ERR for imx6sl */
			val = readl(sdhci->base + SDHCI_ACMD12_ERR__HOST_CONTROL2);

		if (val & ESDHC_MIX_CTRL_EXE_TUNE)
			ret |= SDHCI_CTRL_EXEC_TUNING;
		if (val & ESDHC_MIX_CTRL_SMPCLK_SEL)
			ret |= SDHCI_CTRL_TUNED_CLK;

		ret &= ~SDHCI_CTRL_PRESET_VAL_ENABLE;

		return ret;
	}

	if (unlikely(reg == SDHCI_TRANSFER_MODE)) {
		u32 m = readl(sdhci->base + ESDHC_MIX_CTRL);
		ret = m & ESDHC_MIX_CTRL_SDHCI_MASK;
		/* Swap AC23 bit */
		if (m & ESDHC_MIX_CTRL_AC23EN) {
			ret &= ~ESDHC_MIX_CTRL_AC23EN;
			ret |= SDHCI_TRNS_AUTO_CMD23;
		}

		return ret;
	}

	return readw(sdhci->base + reg);
}

static inline void esdhc_wait_for_card_clock_gate_off(struct sdhci *sdhci)
{
	struct fsl_esdhc_host *host = sdhci_to_esdhc(sdhci);
	u32 present_state;
	int ret;

	ret = readl_poll_timeout(sdhci->base + ESDHC_PRSSTAT, present_state,
				(present_state & ESDHC_CLOCK_GATE_OFF), 100);
	if (ret == -ETIMEDOUT)
		dev_warn(host->dev, "%s: card clock still not gate off in 100us!.\n", __func__);
}

static inline void esdhc_clrset_le(struct sdhci *sdhci, u32 mask, u32 val, int reg)
{
	void __iomem *base = sdhci->base + (reg & ~0x3);
	u32 shift = (reg & 0x3) * 8;

	writel(((readl(base) & ~(mask << shift)) | (val << shift)), base);
}

static void esdhc_op_write16_le_tuning(struct sdhci *sdhci, int reg, u16 val)
{
	struct fsl_esdhc_host *host = sdhci_to_esdhc(sdhci);
	u32 new_val = 0;

	switch (reg) {
	case SDHCI_CLOCK_CONTROL:
		new_val = readl(sdhci->base + ESDHC_VENDOR_SPEC);
		if (val & SDHCI_CLOCK_CARD_EN)
			new_val |= ESDHC_VENDOR_SPEC_FRC_SDCLK_ON;
		else
			new_val &= ~ESDHC_VENDOR_SPEC_FRC_SDCLK_ON;
		writel(new_val, sdhci->base + ESDHC_VENDOR_SPEC);
		if (!(new_val & ESDHC_VENDOR_SPEC_FRC_SDCLK_ON))
			esdhc_wait_for_card_clock_gate_off(sdhci);
		return;
	case SDHCI_HOST_CONTROL2:
		new_val = readl(sdhci->base + ESDHC_VENDOR_SPEC);
		if (val & SDHCI_CTRL_VDD_180)
			new_val |= ESDHC_VENDOR_SPEC_VSELECT;
		else
			new_val &= ~ESDHC_VENDOR_SPEC_VSELECT;
		writel(new_val, sdhci->base + ESDHC_VENDOR_SPEC);
		if (host->socdata->flags & ESDHC_FLAG_STD_TUNING) {
			u32 v = readl(sdhci->base + SDHCI_ACMD12_ERR__HOST_CONTROL2);
			u32 m = readl(sdhci->base + ESDHC_MIX_CTRL);
			if (val & SDHCI_CTRL_TUNED_CLK) {
				v |= ESDHC_MIX_CTRL_SMPCLK_SEL;
			} else {
				v &= ~ESDHC_MIX_CTRL_SMPCLK_SEL;
				m &= ~ESDHC_MIX_CTRL_FBCLK_SEL;
			}

			if (val & SDHCI_CTRL_EXEC_TUNING) {
				v |= ESDHC_MIX_CTRL_EXE_TUNE;
				m |= ESDHC_MIX_CTRL_FBCLK_SEL;
			} else {
				v &= ~ESDHC_MIX_CTRL_EXE_TUNE;
			}

			writel(v, sdhci->base + SDHCI_ACMD12_ERR__HOST_CONTROL2);
			writel(m, sdhci->base + ESDHC_MIX_CTRL);
		}
		return;
	case SDHCI_COMMAND:
		if (host->last_cmd == MMC_CMD_STOP_TRANSMISSION)
			val |= SDHCI_COMMAND_CMDTYP_ABORT;

		writel(val << 16,
		       sdhci->base + SDHCI_TRANSFER_MODE);
		return;
	case SDHCI_BLOCK_SIZE:
		val &= ~SDHCI_MAKE_BLKSZ(0x7, 0);
		break;
	}
	esdhc_clrset_le(sdhci, 0xffff, val, reg);
}

static u8 esdhc_readb_le(struct sdhci *sdhci, int reg)
{
	u8 ret;
	u8 val;

	switch (reg) {
	case SDHCI_HOST_CONTROL:
		val = readl(sdhci->base + reg);

		ret = val & SDHCI_CTRL_LED;
		ret |= (val >> 5) & SDHCI_CTRL_DMA_MASK;
		ret |= (val & ESDHC_CTRL_4BITBUS);
		ret |= (val & ESDHC_CTRL_8BITBUS) << 3;
		return ret;
	}

	return readb(sdhci->base + reg);
}

static void esdhc_writeb_le(struct sdhci *sdhci, int reg, u8 val)
{
	u32 new_val = 0;
	u32 mask;

	switch (reg) {
	case SDHCI_POWER_CONTROL:
		/*
		 * FSL put some DMA bits here
		 * If your board has a regulator, code should be here
		 */
		return;
	case SDHCI_HOST_CONTROL:
		/* FSL messed up here, so we need to manually compose it. */
		new_val = val & SDHCI_CTRL_LED;
		/* ensure the endianness */
		new_val |= ESDHC_HOST_CONTROL_LE;
		/* DMA mode bits are shifted */
		new_val |= (val & SDHCI_CTRL_DMA_MASK) << 5;

		/*
		 * Do not touch buswidth bits here. This is done in
		 * esdhc_pltfm_bus_width.
		 * Do not touch the D3CD bit either which is used for the
		 * SDIO interrupt erratum workaround.
		 */
		mask = 0xffff & ~(ESDHC_CTRL_BUSWIDTH_MASK | ESDHC_CTRL_D3CD);

		esdhc_clrset_le(sdhci, mask, new_val, reg);
		return;
	case SDHCI_SOFTWARE_RESET:
		if (val & SDHCI_RESET_DATA)
			new_val = readl(sdhci->base + SDHCI_HOST_CONTROL);
		break;
	}
	esdhc_clrset_le(sdhci, 0xff, val, reg);

	if (reg == SDHCI_SOFTWARE_RESET) {
		if (val & SDHCI_RESET_ALL) {
			/*
			 * The esdhc has a design violation to SDHC spec which
			 * tells that software reset should not affect card
			 * detection circuit. But esdhc clears its SYSCTL
			 * register bits [0..2] during the software reset. This
			 * will stop those clocks that card detection circuit
			 * relies on. To work around it, we turn the clocks on
			 * back to keep card detection circuit functional.
			 */
			esdhc_clrset_le(sdhci, 0x7, 0x7, ESDHC_SYSTEM_CONTROL);
			/*
			 * The reset on usdhc fails to clear MIX_CTRL register.
			 * Do it manually here.
			 */
			/*
			 * the tuning bits should be kept during reset
			 */
			new_val = readl(sdhci->base + ESDHC_MIX_CTRL);
			writel(new_val & ESDHC_MIX_CTRL_TUNING_MASK,
					sdhci->base + ESDHC_MIX_CTRL);
		} else if (val & SDHCI_RESET_DATA) {
			/*
			 * The eSDHC DAT line software reset clears at least the
			 * data transfer width on i.MX25, so make sure that the
			 * Host Control register is unaffected.
			 */
			esdhc_clrset_le(sdhci, 0xff, new_val,
					SDHCI_HOST_CONTROL);
		}
	}
}

void esdhc_populate_sdhci(struct fsl_esdhc_host *host)
{
	if (host->socdata->flags & ESDHC_FLAG_BIGENDIAN) {
		host->sdhci.read16 = esdhc_op_read16_be;
		host->sdhci.write16 = esdhc_op_write16_be;
		host->sdhci.read32 = esdhc_op_read32_be;
		host->sdhci.write32 = esdhc_op_write32_be;
	} else if (esdhc_is_usdhc(host)){
		host->sdhci.read8 = esdhc_readb_le;
		host->sdhci.write8 = esdhc_writeb_le;
		host->sdhci.read16 = esdhc_op_read16_le_tuning;
		host->sdhci.write16 = esdhc_op_write16_le_tuning;
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
	u32	val, irqflags, irqstat;
	dma_addr_t dma = SDHCI_NO_DMA;
	int ret;

	host->last_cmd = cmd ? cmd->cmdidx : 0;

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

	irqflags = SDHCI_INT_CMD_COMPLETE;
	if (mmc_op_tuning(cmd->cmdidx))
		irqflags = SDHCI_INT_DATA_AVAIL;

	/* Wait for the command to complete */
	ret = esdhc_poll(host, SDHCI_INT_STATUS, irqstat,
			 irqstat & irqflags,
			 100 * MSECOND);
	if (ret) {
		dev_dbg(host->dev, "timeout 1\n");
		goto undo_setup_data;
	}

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
				 max_t(u64, sdhci_compute_timeout(cmd, NULL), 2500 * MSECOND));
		if (ret) {
			dev_err(host->dev, "timeout PRSSTAT_DAT0\n");
			goto undo_setup_data;
		}
	}

	sdhci_read_response(&host->sdhci, cmd);

	/* Wait until all of the blocks are transferred */
	if (data) {
		if (esdhc_use_pio_mode())
			ret = sdhci_transfer_data_pio(&host->sdhci, cmd, data);
		else
			ret = sdhci_transfer_data_dma(&host->sdhci, cmd, data, dma);

		if (ret)
			return ret;
	}

	sdhci_write32(&host->sdhci, SDHCI_INT_STATUS, -1);

	/* Wait for the bus to be idle */
	ret = esdhc_poll(host, SDHCI_PRESENT_STATE, val,
			 (val & (SDHCI_CMD_INHIBIT_CMD | SDHCI_CMD_INHIBIT_DATA)) == 0,
			 max_t(u64, sdhci_compute_timeout(cmd, data), SECOND));
	if (ret) {
		dev_err(host->dev, "timeout 2\n");
		return -ETIMEDOUT;
	}

	ret = esdhc_poll(host, SDHCI_PRESENT_STATE, val,
			 (val & SDHCI_DATA_LINE_ACTIVE) == 0,
			 max_t(u64, sdhci_compute_timeout(cmd, NULL), 100 * MSECOND));
	if (ret) {
		dev_err(host->dev, "timeout 3\n");
		return -ETIMEDOUT;
	}

	return 0;
undo_setup_data:
	sdhci_teardown_data(&host->sdhci, data, dma);
	return ret;
}

