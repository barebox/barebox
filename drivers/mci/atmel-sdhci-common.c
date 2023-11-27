// SPDX-License-Identifier: GPL-2.0-only AND BSD-1-Clause
/*
 * Copyright (c) 2015, Atmel Corporation
 * Copyright (c) 2019, Ahmad Fatoum, Pengutronix
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 */

#define pr_fmt(fmt) "atmel-sdhci: " fmt

#include <common.h>
#include <mci.h>
#include <linux/bitfield.h>

#include <mach/at91/early_udelay.h>

#ifdef __PBL__
#define udelay early_udelay
#undef  dev_err
#define dev_err(d, ...)		pr_err(__VA_ARGS__)
#undef  dev_warn
#define dev_warn(d, ...)	pr_warn(__VA_ARGS__)
#endif

#include "atmel-sdhci.h"

#define AT91_SDHCI_MC1R		0x204
#define		AT91_SDHCI_MC1_FCD		BIT(7)
#define AT91_SDHCI_CALCR	0x240
#define		AT91_SDHCI_CALCR_EN		BIT(0)
#define		AT91_SDHCI_CALCR_ALWYSON	BIT(4)

static inline struct at91_sdhci *to_priv(struct sdhci *sdhci)
{
	return container_of(sdhci, struct at91_sdhci, sdhci);
}

void at91_sdhci_host_capability(struct at91_sdhci *host,
				unsigned *voltages)
{
	u16 caps;

	caps = sdhci_read32(&host->sdhci, SDHCI_CAPABILITIES);

	if (caps & SDHCI_CAN_VDD_330)
		*voltages |= MMC_VDD_32_33 | MMC_VDD_33_34;
	if (caps & SDHCI_CAN_VDD_300)
		*voltages |= MMC_VDD_29_30 | MMC_VDD_30_31;
	if (caps & SDHCI_CAN_VDD_180)
		*voltages |= MMC_VDD_165_195;
}

bool at91_sdhci_is_card_inserted(struct at91_sdhci *host)
{
	struct sdhci *sdhci = &host->sdhci;
	bool is_inserted = false;
	u32 status_mask, reg;
	int ret;

	/* Enable (unmask) the Interrupt Status 'card inserted' bit */
	status_mask = sdhci_read32(sdhci, SDHCI_INT_ENABLE);
	status_mask |= SDHCI_INT_CARD_INSERT;
	sdhci_write32(sdhci, SDHCI_INT_ENABLE, status_mask);

	reg = sdhci_read32(sdhci, SDHCI_PRESENT_STATE);
	if (reg & SDHCI_CARD_PRESENT) {
		is_inserted = true;
		goto exit;
	}

	/*
	 * Debouncing of the card detect pin is up to 13ms on sama5d2 rev B
	 * and later.  Try to be safe and wait for up to 50ms.
	 */
	ret = sdhci_read32_poll_timeout(sdhci, SDHCI_INT_STATUS, reg,
					reg & SDHCI_INT_CARD_INSERT,
					50 * USEC_PER_MSEC);
	if (ret == 0)
		is_inserted = true;
exit:
	status_mask &= ~SDHCI_INT_CARD_INSERT;
	sdhci_write32(sdhci, SDHCI_INT_ENABLE, status_mask);

	status_mask = sdhci_read32(sdhci, SDHCI_INT_STATUS);
	status_mask |= SDHCI_INT_CARD_INSERT;
	sdhci_write32(sdhci, SDHCI_INT_STATUS, status_mask);

	return is_inserted;
}

int at91_sdhci_send_command(struct at91_sdhci *host, struct mci_cmd *cmd,
			    struct mci_data *data)
{
	unsigned command, xfer;
	struct sdhci *sdhci = &host->sdhci;
	u32 mask;
	int status;
	int ret;

	ret = sdhci_wait_idle(&host->sdhci, cmd);
	if (ret)
		return ret;

	sdhci_write32(sdhci, SDHCI_INT_STATUS, ~0U);

	mask = SDHCI_INT_CMD_COMPLETE;

	sdhci_set_cmd_xfer_mode(sdhci, cmd, data, false, &command, &xfer);

	if (data) {
		sdhci_write8(sdhci, SDHCI_TIMEOUT_CONTROL, 0xe);
		sdhci_write16(sdhci, SDHCI_BLOCK_SIZE,
			      SDHCI_DMA_BOUNDARY_512K
			      | SDHCI_TRANSFER_BLOCK_SIZE(data->blocksize));
		sdhci_write16(sdhci, SDHCI_BLOCK_COUNT, data->blocks);
		sdhci_write16(sdhci, SDHCI_TRANSFER_MODE, xfer);
		if (cmd->resp_type & MMC_RSP_BUSY)
			mask |= SDHCI_INT_XFER_COMPLETE;
	} else if (cmd->resp_type & MMC_RSP_BUSY) {
		sdhci_write16(sdhci, SDHCI_TIMEOUT_CONTROL, 0xe);
	}

	sdhci_write32(sdhci, SDHCI_ARGUMENT, cmd->cmdarg);
	sdhci_write16(sdhci, SDHCI_COMMAND, command);

	status = sdhci_wait_for_done(&host->sdhci, mask);
	if (status < 0)
		goto error;

	sdhci_read_response(sdhci, cmd);
	sdhci_write32(sdhci, SDHCI_INT_STATUS, mask);

	if (data)
		sdhci_transfer_data_pio(sdhci, data);

	udelay(1000);

	status = sdhci_read32(sdhci, SDHCI_INT_STATUS);
	sdhci_write32(sdhci, SDHCI_INT_STATUS, ~0U);

	return 0;

error:
	sdhci_write32(sdhci, SDHCI_INT_STATUS, ~0U);

	sdhci_reset(sdhci, SDHCI_RESET_CMD);
	sdhci_reset(sdhci, SDHCI_RESET_DATA);

	return status;
}

static void at91_sdhci_set_power(struct at91_sdhci *host, unsigned vdd)
{
	struct sdhci *sdhci = &host->sdhci;
	u8 pwr = 0;

	switch (vdd) {
	case MMC_VDD_165_195:
		pwr = SDHCI_POWER_180;
		break;
	case MMC_VDD_29_30:
	case MMC_VDD_30_31:
		pwr = SDHCI_POWER_300;
		break;
	case MMC_VDD_32_33:
	case MMC_VDD_33_34:
		pwr = SDHCI_POWER_330;
		break;
	}

	if (pwr == 0) {
		sdhci_write8(sdhci, SDHCI_POWER_CONTROL, 0);
		return;
	}

	pwr |= SDHCI_BUS_POWER_EN;

	sdhci_write8(sdhci, SDHCI_POWER_CONTROL, pwr);
}

static int at91_sdhci_set_clock(struct at91_sdhci *host, unsigned clock)
{

	struct sdhci *sdhci = &host->sdhci;
	unsigned clk = 0, clk_div;
	unsigned reg;
	u32 caps, caps_clk_mult;
	int ret;

	ret = sdhci_wait_idle(&host->sdhci, NULL);
	if (ret)
		return ret;

	sdhci_write16(sdhci, SDHCI_CLOCK_CONTROL, 0);

	if (clock == 0)
		return 0;

	caps = sdhci_read32(sdhci, SDHCI_CAPABILITIES_1);

	caps_clk_mult = FIELD_GET(SDHCI_CLOCK_MUL_MASK, caps);

	if (caps_clk_mult) {
		for (clk_div = 1; clk_div <= 1024; clk_div++) {
			if (host->caps_max_clock / clk_div <= clock)
				break;
		}
		clk = SDHCI_PROG_CLOCK_MODE;
		clk_div -= 1;
	} else {
		/* Version 3.00 divisors must be a multiple of 2. */
		if (host->caps_max_clock <= clock) {
			clk_div = 1;
		} else {
			for (clk_div = 2; clk_div < 2048; clk_div += 2) {
				if (host->caps_max_clock / clk_div <= clock)
					break;
			}
		}
		clk_div >>= 1;
	}

	clk |= SDHCI_FREQ_SEL(clk_div);
	clk |= ((clk_div & SDHCI_DIV_HI_MASK) >> SDHCI_DIV_MASK_LEN)
		<< SDHCI_DIVIDER_HI_SHIFT;
	clk |= SDHCI_CLOCK_INT_EN;

	sdhci_write16(sdhci, SDHCI_CLOCK_CONTROL, clk);

	ret = sdhci_read32_poll_timeout(sdhci, SDHCI_CLOCK_CONTROL, clk,
					clk & SDHCI_CLOCK_INT_STABLE,
					20 * USEC_PER_MSEC);
	if (ret) {
		dev_warn(host->dev, "Timeout waiting for clock stable\n");
		return ret;
	}

	clk |= SDHCI_CLOCK_CARD_EN;
	sdhci_write16(sdhci, SDHCI_CLOCK_CONTROL, clk);

	reg = sdhci_read8(sdhci, SDHCI_HOST_CONTROL);
	if (clock > 26000000)
		reg |= SDHCI_CTRL_HISPD;
	else
		reg &= ~SDHCI_CTRL_HISPD;

	sdhci_write8(sdhci, SDHCI_HOST_CONTROL, reg);

	return 0;
}

static int at91_sdhci_set_bus_width(struct at91_sdhci *host, unsigned bus_width)
{
	struct sdhci *sdhci = &host->sdhci;
	unsigned reg;

	reg = sdhci_read8(sdhci, SDHCI_HOST_CONTROL);

	switch(bus_width) {
	case MMC_BUS_WIDTH_8:
		reg |= SDHCI_CTRL_8BITBUS;
		break;
	case MMC_BUS_WIDTH_4:
		reg &= ~SDHCI_CTRL_8BITBUS;
		reg |= SDHCI_CTRL_4BITBUS;
		break;
	default:
		reg &= ~SDHCI_CTRL_8BITBUS;
		reg &= ~SDHCI_CTRL_4BITBUS;
	}

	sdhci_write8(sdhci, SDHCI_HOST_CONTROL, reg);

	return 0;
}

int at91_sdhci_set_ios(struct at91_sdhci *host, struct mci_ios *ios)
{
	int ret;

	ret = at91_sdhci_set_clock(host, ios->clock);
	if (ret)
		return ret;

	return at91_sdhci_set_bus_width(host, ios->bus_width);
}

int at91_sdhci_init(struct at91_sdhci *host, u32 maxclk,
		    bool force_cd, bool cal_always_on)
{
	struct sdhci *sdhci = &host->sdhci;
	unsigned status_mask;

	host->caps_max_clock = maxclk;

	at91_sdhci_set_power(host, MMC_VDD_32_33);

	status_mask = SDHCI_INT_CMD_COMPLETE
		| SDHCI_INT_XFER_COMPLETE
		| SDHCI_INT_SPACE_AVAIL
		| SDHCI_INT_DATA_AVAIL;

	status_mask |= SDHCI_INT_TIMEOUT
		| SDHCI_INT_CRC
		| SDHCI_INT_END_BIT
		| SDHCI_INT_INDEX
		| SDHCI_INT_DATA_TIMEOUT
		| SDHCI_INT_DATA_CRC
		| SDHCI_INT_DATA_END_BIT;

	sdhci_write32(sdhci, SDHCI_INT_ENABLE, status_mask);

	sdhci_write32(sdhci, SDHCI_SIGNAL_ENABLE, 0);

	/*
	 * If the device attached to the mci bus is not removable, it is safer
	 * to set the Force Card Detect bit. People often don't connect the
	 * card detect signal and use this pin for another purpose. If the card
	 * detect pin is not muxed to SDHCI controller, a default value is
	 * used. This value can be different from a SoC revision to another
	 * one. Problems come when this default value is not card present. To
	 * avoid this case, if the device is non removable then the card
	 * detection procedure using the SDMCC_CD signal is bypassed.
	 * This bit is reset when a software reset for all command is performed
	 * so we need to implement our own reset function to set back this bit.
	 */
	if (force_cd) {
		u8 mc1r = sdhci_read8(sdhci, AT91_SDHCI_MC1R);
		mc1r |= AT91_SDHCI_MC1_FCD;
		sdhci_write8(sdhci, AT91_SDHCI_MC1R, mc1r);
	}

	if (cal_always_on) {
		sdhci_write32(sdhci, AT91_SDHCI_CALCR,
			     AT91_SDHCI_CALCR_ALWYSON | AT91_SDHCI_CALCR_EN);
	}

	return 0;
}

void at91_sdhci_mmio_init(struct at91_sdhci *host, void __iomem *base)
{
	host->sdhci.base = base;
}
