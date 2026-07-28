/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: 2024 Jules Maselbas */
/* derived from: linux/drivers/mmc/host/sunxi-mmc.c */

#ifndef SUNXI_MMC_H
#define SUNXI_MMC_H

#include <io.h>
#include <linux/bitops.h>
#include <linux/gpio/consumer.h>
#include <clock.h>
#include <mci.h>

#define SDXC_REG_GCTRL	(0x00) /* Global Control */
#define SDXC_REG_CLKCR	(0x04) /* Clock Control */
#define SDXC_REG_TMOUT	(0x08) /* Time Out */
#define SDXC_REG_WIDTH	(0x0C) /* Bus Width */
#define SDXC_REG_BLKSZ	(0x10) /* Block Size */
#define SDXC_REG_BCNTR	(0x14) /* Byte Count */
#define SDXC_REG_CMDR	(0x18) /* Command */
#define SDXC_REG_CARG	(0x1C) /* Argument */
#define SDXC_REG_RESP0	(0x20) /* Response 0 */
#define SDXC_REG_RESP1	(0x24) /* Response 1 */
#define SDXC_REG_RESP2	(0x28) /* Response 2 */
#define SDXC_REG_RESP3	(0x2C) /* Response 3 */
#define SDXC_REG_IMASK	(0x30) /* Interrupt Mask */
#define SDXC_REG_MISTA	(0x34) /* Masked Interrupt Status */
#define SDXC_REG_RINTR	(0x38) /* Raw Interrupt Status */
#define SDXC_REG_STAS	(0x3C) /* Status */
#define SDXC_REG_FTRGL	(0x40) /* FIFO Threshold Watermark */
#define SDXC_REG_FUNS	(0x44) /* Function Select */
#define SDXC_REG_CBCR	(0x48) /* CIU Byte Count */
#define SDXC_REG_BBCR	(0x4C) /* BIU Byte Count */
#define SDXC_REG_DBGC	(0x50) /* Debug Enable */
#define SDXC_REG_A12A	(0x58) /* Auto Command 12 Argument */
#define SDXC_REG_NTSR	(0x5C) /* SMC New Timing Set Register */
#define SDXC_REG_HWRST	(0x78) /* Card Hardware Reset */
#define SDXC_REG_DMAC	(0x80) /* IDMAC Control */
#define SDXC_REG_DLBA	(0x84) /* IDMAC Descriptor List Base Addresse */
#define SDXC_REG_IDST	(0x88) /* IDMAC Status */
#define SDXC_REG_IDIE	(0x8C) /* IDMAC Interrupt Enable */
#define SDXC_REG_CHDA	(0x90)
#define SDXC_REG_CBDA	(0x94)

#define SDXC_REG_DRV_DL		0x140 /* Drive Delay Control Register */
#define SDXC_REG_SAMP_DL_REG	0x144 /* SMC sample delay control */
#define SDXC_REG_DS_DL_REG	0x148 /* SMC data strobe delay control */

#define SDXC_REG_FIFO	(0x200) /* FIFO */

#define SDXC_GCTRL_SOFT_RESET		BIT(0)
#define SDXC_GCTRL_FIFO_RESET		BIT(1)
#define SDXC_GCTRL_DMA_RESET		BIT(2)
#define SDXC_GCTRL_RESET \
	(SDXC_GCTRL_SOFT_RESET | SDXC_GCTRL_FIFO_RESET | SDXC_GCTRL_DMA_RESET)
#define SDXC_GCTRL_DMA_ENABLE		BIT(5)
#define SDXC_GCTRL_ACCESS_BY_AHB	BIT(31)

#define SDXC_CMD_RESP_EXPIRE		BIT(6)
#define SDXC_CMD_LONG_RESPONSE		BIT(7)
#define SDXC_CMD_CHK_RESPONSE_CRC	BIT(8)
#define SDXC_CMD_DATA_EXPIRE		BIT(9)
#define SDXC_CMD_WRITE			BIT(10)
#define SDXC_CMD_AUTO_STOP		BIT(12)
#define SDXC_CMD_WAIT_PRE_OVER		BIT(13)
#define SDXC_CMD_ABORT_STOP		BIT(14)
#define SDXC_CMD_SEND_INIT_SEQ		BIT(15)
#define SDXC_CMD_UPCLK_ONLY		BIT(21)
#define SDXC_CMD_START			BIT(31)

#define SDXC_NTSR_2X_TIMING_MODE	BIT(31)

/* clock control bits */
#define SDXC_CLK_MASK_DATA0	BIT(31)
#define SDXC_CLK_LOW_POWER_ON	BIT(17)
#define SDXC_CLK_ENABLE		BIT(16)
#define SDXC_CLK_DIVIDER_MASK	(0xff)

/* bus width */
#define SDXC_WIDTH_1BIT	0
#define SDXC_WIDTH_4BIT	BIT(0)
#define SDXC_WIDTH_8BIT	BIT(1)

/* interrupt bits */
#define SDXC_INT_RESP_ERROR		BIT(1)
#define SDXC_INT_COMMAND_DONE		BIT(2)
#define SDXC_INT_DATA_OVER		BIT(3)
#define SDXC_INT_TX_DATA_REQUEST	BIT(4)
#define SDXC_INT_RX_DATA_REQUEST	BIT(5)
#define SDXC_INT_RESP_CRC_ERROR		BIT(6)
#define SDXC_INT_DATA_CRC_ERROR		BIT(7)
#define SDXC_INT_RESP_TIMEOUT		BIT(8)
#define SDXC_INT_DATA_TIMEOUT		BIT(9)
#define SDXC_INT_VOLTAGE_CHANGE_DONE	BIT(10)
#define SDXC_INT_FIFO_RUN_ERROR		BIT(11)
#define SDXC_INT_HARD_WARE_LOCKED	BIT(12)
#define SDXC_INT_START_BIT_ERROR	BIT(13)
#define SDXC_INT_AUTO_COMMAND_DONE	BIT(14)
#define SDXC_INT_END_BIT_ERROR		BIT(15)
#define SDXC_INT_SDIO_INTERRUPT		BIT(16)
#define SDXC_INT_CARD_INSERT		BIT(30)
#define SDXC_INT_CARD_REMOVE		BIT(31)
#define SDXC_INTERRUPT_ERROR_BIT	\
	(SDXC_INT_RESP_ERROR |		\
	 SDXC_INT_RESP_CRC_ERROR |	\
	 SDXC_INT_DATA_CRC_ERROR |	\
	 SDXC_INT_RESP_TIMEOUT |	\
	 SDXC_INT_DATA_TIMEOUT |	\
	 SDXC_INT_HARD_WARE_LOCKED |	\
	 SDXC_INT_START_BIT_ERROR |	\
	 SDXC_INT_END_BIT_ERROR)

#define SDXC_INTERRUPT_DONE_BIT		\
	(SDXC_INT_AUTO_COMMAND_DONE |	\
	 SDXC_INT_DATA_OVER |		\
	 SDXC_INT_COMMAND_DONE |	\
	 SDXC_INT_VOLTAGE_CHANGE_DONE)

/* status */
#define SDXC_STATUS_FIFO_EMPTY		BIT(2)
#define SDXC_STATUS_FIFO_FULL		BIT(3)
#define SDXC_STATUS_CARD_PRESENT	BIT(8)
#define SDXC_STATUS_BUSY		BIT(9)

struct sunxi_mmc_clk_delay {
	u32 output;
	u32 sample;
};

struct sunxi_mmc_cfg {
	u32 idma_des_size_bits;
	u32 idma_des_shift;
	const struct sunxi_mmc_clk_delay *clk_delays;

	/* does the IP block support autocalibration? */
	bool can_calibrate;

	/* Does DATA0 needs to be masked while the clock is updated */
	bool mask_data0;

	/*
	 * hardware only supports new timing mode, either due to lack of
	 * a mode switch in the clock controller, or the mmc controller
	 * is permanently configured in the new timing mode, without the
	 * NTSR mode switch.
	 */
	bool needs_new_timings;

	/* clock hardware can switch between old and new timing modes */
	bool ccu_has_timings_switch;
};

struct sunxi_mmc_host {
	struct mci_host mci;
	struct device *dev;
	struct clk *clk_ahb, *clk_mmc;
	void __iomem *base;
	struct gpio_desc *gpio_cd;

	const struct sunxi_mmc_cfg *cfg;
	u32 clkrate;
};

static inline struct sunxi_mmc_host *to_sunxi_mmc_host(struct mci_host *mci)
{
	return container_of(mci, struct sunxi_mmc_host, mci);
}

static inline u32 sdxc_readl(struct sunxi_mmc_host *host, u32 reg)
{
	return readl(host->base + reg);
}

static inline void sdxc_writel(struct sunxi_mmc_host *host, u32 reg, u32 val)
{
	writel(val, host->base + reg);
}

static inline int sdxc_is_fifo_empty(struct sunxi_mmc_host *host)
{
	return sdxc_readl(host, SDXC_REG_STAS) & SDXC_STATUS_FIFO_EMPTY;
}

static inline int sdxc_is_fifo_full(struct sunxi_mmc_host *host)
{
	return sdxc_readl(host, SDXC_REG_STAS) & SDXC_STATUS_FIFO_FULL;
}

static inline int sdxc_is_card_busy(struct sunxi_mmc_host *host)
{
	return sdxc_readl(host, SDXC_REG_STAS) & SDXC_STATUS_BUSY;
}

#ifdef __PBL__
/*
 * Stubs to make timeout logic below work in PBL
 */
#define get_time_ns()		0
/*
 * Use time in us as a busy counter timeout value
 */
#define is_timeout(s, t)	((s)++ > ((t) / 1000))
#endif

static inline int sdxc_xfer_complete(struct sunxi_mmc_host *host, u64 timeout, u32 flags)
{
	u64 start;
	u32 rint;

	start = get_time_ns();
	do {
		rint = sdxc_readl(host, SDXC_REG_RINTR);
		if (rint & SDXC_INTERRUPT_ERROR_BIT)
			break;
		if (rint & flags)
			return 0;
	} while (!is_timeout(start, timeout));

	return -ETIMEDOUT;
}

#endif
