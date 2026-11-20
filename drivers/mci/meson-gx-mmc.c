// SPDX-License-Identifier: GPL-2.0-only
// SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/mmc/host/meson-gx-mmc.c?id=a5dbd6e7ed29507681dbab2cc2d01c6df104a02a
/*
 * Amlogic SD/eMMC driver for the GX/S905 family SoCs
 *
 * Copyright (c) 2016 BayLibre, SAS.
 * Author: Kevin Hilman <khilman@baylibre.com>
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <init.h>
#include <clock.h>
#include <linux/device.h>
#include <linux/iopoll.h>
#include <of.h>
#include <of_device.h>
#include <driver.h>
#include <linux/ioport.h>
#include <dma.h>
#include <mci.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/reset.h>
#include <linux/bitfield.h>
#include <linux/log2.h>
#include <linux/sprintf.h>
#include <linux/pinctrl/consumer.h>

#define DRIVER_NAME "meson-gx-mmc"

#define SD_EMMC_CLOCK 0x0
#define   CLK_DIV_MASK GENMASK(5, 0)
#define   CLK_SRC_MASK GENMASK(7, 6)
#define   CLK_CORE_PHASE_MASK GENMASK(9, 8)
#define   CLK_TX_PHASE_MASK GENMASK(11, 10)
#define   CLK_RX_PHASE_MASK GENMASK(13, 12)
#define   CLK_PHASE_0 0
#define   CLK_PHASE_180 2
#define   CLK_V2_TX_DELAY_MASK GENMASK(19, 16)
#define   CLK_V2_RX_DELAY_MASK GENMASK(23, 20)
#define   CLK_V2_ALWAYS_ON BIT(24)
#define   CLK_V2_IRQ_SDIO_SLEEP BIT(25)

#define   CLK_V3_TX_DELAY_MASK GENMASK(21, 16)
#define   CLK_V3_RX_DELAY_MASK GENMASK(27, 22)
#define   CLK_V3_ALWAYS_ON BIT(28)
#define   CLK_V3_IRQ_SDIO_SLEEP BIT(29)

#define   CLK_TX_DELAY_MASK(h)		(h->data->tx_delay_mask)
#define   CLK_RX_DELAY_MASK(h)		(h->data->rx_delay_mask)
#define   CLK_ALWAYS_ON(h)		(h->data->always_on)
#define   CLK_IRQ_SDIO_SLEEP(h)		(h->data->irq_sdio_sleep)

#define SD_EMMC_DELAY 0x4
#define SD_EMMC_ADJUST 0x8
#define   ADJUST_ADJ_DELAY_MASK GENMASK(21, 16)
#define   ADJUST_DS_EN BIT(15)
#define   ADJUST_ADJ_EN BIT(13)

#define SD_EMMC_DELAY1 0x4
#define SD_EMMC_DELAY2 0x8
#define SD_EMMC_V3_ADJUST 0xc

#define SD_EMMC_CALOUT 0x10
#define SD_EMMC_START 0x40
#define   START_DESC_INIT BIT(0)
#define   START_DESC_BUSY BIT(1)
#define   START_DESC_ADDR_MASK GENMASK(31, 2)

#define SD_EMMC_CFG 0x44
#define   CFG_BUS_WIDTH_MASK GENMASK(1, 0)
#define   CFG_BUS_WIDTH_1 0x0
#define   CFG_BUS_WIDTH_4 0x1
#define   CFG_BUS_WIDTH_8 0x2
#define   CFG_DDR BIT(2)
#define   CFG_BLK_LEN_MASK GENMASK(7, 4)
#define   CFG_RESP_TIMEOUT_MASK GENMASK(11, 8)
#define   CFG_RC_CC_MASK GENMASK(15, 12)
#define   CFG_STOP_CLOCK BIT(22)
#define   CFG_CLK_ALWAYS_ON BIT(18)
#define   CFG_CHK_DS BIT(20)
#define   CFG_AUTO_CLK BIT(23)
#define   CFG_ERR_ABORT BIT(27)

#define SD_EMMC_STATUS 0x48
#define   STATUS_BUSY BIT(31)
#define   STATUS_DESC_BUSY BIT(30)
#define   STATUS_DATI GENMASK(23, 16)

#define SD_EMMC_IRQ_EN 0x4c
#define   IRQ_RXD_ERR_MASK GENMASK(7, 0)
#define   IRQ_TXD_ERR BIT(8)
#define   IRQ_DESC_ERR BIT(9)
#define   IRQ_RESP_ERR BIT(10)
#define   IRQ_CRC_ERR \
	(IRQ_RXD_ERR_MASK | IRQ_TXD_ERR | IRQ_DESC_ERR | IRQ_RESP_ERR)
#define   IRQ_RESP_TIMEOUT BIT(11)
#define   IRQ_DESC_TIMEOUT BIT(12)
#define   IRQ_TIMEOUTS \
	(IRQ_RESP_TIMEOUT | IRQ_DESC_TIMEOUT)
#define   IRQ_END_OF_CHAIN BIT(13)
#define   IRQ_RESP_STATUS BIT(14)
#define   IRQ_SDIO BIT(15)
#define   IRQ_EN_MASK \
	(IRQ_CRC_ERR | IRQ_TIMEOUTS | IRQ_END_OF_CHAIN)

#define SD_EMMC_CMD_CFG 0x50
#define SD_EMMC_CMD_ARG 0x54
#define SD_EMMC_CMD_DAT 0x58
#define SD_EMMC_CMD_RSP 0x5c
#define SD_EMMC_CMD_RSP1 0x60
#define SD_EMMC_CMD_RSP2 0x64
#define SD_EMMC_CMD_RSP3 0x68

#define SD_EMMC_RXD 0x94
#define SD_EMMC_TXD 0x94
#define SD_EMMC_LAST_REG SD_EMMC_TXD

#define SD_EMMC_SRAM_DATA_BUF_LEN 1536
#define SD_EMMC_SRAM_DATA_BUF_OFF 0x200

#define SD_EMMC_CFG_BLK_SIZE 512 /* internal buffer max: 512 bytes */
#define SD_EMMC_CFG_RESP_TIMEOUT 256 /* in clock cycles */
#define SD_EMMC_CMD_TIMEOUT 1024 /* in ms */
#define SD_EMMC_CMD_TIMEOUT_DATA 4096 /* in ms */
#define SD_EMMC_CFG_CMD_GAP 16 /* in clock cycles */

#define SD_EMMC_DESC_CHAIN_MODE BIT(1)

#define MUX_CLK_NUM_PARENTS 2

struct meson_mmc_data {
	unsigned int tx_delay_mask;
	unsigned int rx_delay_mask;
	unsigned int always_on;
	unsigned int adjust;
	unsigned int irq_sdio_sleep;
	bool has_pipeline_clk;
	bool resample_mmc_hs;
};

struct meson_host {
	struct	device		*dev;
	const struct meson_mmc_data *data;
	struct	mci_host	*mmc;
	struct	mci_cmd	*cmd;

	void __iomem *regs;
	struct clk *mux_clk;
	struct clk *mmc_clk;
	unsigned long req_rate;
	bool ddr;

	bool dram_access_quirk;

	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_clk_gate;

	unsigned int bounce_buf_size;
	void *bounce_buf;
	void __iomem *bounce_iomem_buf;
	dma_addr_t bounce_dma_addr;

	dma_addr_t direct_dma_addr;

	struct mci_host mci;
};

#define mmc_priv(mmc) container_of(mmc, struct meson_host, mci)
#define mmc_dev(mmc) ((mmc)->hw_dev)

#define CMD_CFG_LENGTH_MASK GENMASK(8, 0)
#define CMD_CFG_BLOCK_MODE BIT(9)
#define CMD_CFG_R1B BIT(10)
#define CMD_CFG_END_OF_CHAIN BIT(11)
#define CMD_CFG_TIMEOUT_MASK GENMASK(15, 12)
#define CMD_CFG_NO_RESP BIT(16)
#define CMD_CFG_NO_CMD BIT(17)
#define CMD_CFG_DATA_IO BIT(18)
#define CMD_CFG_DATA_WR BIT(19)
#define CMD_CFG_RESP_NOCRC BIT(20)
#define CMD_CFG_RESP_128 BIT(21)
#define CMD_CFG_RESP_NUM BIT(22)
#define CMD_CFG_DATA_NUM BIT(23)
#define CMD_CFG_CMD_INDEX_MASK GENMASK(29, 24)
#define CMD_CFG_ERROR BIT(30)
#define CMD_CFG_OWNER BIT(31)

#define CMD_DATA_MASK GENMASK(31, 2)
#define CMD_DATA_BIG_ENDIAN BIT(1)
#define CMD_DATA_SRAM BIT(0)
#define CMD_RESP_MASK GENMASK(31, 1)
#define CMD_RESP_SRAM BIT(0)

static unsigned int meson_mmc_get_timeout_msecs(struct mci_data *data)
{
	unsigned int timeout = data->timeout_ns / NSEC_PER_MSEC;

	if (!timeout)
		return SD_EMMC_CMD_TIMEOUT_DATA;

	timeout = roundup_pow_of_two(timeout);

	return min(timeout, 32768U); /* max. 2^15 ms */
}

static void meson_mmc_get_transfer_mode(struct mci_host *mmc,
					struct mci_data *data)
{
	struct meson_host *host = mmc_priv(mmc);

	/*
	 * When Controller DMA cannot directly access DDR memory, disable
	 * support for Chain Mode to directly use the internal SRAM using
	 * the bounce buffer mode.
	 */
	if (host->dram_access_quirk)
		return;

	/* dma_map_single() maintains the caches by whole lines */
	if (!IS_ALIGNED((uintptr_t)data->dest, DMA_ALIGNMENT) ||
	    !IS_ALIGNED(data->blocksize * data->blocks, DMA_ALIGNMENT))
		return;

	data->host_cookie |= SD_EMMC_DESC_CHAIN_MODE;
}

static inline bool meson_mmc_desc_chain_mode(const struct mci_data *data)
{
	return data->host_cookie & SD_EMMC_DESC_CHAIN_MODE;
}

static inline bool meson_mmc_bounce_buf_read(struct meson_host *host,
					     const struct mci_data *data)
{
	return data && data->flags & MMC_DATA_READ &&
	       !meson_mmc_desc_chain_mode(data);
}

static enum dma_data_direction mmc_get_dma_dir(struct mci_data *data)
{
	return data->flags & MMC_DATA_WRITE ? DMA_TO_DEVICE : DMA_FROM_DEVICE;
}

static void meson_mmc_pre_req(struct mci_host *mmc, struct mci_data *data)
{
	struct meson_host *host = mmc_priv(mmc);

	meson_mmc_get_transfer_mode(mmc, data);
	if (!meson_mmc_desc_chain_mode(data))
		return;

	host->direct_dma_addr = dma_map_single(mmc_dev(mmc), data->dest,
					       data->blocksize * data->blocks,
					       mmc_get_dma_dir(data));
	if (dma_mapping_error(mmc_dev(mmc), host->direct_dma_addr))
		data->host_cookie &= ~SD_EMMC_DESC_CHAIN_MODE;
}

static void meson_mmc_post_req(struct mci_host *mmc, struct mci_data *data)
{
	struct meson_host *host = mmc_priv(mmc);

	if (meson_mmc_desc_chain_mode(data))
		dma_unmap_single(mmc_dev(mmc), host->direct_dma_addr,
				 data->blocksize * data->blocks,
				 mmc_get_dma_dir(data));
}

/*
 * Gating the clock on this controller is tricky.  It seems the mmc clock
 * is also used by the controller.  It may crash during some operation if the
 * clock is stopped.  The safest thing to do, whenever possible, is to keep
 * clock running at stop it at the pad using the pinmux.
 */
static void meson_mmc_clk_gate(struct meson_host *host)
{
	u32 cfg;

	if (host->pins_clk_gate) {
		pinctrl_select_state(host->pinctrl, host->pins_clk_gate);
	} else {
		/*
		 * If the pinmux is not provided - default to the classic and
		 * unsafe method
		 */
		cfg = readl(host->regs + SD_EMMC_CFG);
		cfg |= CFG_STOP_CLOCK;
		writel(cfg, host->regs + SD_EMMC_CFG);
	}
}

static void meson_mmc_clk_ungate(struct meson_host *host)
{
	u32 cfg;

	if (host->pins_clk_gate)
		pinctrl_select_state_default(host->dev);

	/* Make sure the clock is not stopped in the controller */
	cfg = readl(host->regs + SD_EMMC_CFG);
	cfg &= ~CFG_STOP_CLOCK;
	writel(cfg, host->regs + SD_EMMC_CFG);
}

static int meson_mmc_clk_set(struct meson_host *host, unsigned long rate,
			     bool ddr)
{
	struct mci_host *mmc = host->mmc;
	int ret;
	u32 cfg;

	/* Same request - bail-out */
	if (host->ddr == ddr && host->req_rate == rate)
		return 0;

	/* stop clock */
	meson_mmc_clk_gate(host);
	host->req_rate = 0;
	mmc->actual_clock = 0;

	/* return with clock being stopped */
	if (!rate)
		return 0;

	/* Stop the clock during rate change to avoid glitches */
	cfg = readl(host->regs + SD_EMMC_CFG);
	cfg |= CFG_STOP_CLOCK;
	writel(cfg, host->regs + SD_EMMC_CFG);

	if (ddr) {
		/* DDR modes require higher module clock */
		rate <<= 1;
		cfg |= CFG_DDR;
	} else {
		cfg &= ~CFG_DDR;
	}
	writel(cfg, host->regs + SD_EMMC_CFG);
	host->ddr = ddr;

	ret = clk_set_rate(host->mmc_clk, rate);
	if (ret) {
		dev_err(host->dev, "Unable to set cfg_div_clk to %lu. ret=%d\n",
			rate, ret);
		return ret;
	}

	host->req_rate = rate;
	mmc->actual_clock = clk_get_rate(host->mmc_clk);

	/* We should report the real output frequency of the controller */
	if (ddr) {
		host->req_rate >>= 1;
		mmc->actual_clock >>= 1;
	}

	dev_dbg(host->dev, "clk rate: %u Hz\n", mmc->actual_clock);
	if (rate != mmc->actual_clock)
		dev_dbg(host->dev, "requested rate was %lu\n", rate);

	/* (re)start clock */
	meson_mmc_clk_ungate(host);

	return 0;
}

/*
 * The SD/eMMC IP block has an internal mux and divider used for
 * generating the MMC clock.  Use the clock framework to create and
 * manage these clocks.
 */
static int meson_mmc_clk_init(struct meson_host *host)
{
	struct clk_init_data init = {};
	struct clk_mux *mux;
	struct clk_divider *div;
	char clk_name[32];
	int i, ret = 0;
	const char *mux_parent_names[MUX_CLK_NUM_PARENTS];
	const char *clk_parent[1];
	u32 clk_reg;

	/* init SD_EMMC_CLOCK to sane defaults w/min clock rate */
	clk_reg = CLK_ALWAYS_ON(host);
	clk_reg |= CLK_DIV_MASK;
	clk_reg |= FIELD_PREP(CLK_CORE_PHASE_MASK, CLK_PHASE_180);
	clk_reg |= FIELD_PREP(CLK_TX_PHASE_MASK, CLK_PHASE_0);
	clk_reg |= FIELD_PREP(CLK_RX_PHASE_MASK, CLK_PHASE_0);
	writel(clk_reg, host->regs + SD_EMMC_CLOCK);

	/* get the mux parents */
	for (i = 0; i < MUX_CLK_NUM_PARENTS; i++) {
		struct clk *clk;
		char name[16];

		snprintf(name, sizeof(name), "clkin%d", i);
		clk = clk_get(host->dev, name);
		if (IS_ERR(clk))
			return dev_err_probe(host->dev, PTR_ERR(clk),
					     "Missing clock %s\n", name);

		mux_parent_names[i] = __clk_get_name(clk);
	}

	/* create the mux */
	mux = devm_kzalloc(host->dev, sizeof(*mux), GFP_KERNEL);
	if (!mux)
		return -ENOMEM;

	snprintf(clk_name, sizeof(clk_name), "%s#mux", dev_name(host->dev));
	init.name = clk_name;
	init.ops = &clk_mux_ops;
	init.flags = 0;
	init.parent_names = mux_parent_names;
	init.num_parents = MUX_CLK_NUM_PARENTS;

	mux->reg = host->regs + SD_EMMC_CLOCK;
	mux->shift = __ffs(CLK_SRC_MASK);
	mux->mask = CLK_SRC_MASK >> mux->shift;
	mux->hw.init = &init;

	host->mux_clk = clk_register(host->dev, &mux->hw);
	if (WARN_ON(IS_ERR(host->mux_clk)))
		return PTR_ERR(host->mux_clk);

	/* create the divider */
	div = devm_kzalloc(host->dev, sizeof(*div), GFP_KERNEL);
	if (!div)
		return -ENOMEM;

	snprintf(clk_name, sizeof(clk_name), "%s#div", dev_name(host->dev));
	init.name = clk_name;
	init.ops = &clk_divider_ops;
	init.flags = CLK_SET_RATE_PARENT;
	clk_parent[0] = __clk_get_name(host->mux_clk);
	init.parent_names = clk_parent;
	init.num_parents = 1;

	div->reg = host->regs + SD_EMMC_CLOCK;
	div->shift = __ffs(CLK_DIV_MASK);
	div->width = __builtin_popcountl(CLK_DIV_MASK);
	div->hw.init = &init;
	div->flags = CLK_DIVIDER_ONE_BASED;

	host->mmc_clk = clk_register(host->dev, &div->hw);
	if (WARN_ON(IS_ERR(host->mmc_clk)))
		return PTR_ERR(host->mmc_clk);

	/* init SD_EMMC_CLOCK to sane defaults w/min clock rate */
	host->mmc->f_min = clk_round_rate(host->mmc_clk, 400000);
	ret = clk_set_rate(host->mmc_clk, host->mmc->f_min);
	if (ret)
		return ret;

	return clk_prepare_enable(host->mmc_clk);
}

static void meson_mmc_disable_resampling(struct meson_host *host)
{
	unsigned int val = readl(host->regs + host->data->adjust);

	val &= ~ADJUST_ADJ_EN;
	writel(val, host->regs + host->data->adjust);
}

static void meson_mmc_reset_resampling(struct meson_host *host)
{
	unsigned int val;

	meson_mmc_disable_resampling(host);

	val = readl(host->regs + host->data->adjust);
	val &= ~ADJUST_ADJ_DELAY_MASK;
	writel(val, host->regs + host->data->adjust);
}

/* resampling with no delay, the vendor driver's setting for MMC high speed */
static void meson_mmc_enable_resampling(struct meson_host *host)
{
	unsigned int val;

	meson_mmc_reset_resampling(host);

	val = readl(host->regs + host->data->adjust);
	val |= ADJUST_ADJ_EN;
	writel(val, host->regs + host->data->adjust);
}

static int meson_mmc_resampling_tuning(struct mci_host *mmc, u32 opcode)
{
	struct meson_host *host = mmc_priv(mmc);
	unsigned int val, dly = 0, max_dly, i;
	int ret;

	/* Resampling is done using the source clock */
	max_dly = DIV_ROUND_UP(clk_get_rate(host->mux_clk),
			       clk_get_rate(host->mmc_clk));

	val = readl(host->regs + host->data->adjust);
	val |= ADJUST_ADJ_EN;
	writel(val, host->regs + host->data->adjust);

	for (i = 0; i < max_dly; i++) {
		val &= ~ADJUST_ADJ_DELAY_MASK;
		val |= FIELD_PREP(ADJUST_ADJ_DELAY_MASK, (dly + i) % max_dly);
		writel(val, host->regs + host->data->adjust);

		ret = mmc_send_tuning(mmc->mci, opcode);
		if (!ret) {
			dev_dbg(mmc_dev(mmc), "resampling delay: %u\n",
				(dly + i) % max_dly);
			return 0;
		}
	}

	meson_mmc_reset_resampling(host);
	return -EIO;
}

static int meson_mmc_prepare_ios_clock(struct meson_host *host,
				       struct mci_ios *ios)
{
	bool ddr;

	switch (ios->timing) {
	case MMC_TIMING_MMC_DDR52:
	case MMC_TIMING_UHS_DDR50:
		ddr = true;
		break;

	default:
		ddr = false;
		break;
	}

	return meson_mmc_clk_set(host, ios->clock, ddr);
}

static void meson_mmc_check_resampling(struct meson_host *host,
				       struct mci_ios *ios)
{
	switch (ios->timing) {
	case MMC_TIMING_MMC_HS:
		if (host->data->resample_mmc_hs) {
			meson_mmc_enable_resampling(host);
			break;
		}
		fallthrough;
	case MMC_TIMING_LEGACY:
	case MMC_TIMING_SD_HS:
	case MMC_TIMING_MMC_DDR52:
		meson_mmc_disable_resampling(host);
		break;
	default:
		break;
	}
}

static void meson_mmc_set_ios(struct mci_host *mmc, struct mci_ios *ios)
{
	struct meson_host *host = mmc_priv(mmc);
	u32 bus_width, val;
	int err;

	/* Bus width */
	switch (ios->bus_width) {
	case MMC_BUS_WIDTH_1:
		bus_width = CFG_BUS_WIDTH_1;
		break;
	case MMC_BUS_WIDTH_4:
		bus_width = CFG_BUS_WIDTH_4;
		break;
	case MMC_BUS_WIDTH_8:
		bus_width = CFG_BUS_WIDTH_8;
		break;
	default:
		dev_err(host->dev, "Invalid ios->bus_width: %u.  Setting to 4.\n",
			ios->bus_width);
		bus_width = CFG_BUS_WIDTH_4;
	}

	val = readl(host->regs + SD_EMMC_CFG);
	val &= ~CFG_BUS_WIDTH_MASK;
	val |= FIELD_PREP(CFG_BUS_WIDTH_MASK, bus_width);
	writel(val, host->regs + SD_EMMC_CFG);

	meson_mmc_check_resampling(host, ios);
	err = meson_mmc_prepare_ios_clock(host, ios);
	if (err)
		dev_err(host->dev, "Failed to set clock: %d\n,", err);

	dev_dbg(host->dev, "SD_EMMC_CFG:  0x%08x\n", val);
}

static void meson_mmc_set_blksz(struct mci_host *mmc, unsigned int blksz)
{
	struct meson_host *host = mmc_priv(mmc);
	u32 cfg, blksz_old;

	cfg = readl(host->regs + SD_EMMC_CFG);
	blksz_old = FIELD_GET(CFG_BLK_LEN_MASK, cfg);

	if (!is_power_of_2(blksz))
		dev_err(host->dev, "blksz %u is not a power of 2\n", blksz);

	blksz = ilog2(blksz);

	/* check if block-size matches, if not update */
	if (blksz == blksz_old)
		return;

	dev_dbg(host->dev, "%s: update blk_len %d -> %d\n", __func__,
		blksz_old, blksz);

	cfg &= ~CFG_BLK_LEN_MASK;
	cfg |= FIELD_PREP(CFG_BLK_LEN_MASK, blksz);
	writel(cfg, host->regs + SD_EMMC_CFG);
}

static void meson_mmc_set_response_bits(struct mci_cmd *cmd, u32 *cmd_cfg)
{
	if (cmd->resp_type & MMC_RSP_PRESENT) {
		if (cmd->resp_type & MMC_RSP_136)
			*cmd_cfg |= CMD_CFG_RESP_128;
		*cmd_cfg |= CMD_CFG_RESP_NUM;

		if (!(cmd->resp_type & MMC_RSP_CRC))
			*cmd_cfg |= CMD_CFG_RESP_NOCRC;

		if (cmd->resp_type & MMC_RSP_BUSY)
			*cmd_cfg |= CMD_CFG_R1B;
	} else {
		*cmd_cfg |= CMD_CFG_NO_RESP;
	}
}

/* local copy for dram_access_quirk */
static void meson_mmc_copy_buffer(struct meson_host *host, struct mci_data *data,
				  size_t buflen, bool to_buffer)
{
	unsigned int buf_offset = 0;
	unsigned int left = buflen;
	u32 *buf = (u32 *)data->dest;

	if (to_buffer) {
		do {
			writel(*buf++, host->bounce_iomem_buf + buf_offset);

			buf_offset += 4;
			left -= 4;
		} while (left);
	} else {
		do {
			*buf++ = readl(host->bounce_iomem_buf + buf_offset);

			buf_offset += 4;
			left -= 4;
		} while (left);
	}
}

static void meson_mmc_start_cmd(struct mci_host *mmc, struct mci_cmd *cmd)
{
	struct meson_host *host = mmc_priv(mmc);
	struct mci_data *data = cmd->data;
	u32 cmd_cfg = 0, cmd_data = 0;
	unsigned int xfer_bytes = 0;

	host->cmd = cmd;

	cmd_cfg |= FIELD_PREP(CMD_CFG_CMD_INDEX_MASK, cmd->cmdidx);
	cmd_cfg |= CMD_CFG_OWNER;  /* owned by CPU */

	meson_mmc_set_response_bits(cmd, &cmd_cfg);

	/* data? */
	if (data) {
		cmd_cfg |= CMD_CFG_DATA_IO;
		cmd_cfg |= FIELD_PREP(CMD_CFG_TIMEOUT_MASK,
				      ilog2(meson_mmc_get_timeout_msecs(data)));

		if (data->blocks > 1) {
			cmd_cfg |= CMD_CFG_BLOCK_MODE;
			cmd_cfg |= FIELD_PREP(CMD_CFG_LENGTH_MASK,
					      data->blocks);
			meson_mmc_set_blksz(mmc, data->blocksize);
		} else {
			cmd_cfg |= FIELD_PREP(CMD_CFG_LENGTH_MASK, data->blocksize);
		}

		xfer_bytes = data->blocksize * data->blocks;
		if (data->flags & MMC_DATA_WRITE) {
			cmd_cfg |= CMD_CFG_DATA_WR;
			WARN_ON(xfer_bytes > host->bounce_buf_size);
			if (host->dram_access_quirk) {
				meson_mmc_copy_buffer(host, data, xfer_bytes, true);
			} else if (!meson_mmc_desc_chain_mode(data)) {
				memcpy(host->bounce_buf, data->src, xfer_bytes);
				dma_sync_single_for_device(host->dev, host->bounce_dma_addr,
							   xfer_bytes, DMA_BIDIRECTIONAL);
			}
		}

		if (meson_mmc_desc_chain_mode(data))
			cmd_data = host->direct_dma_addr & CMD_DATA_MASK;
		else
			cmd_data = host->bounce_dma_addr & CMD_DATA_MASK;
	} else {
		cmd_cfg |= FIELD_PREP(CMD_CFG_TIMEOUT_MASK,
				      ilog2(SD_EMMC_CMD_TIMEOUT));
	}

	/* Last descriptor */
	cmd_cfg |= CMD_CFG_END_OF_CHAIN;
	writel(cmd_cfg, host->regs + SD_EMMC_CMD_CFG);
	writel(cmd_data, host->regs + SD_EMMC_CMD_DAT);
	writel(0, host->regs + SD_EMMC_CMD_RSP);
	writel(cmd->cmdarg, host->regs + SD_EMMC_CMD_ARG);
}

static int meson_mmc_validate_dram_access(struct mci_host *mmc, struct mci_data *data)
{
	unsigned int len = data->blocksize * data->blocks;

	/* Reject request if any element offset or size is not 32bit aligned */
	if (!IS_ALIGNED((uintptr_t)data->dest, sizeof(u32)) ||
	    !IS_ALIGNED(len, sizeof(u32))) {
		dev_err(mmc_dev(mmc), "unaligned buffer %p len %u\n",
			data->dest, len);
		return -EINVAL;
	}

	return 0;
}

static void meson_mmc_read_resp(struct mci_host *mmc, struct mci_cmd *cmd)
{
	struct meson_host *host = mmc_priv(mmc);

	if (cmd->resp_type & MMC_RSP_136) {
		cmd->response[0] = readl(host->regs + SD_EMMC_CMD_RSP3);
		cmd->response[1] = readl(host->regs + SD_EMMC_CMD_RSP2);
		cmd->response[2] = readl(host->regs + SD_EMMC_CMD_RSP1);
		cmd->response[3] = readl(host->regs + SD_EMMC_CMD_RSP);
	} else if (cmd->resp_type & MMC_RSP_PRESENT) {
		cmd->response[0] = readl(host->regs + SD_EMMC_CMD_RSP);
	}
}

static int meson_mmc_irq(struct meson_host *host)
{
	struct mci_cmd *cmd;
	u32 status, raw_status, irq_mask = IRQ_EN_MASK;
	int ret = 0;

	raw_status = readl(host->regs + SD_EMMC_STATUS);
	status = raw_status & irq_mask;

	/* ack all raised interrupts */
	writel(status, host->regs + SD_EMMC_STATUS);

	cmd = host->cmd;

	if (status & IRQ_CRC_ERR) {
		dev_dbg(host->dev, "CRC Error - status 0x%08x\n", status);
		ret = -EILSEQ;
		goto out;
	}

	if (status & IRQ_TIMEOUTS) {
		dev_dbg(host->dev, "Timeout - status 0x%08x\n", status);
		ret = -ETIMEDOUT;
		goto out;
	}

	meson_mmc_read_resp(host->mmc, cmd);

	if (status & (IRQ_END_OF_CHAIN | IRQ_RESP_STATUS))
		return 0;

out:
	if (ret) {
		/* Stop desc in case of errors */
		u32 start = readl(host->regs + SD_EMMC_START);

		start &= ~START_DESC_BUSY;
		writel(start, host->regs + SD_EMMC_START);
	}

	return ret;
}

static int meson_mmc_wait_desc_stop(struct meson_host *host)
{
	u32 status;

	/*
	 * It may sometimes take a while for it to actually halt. Here, we
	 * are giving it 5ms to comply
	 *
	 * If we don't confirm the descriptor is stopped, it might raise new
	 * IRQs after we have called mmc_request_done() which is bad.
	 */

	return readl_poll_timeout(host->regs + SD_EMMC_STATUS, status,
				  !(status & (STATUS_BUSY | STATUS_DESC_BUSY)),
				  5000);
}

static void meson_mmc_irq_thread(struct meson_host *host, int err)
{
	struct mci_cmd *cmd = host->cmd;
	struct mci_data *data;
	unsigned int xfer_bytes;

	if (err) {
		meson_mmc_wait_desc_stop(host);
		return;
	}

	data = cmd->data;
	if (meson_mmc_bounce_buf_read(host, data)) {
		xfer_bytes = data->blocksize * data->blocks;
		WARN_ON(xfer_bytes > host->bounce_buf_size);
		if (host->dram_access_quirk) {
			meson_mmc_copy_buffer(host, data, xfer_bytes, false);
		} else {
			dma_sync_single_for_cpu(host->dev, host->bounce_dma_addr,
						xfer_bytes, DMA_BIDIRECTIONAL);
			memcpy(data->dest, host->bounce_buf, xfer_bytes);
		}
	}
}

static int meson_mmc_send_cmd(struct mci_host *mmc, struct mci_cmd *cmd)
{
	struct meson_host *host = mmc_priv(mmc);
	struct mci_data *data = cmd->data;
	unsigned int timeout;
	u32 status;
	int ret;

	/*
	 * The memory at the end of the controller used as bounce buffer for
	 * the dram_access_quirk only accepts 32bit read/write access,
	 * check the alignment and length of the data before starting the request.
	 */
	if (host->dram_access_quirk && data) {
		ret = meson_mmc_validate_dram_access(mmc, data);
		if (ret)
			return ret;
	}

	if (data)
		meson_mmc_pre_req(mmc, data);

	/* Stop execution */
	writel(0, host->regs + SD_EMMC_START);

	/* drop what an earlier command raised after its end */
	writel(IRQ_EN_MASK, host->regs + SD_EMMC_STATUS);

	meson_mmc_start_cmd(mmc, cmd);

	/* the controller times out on its own, this is only a backstop */
	timeout = data ? meson_mmc_get_timeout_msecs(data) : SD_EMMC_CMD_TIMEOUT;
	ret = readl_poll_timeout(host->regs + SD_EMMC_STATUS, status,
				 status & IRQ_EN_MASK, 2 * timeout * USEC_PER_MSEC);
	if (!ret)
		ret = meson_mmc_irq(host);

	meson_mmc_irq_thread(host, ret);

	if (data)
		meson_mmc_post_req(mmc, data);

	return ret;
}

static void meson_mmc_cfg_init(struct meson_host *host)
{
	u32 cfg = 0;

	cfg |= FIELD_PREP(CFG_RESP_TIMEOUT_MASK,
			  ilog2(SD_EMMC_CFG_RESP_TIMEOUT));
	cfg |= FIELD_PREP(CFG_RC_CC_MASK, ilog2(SD_EMMC_CFG_CMD_GAP));
	cfg |= FIELD_PREP(CFG_BLK_LEN_MASK, ilog2(SD_EMMC_CFG_BLK_SIZE));

	/* abort chain on R/W errors */
	cfg |= CFG_ERR_ABORT;

	writel(cfg, host->regs + SD_EMMC_CFG);
}

static int meson_mmc_init(struct mci_host *mmc, struct device *dev)
{
	return 0;
}

static const struct mci_ops meson_mmc_ops = {
	.init		= meson_mmc_init,
	.send_cmd	= meson_mmc_send_cmd,
	.set_ios	= meson_mmc_set_ios,
	.execute_tuning = meson_mmc_resampling_tuning,
};

static int meson_mmc_probe(struct device *dev)
{
	struct resource *res;
	struct meson_host *host;
	struct mci_host *mmc;
	struct clk *core_clk;
	int ret;

	host = xzalloc(sizeof(*host));
	mmc = &host->mci;
	host->mmc = mmc;
	host->dev = dev;
	dev_set_drvdata(dev, host);
	mmc->hw_dev = dev;
	dma_set_mask(dev, DMA_BIT_MASK(32));

	/* The G12A SDIO Controller needs an SRAM bounce buffer */
	host->dram_access_quirk = of_property_read_bool(dev->of_node,
							"amlogic,dram-access-quirk");

	mmc->voltages = MMC_VDD_31_32 | MMC_VDD_32_33 | MMC_VDD_33_34 |
			MMC_VDD_165_195;

	mci_of_parse(mmc);

	/* mci_of_parse() leaves these to the host driver */
	if (of_property_read_bool(dev->of_node, "cap-sd-highspeed"))
		mmc->host_caps |= MMC_CAP_SD_HIGHSPEED;
	if (of_property_read_bool(dev->of_node, "cap-mmc-highspeed"))
		mmc->host_caps |= MMC_CAP_MMC_HIGHSPEED |
				  MMC_CAP_MMC_HIGHSPEED_52MHZ;
	if (of_property_read_bool(dev->of_node, "mmc-ddr-3_3v"))
		mmc->host_caps |= MMC_CAP_3_3V_DDR;
	if (of_property_read_bool(dev->of_node, "mmc-ddr-1_8v"))
		mmc->host_caps |= MMC_CAP_1_8V_DDR;

	host->data = of_device_get_match_data(dev);
	if (!host->data)
		return -EINVAL;

	ret = device_reset_optional(dev);
	if (ret)
		return dev_err_probe(dev, ret, "device reset failed\n");

	host->regs = dev_platform_get_and_ioremap_resource(dev, 0, &res);
	if (IS_ERR(host->regs))
		return PTR_ERR(host->regs);

	host->pinctrl = pinctrl_get(dev);
	if (IS_ERR(host->pinctrl))
		return PTR_ERR(host->pinctrl);

	host->pins_clk_gate = pinctrl_lookup_state(host->pinctrl,
						   "clk-gate");
	if (IS_ERR(host->pins_clk_gate)) {
		dev_warn(dev,
			 "can't get clk-gate pinctrl, using clk_stop bit\n");
		host->pins_clk_gate = NULL;
	}

	core_clk = clk_get_enabled(dev, "core");
	if (IS_ERR(core_clk))
		return PTR_ERR(core_clk);

	if (host->data->has_pipeline_clk) {
		struct clk *pipe_clk;

		pipe_clk = clk_get_enabled(dev, "pipeline");
		if (IS_ERR(pipe_clk))
			return dev_err_probe(dev, PTR_ERR(pipe_clk),
					     "missing pipeline clock\n");
	}

	ret = meson_mmc_clk_init(host);
	if (ret)
		return ret;

	/* set config to sane default */
	meson_mmc_cfg_init(host);

	/* Stop execution */
	writel(0, host->regs + SD_EMMC_START);

	/* clear and ack interrupts, they are polled */
	writel(0, host->regs + SD_EMMC_IRQ_EN);
	writel(IRQ_EN_MASK, host->regs + SD_EMMC_STATUS);

	if (host->dram_access_quirk) {
		/* Limit to the available sram memory */
		mmc->max_req_size = SD_EMMC_SRAM_DATA_BUF_LEN;
	} else {
		mmc->max_req_size = CMD_CFG_LENGTH_MASK * SD_EMMC_CFG_BLK_SIZE;
	}

	/*
	 * At the moment, we don't know how to reliably enable HS400.
	 * From the different datasheets, it is not even clear if this mode
	 * is officially supported by any of the SoCs
	 */
	mmc->caps2 &= ~MMC_CAP2_HS400;

	if (host->dram_access_quirk) {
		/*
		 * The MMC Controller embeds 1,5KiB of internal SRAM
		 * that can be used to be used as bounce buffer.
		 * In the case of the G12A SDIO controller, use these
		 * instead of the DDR memory
		 */
		host->bounce_buf_size = SD_EMMC_SRAM_DATA_BUF_LEN;
		host->bounce_iomem_buf = host->regs + SD_EMMC_SRAM_DATA_BUF_OFF;
		host->bounce_dma_addr = res->start + SD_EMMC_SRAM_DATA_BUF_OFF;
	} else {
		/* data bounce buffer */
		host->bounce_buf_size = mmc->max_req_size;
		host->bounce_buf = dma_alloc(host->bounce_buf_size);
		if (host->bounce_buf == NULL) {
			dev_err(host->dev, "Unable to map allocate DMA bounce buffer.\n");
			ret = -ENOMEM;
			goto err_init_clk;
		}
		host->bounce_dma_addr = dma_map_single(host->dev, host->bounce_buf,
						       host->bounce_buf_size,
						       DMA_BIDIRECTIONAL);
		if (dma_mapping_error(host->dev, host->bounce_dma_addr)) {
			ret = -EFAULT;
			goto err_init_clk;
		}
	}

	mmc->ops = meson_mmc_ops;
	ret = mci_register(mmc);
	if (ret)
		goto err_init_clk;

	return 0;

err_init_clk:
	clk_disable_unprepare(host->mmc_clk);
	return ret;
}

static const struct meson_mmc_data meson_gx_data = {
	.tx_delay_mask	= CLK_V2_TX_DELAY_MASK,
	.rx_delay_mask	= CLK_V2_RX_DELAY_MASK,
	.always_on	= CLK_V2_ALWAYS_ON,
	.adjust		= SD_EMMC_ADJUST,
	.irq_sdio_sleep	= CLK_V2_IRQ_SDIO_SLEEP,
};

static const struct meson_mmc_data meson_axg_data = {
	.tx_delay_mask	= CLK_V3_TX_DELAY_MASK,
	.rx_delay_mask	= CLK_V3_RX_DELAY_MASK,
	.always_on	= CLK_V3_ALWAYS_ON,
	.adjust		= SD_EMMC_V3_ADJUST,
	.irq_sdio_sleep	= CLK_V3_IRQ_SDIO_SLEEP,
};

static const struct meson_mmc_data meson_t7_data = {
	.tx_delay_mask	= CLK_V3_TX_DELAY_MASK,
	.rx_delay_mask	= CLK_V3_RX_DELAY_MASK,
	.always_on	= CLK_V3_ALWAYS_ON,
	.adjust		= SD_EMMC_V3_ADJUST,
	.irq_sdio_sleep	= CLK_V3_IRQ_SDIO_SLEEP,
	.has_pipeline_clk = true,
};

static const struct meson_mmc_data meson_a9_data = {
	.tx_delay_mask	= CLK_V3_TX_DELAY_MASK,
	.rx_delay_mask	= CLK_V3_RX_DELAY_MASK,
	.always_on	= CLK_V3_ALWAYS_ON,
	.adjust		= SD_EMMC_V3_ADJUST,
	.irq_sdio_sleep	= CLK_V3_IRQ_SDIO_SLEEP,
	.resample_mmc_hs = true,
};

static const struct of_device_id meson_mmc_of_match[] = {
	{ .compatible = "amlogic,meson-gx-mmc",		.data = &meson_gx_data },
	{ .compatible = "amlogic,meson-gxbb-mmc", 	.data = &meson_gx_data },
	{ .compatible = "amlogic,meson-gxl-mmc",	.data = &meson_gx_data },
	{ .compatible = "amlogic,meson-gxm-mmc",	.data = &meson_gx_data },
	{ .compatible = "amlogic,meson-axg-mmc",	.data = &meson_axg_data },
	{ .compatible = "amlogic,t7-mmc",		.data = &meson_t7_data },
	/* barebox only, Linux has no binding for the A9 yet */
	{ .compatible = "amlogic,a9-mmc",		.data = &meson_a9_data },
	{}
};
MODULE_DEVICE_TABLE(of, meson_mmc_of_match);

static struct driver meson_mmc_driver = {
	.probe		= meson_mmc_probe,
	.name = DRIVER_NAME,
	.of_match_table = meson_mmc_of_match,
};

device_platform_driver(meson_mmc_driver);

MODULE_DESCRIPTION("Amlogic S905*/GX*/AXG SD/eMMC driver");
MODULE_AUTHOR("Kevin Hilman <khilman@baylibre.com>");
MODULE_LICENSE("GPL v2");
