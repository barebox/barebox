/*
 * tc358767 eDP encoder driver
 *
 * Copyright (C) 2016 CogentEmbedded Inc
 * Author: Andrey Gusakov <andrey.gusakov@cogentembedded.com>
 *
 * Copyright (C) 2016 Pengutronix, Philipp Zabel <p.zabel@pengutronix.de>
 *
 * Copyright (C) 2016 Zodiac Inflight Innovations
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <malloc.h>
#include <errno.h>
#include <i2c/i2c.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <gpio.h>
#include <of_gpio.h>
#include <video/media-bus-format.h>
#include <video/vpl.h>
#include <asm-generic/div64.h>

#define DP_LINK_BW_SET			0x100
#define DP_ENHANCED_FRAME_EN		(1 << 7)
#define DP_TRAINING_PATTERN_SET		0x102
#define TRAINING_LANE0_SET		0x103

#define DP_DOWNSPREAD_CTRL		0x107
#define DP_SPREAD_AMP_0_5		(1 << 4)

#define DP_MAIN_LINK_CHANNEL_CODING_SET	0x108
#define DP_SET_ANSI_8B10B		(1 << 0)

#define DP_LANE0_1_STATUS		0x202
#define DP_LANE2_3_STATUS		0x202
#define DP_LANE_CR_DONE			(1 << 0)
#define DP_LANE_CHANNEL_EQ_DONE		(1 << 1)
#define DP_LANE_SYMBOL_LOCKED		(1 << 2)
#define DP_CHANNEL_EQ_BITS (DP_LANE_CR_DONE |           \
			    DP_LANE_CHANNEL_EQ_DONE |   \
			    DP_LANE_SYMBOL_LOCKED)

#define DP_LAINE_ALIGN_STATUS_UPDATED	0x204
#define DP_INTERLANE_ALIGN_DONE		(1 << 0)

#define DP_LINK_SCRAMBLING_DISABLE      0x20
#define DP_TRAINING_PATTERN_1		1
#define DP_TRAINING_PATTERN_2		2

#define DP_EDP_CONFIGURATION_SET		0x10a
#define DP_ALTERNATE_SCRAMBLER_RESET_ENABLE     (1 << 0)

/* Registers */

/* Display Parallel Interface */
#define DPIPXLFMT		0x0440
#define VS_POL_ACTIVE_LOW		(1 << 10)
#define HS_POL_ACTIVE_LOW		(1 << 9)
#define DE_POL_ACTIVE_HIGH		(0 << 8)
#define SUB_CFG_TYPE_CONFIG1		(0 << 2) /* LSB aligned */
#define SUB_CFG_TYPE_CONFIG2		(1 << 2) /* Loosely Packed */
#define SUB_CFG_TYPE_CONFIG3		(2 << 2) /* LSB aligned 8-bit */
#define DPI_BPP_RGB888			(0 << 0)
#define DPI_BPP_RGB666			(1 << 0)
#define DPI_BPP_RGB565			(2 << 0)

/* Video Path */
#define VPCTRL0			0x0450
#define OPXLFMT_RGB666			(0 << 8)
#define OPXLFMT_RGB888			(1 << 8)
#define FRMSYNC_DISABLED		(0 << 4) /* Video Timing Gen Disabled */
#define FRMSYNC_ENABLED			(1 << 4) /* Video Timing Gen Enabled */
#define MSF_DISABLED			(0 << 0) /* Magic Square FRC disabled */
#define MSF_ENABLED			(1 << 0) /* Magic Square FRC enabled */
#define HTIM01			0x0454
#define HTIM02			0x0458
#define VTIM01			0x045c
#define VTIM02			0x0460
#define VFUEN0			0x0464
#define VFUEN				BIT(0)   /* Video Frame Timing Upload */

/* System */
#define TC_IDREG		0x0500
#define SYSCTRL			0x0510
#define DP0_AUDSRC_NO_INPUT		(0 << 3)
#define DP0_AUDSRC_I2S_RX		(1 << 3)
#define DP0_VIDSRC_NO_INPUT		(0 << 0)
#define DP0_VIDSRC_DSI_RX		(1 << 0)
#define DP0_VIDSRC_DPI_RX		(2 << 0)
#define DP0_VIDSRC_COLOR_BAR		(3 << 0)

/* Control */
#define DP0CTL			0x0600
#define VID_MN_GEN			BIT(6)   /* Auto-generate M/N values */
#define EF_EN				BIT(5)   /* Enable Enhanced Framing */
#define VID_EN				BIT(1)   /* Video transmission enable */
#define DP_EN				BIT(0)   /* Enable DPTX function */

/* Clocks */
#define DP0_VIDMNGEN0		0x0610
#define DP0_VIDMNGEN1		0x0614
#define DP0_VMNGENSTATUS	0x0618

/* Main Channel */
#define DP0_SECSAMPLE		0x0640
#define DP0_VIDSYNCDELAY	0x0644
#define DP0_TOTALVAL		0x0648
#define DP0_STARTVAL		0x064c
#define DP0_ACTIVEVAL		0x0650
#define DP0_SYNCVAL		0x0654
#define DP0_MISC		0x0658
#define TU_SIZE_RECOMMENDED		(63) /* LSCLK cycles per TU */
#define BPC_6				(0 << 5)
#define BPC_8				(1 << 5)

/* AUX channel */
#define DP0_AUXCFG0		0x0660
#define DP0_AUXCFG1		0x0664
#define AUX_RX_FILTER_EN		BIT(16)

#define DP0_AUXADDR		0x0668
#define DP0_AUXWDATA(i)		(0x066c + (i) * 4)
#define DP0_AUXRDATA(i)		(0x067c + (i) * 4)
#define DP0_AUXSTATUS		0x068c
#define AUX_STATUS_MASK			0xf0
#define AUX_STATUS_SHIFT		4
#define AUX_TIMEOUT			BIT(1)
#define AUX_BUSY			BIT(0)
#define DP0_AUXI2CADR		0x0698

/* Link Training */
#define DP0_SRCCTRL		0x06a0
#define DP0_SRCCTRL_SCRMBLDIS		BIT(13)
#define DP0_SRCCTRL_EN810B		BIT(12)
#define DP0_SRCCTRL_NOTP		(0 << 8)
#define DP0_SRCCTRL_TP1			(1 << 8)
#define DP0_SRCCTRL_TP2			(2 << 8)
#define DP0_SRCCTRL_LANESKEW		BIT(7)
#define DP0_SRCCTRL_SSCG		BIT(3)
#define DP0_SRCCTRL_LANES_1		(0 << 2)
#define DP0_SRCCTRL_LANES_2		(1 << 2)
#define DP0_SRCCTRL_BW27		(1 << 1)
#define DP0_SRCCTRL_BW162		(0 << 1)
#define DP0_SRCCTRL_AUTOCORRECT		BIT(0)
#define DP0_LTSTAT		0x06d0
#define LT_LOOPDONE			BIT(13)
#define LT_STATUS_MASK			(0x1f << 8)
#define LT_CHANNEL1_EQ_BITS		(DP_CHANNEL_EQ_BITS << 4)
#define LT_INTERLANE_ALIGN_DONE		BIT(3)
#define LT_CHANNEL0_EQ_BITS		(DP_CHANNEL_EQ_BITS)
#define DP0_SNKLTCHGREQ		0x06d4
#define DP0_LTLOOPCTRL		0x06d8
#define DP0_SNKLTCTRL		0x06e4

/* PHY */
#define DP_PHY_CTRL		0x0800
#define DP_PHY_RST			BIT(28)  /* DP PHY Global Soft Reset */
#define BGREN				BIT(25)  /* AUX PHY BGR Enable */
#define PWR_SW_EN			BIT(24)  /* PHY Power Switch Enable */
#define PHY_M1_RST			BIT(12)  /* Reset PHY1 Main Channel */
#define PHY_RDY				BIT(16)  /* PHY Main Channels Ready */
#define PHY_M0_RST			BIT(8)   /* Reset PHY0 Main Channel */
#define PHY_A0_EN			BIT(1)   /* PHY Aux Channel0 Enable */
#define PHY_M0_EN			BIT(0)   /* PHY Main Channel0 Enable */

/* PLL */
#define DP0_PLLCTRL		0x0900
#define DP1_PLLCTRL		0x0904	/* not defined in DS */
#define PXL_PLLCTRL		0x0908
#define PLLUPDATE			BIT(2)
#define PLLBYP				BIT(1)
#define PLLEN				BIT(0)
#define PXL_PLLPARAM		0x0914
#define IN_SEL_REFCLK			(0 << 14)
#define SYS_PLLPARAM		0x0918
#define REF_FREQ_38M4			(0 << 8) /* 38.4 MHz */
#define REF_FREQ_19M2			(1 << 8) /* 19.2 MHz */
#define REF_FREQ_26M			(2 << 8) /* 26 MHz */
#define REF_FREQ_13M			(3 << 8) /* 13 MHz */
#define SYSCLK_SEL_LSCLK		(0 << 4)
#define LSCLK_DIV_1			(0 << 0)
#define LSCLK_DIV_2			(1 << 0)

/* Test & Debug */
#define TSTCTL			0x0a00
#define PLL_DBG			0x0a04

struct tc_edp_link {
	u8			rate;
	u8			rev;
	u8			lanes;
	u8			enhanced;
	u8			assr;
	int			scrambler_dis;
	int			spread;
	int			coding8b10b;
	u8			swing;
	u8			preemp;
};

struct tc_data {
	struct i2c_client	*client;
	struct device_d		*dev;
	/* DP AUX channel */
	struct i2c_adapter	adapter;
	struct vpl		vpl;

	/* link settings */
	struct tc_edp_link	link;

	/* mode */
	struct fb_videomode	*mode;

	u32			rev;
	u8			assr;

	char			*edid;

	int			sd_gpio;
	int			sd_active_high;
	int			reset_gpio;
	int			reset_active_high;
	struct clk		*refclk;
};
#define to_tc_i2c_struct(a)	container_of(a, struct tc_data, adapter)

static int tc_write_reg(struct tc_data *data, u16 reg, u32 value)
{
	int ret;
	u8 buf[4];

	buf[0] = value & 0xff;
	buf[1] = (value >> 8) & 0xff;
	buf[2] = (value >> 16) & 0xff;
	buf[3] = (value >> 24) & 0xff;

	ret = i2c_write_reg(data->client, reg | I2C_ADDR_16_BIT, buf, 4);
	if (ret != 4) {
		dev_err(data->dev, "error writing reg 0x%04x: %d\n",
			reg, ret);
		return ret;
	}

	return 0;
}


static int tc_read_reg(struct tc_data *data, u16 reg, u32 *value)
{
	int ret;
	u8 buf[4];

	ret = i2c_read_reg(data->client, reg | I2C_ADDR_16_BIT, buf, 4);
	if (ret != 4) {
		dev_err(data->dev, "error reading reg 0x%04x: %d\n",
			reg, ret);
		return ret;
	}

	*value = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);

	return 0;
}

/* simple macros to avoid error checks */
#define tc_write(reg, var)	do {		\
	ret = tc_write_reg(tc, reg, var);	\
	if (ret)				\
		goto err;			\
	} while (0)
#define tc_read(reg, var)	do {		\
	ret = tc_read_reg(tc, reg, var);	\
	if (ret)				\
		goto err;			\
	} while (0)

static int tc_aux_get_status(struct tc_data *tc)
{
	int ret;
	u32 value;

	tc_read(DP0_AUXSTATUS, &value);
	if ((value & 0x01) == 0x00) {
		switch (value & 0xf0) {
		case 0x00:
			/* Ack */
			return 0;
		case 0x40:
			/* Nack */
			return -EIO;
		case 0x80:
			dev_err(tc->dev, "i2c defer\n");
			return -EAGAIN;
		}
		return 0;
	}

	if (value & 0x02) {
		dev_err(tc->dev, "i2c access timeout!\n");
		return -ETIME;
	}
	return -EBUSY;
err:
	return ret;
}

static int tc_aux_wait_busy(struct tc_data *tc, unsigned int timeout_ms)
{
	int ret;
	u32 value;

	do {
		tc_read(DP0_AUXSTATUS, &value);
		if ((value & AUX_BUSY) == 0x00)
			return 0;
		mdelay(1);
	} while (timeout_ms--);

	return -EBUSY;
err:
	return ret;
}

static int tc_aux_read(struct tc_data *tc, int reg, char *data, int size)
{
	int i = 0;
	int ret;
	u32 tmp;

	ret = tc_aux_wait_busy(tc, 100);
	if (ret)
		goto err;

	/* store address */
	tc_write(DP0_AUXADDR, reg);
	/* start transfer */
	tc_write(DP0_AUXCFG0, ((size - 1) << 8) | 0x09);

	ret = tc_aux_wait_busy(tc, 100);
	if (ret)
		goto err;

	ret = tc_aux_get_status(tc);
	if (ret)
		goto err;

	/* read data */
	while (i < size) {
		if ((i % 4) == 0)
			tc_read(DP0_AUXRDATA(i >> 2), &tmp);
		data[i] = tmp & 0xFF;
		tmp = tmp >> 8;
		i++;
	}

	return 0;
err:
	dev_err(tc->dev, "tc_aux_read error: %d\n", ret);
	return ret;
}

static int tc_aux_write(struct tc_data *tc, int reg, char *data, int size)
{
	int i = 0;
	int ret;
	u32 tmp = 0;

	ret = tc_aux_wait_busy(tc, 100);
	if (ret)
		goto err;

	i = 0;
	/* store data */
	while (i < size) {
		tmp = tmp | (data[i] << (8 * (i & 0x03)));
		i++;
		if (((i % 4) == 0) ||
		    (i == size)) {
			tc_write(DP0_AUXWDATA((i - 1) >> 2), tmp);
			tmp = 0;
		}
	}
	/* store address */
	tc_write(DP0_AUXADDR, reg);
	/* start transfer */
	tc_write(DP0_AUXCFG0, ((size - 1) << 8) | 0x08);

	ret = tc_aux_wait_busy(tc, 100);
	if (ret)
		goto err;

	ret = tc_aux_get_status(tc);
	if (ret)
		goto err;

	return 0;
err:
	dev_err(tc->dev, "tc_aux_write error: %d\n", ret);
	return ret;
}

static int tc_aux_i2c_read(struct tc_data *tc, struct i2c_msg *msg)
{
	int i = 0;
	int ret;
	u32 tmp;

	ret = tc_aux_wait_busy(tc, 100);
	if (ret)
		goto err;

	/* store address */
	tc_write(DP0_AUXADDR, msg->addr);

	/* start transfer */
	tc_write(DP0_AUXCFG0, ((msg->len - 1) << 8) | 0x01);

	ret = tc_aux_wait_busy(tc, 100);
	if (ret)
		goto err;

	ret = tc_aux_get_status(tc);
	if (ret)
		goto err;

	/* read data */
	while (i < msg->len) {
		if ((i % 4) == 0)
			tc_read(DP0_AUXRDATA(i >> 2), &tmp);
		msg->buf[i] = tmp & 0xFF;
		tmp = tmp >> 8;
		i++;
	}

	return 0;
err:
	return ret;
}

static int tc_aux_i2c_write(struct tc_data *tc, struct i2c_msg *msg)
{
	int i = 0;
	int ret;
	u32 tmp = 0;

	ret = tc_aux_wait_busy(tc, 100);
	if (ret)
		goto err;

	/* store data */
	while (i < msg->len) {
		tmp = (tmp << 8) | msg->buf[i];
		i++;
		if (((i % 4) == 0) ||
		    (i == msg->len)) {
			tc_write(DP0_AUXWDATA((i - 1) >> 2), tmp);
			tmp = 0;
		}
	}
	/* store address */
	tc_write(DP0_AUXADDR, msg->addr);
	/* start transfer */
	tc_write(DP0_AUXCFG0, ((msg->len - 1) << 8) | 0x00);

	ret = tc_aux_wait_busy(tc, 100);
	if (ret)
		goto err;

	ret = tc_aux_get_status(tc);
	if (ret)
		goto err;

	return 0;
err:
	return ret;
}

static int tc_aux_i2c_xfer(struct i2c_adapter *adapter,
			struct i2c_msg *msgs, int num)
{
	struct tc_data *tc = to_tc_i2c_struct(adapter);
	unsigned int i;
	int ret;

	/* check */
	for (i = 0; i < num; i++) {
		if (msgs[i].len > 16) {
			dev_err(tc->dev, "this bus support max 16 bytes per transfer\n");
			return -EINVAL;
		}
		if (msgs[i].flags & I2C_M_DATA_ONLY)
			return -EINVAL;
	}

	/* read/write data */
	for (i = 0; i < num; i++) {
		/* write/read data */
		if (msgs[i].flags & I2C_M_RD)
			ret = tc_aux_i2c_read(tc, &msgs[i]);
		else
			ret = tc_aux_i2c_write(tc, &msgs[i]);
		if (ret)
			goto err;
	}

err:
	return (ret < 0) ? ret : num;
}

static const char * const training_pattern1_errors[] = {
	"No errors",
	"Aux write error",
	"Aux read error",
	"Max voltage reached error",
	"Loop counter expired error",
	"res", "res", "res"
};

static const char * const training_pattern2_errors[] = {
	"No errors",
	"Aux write error",
	"Aux read error",
	"Clock recovery failed error",
	"Loop counter expired error",
	"res", "res", "res"
};

static u32 tc_srcctrl(struct tc_data *tc)
{
	/*
	 * No training pattern, skew lane 1 data by two LSCLK cycles with
	 * respect to lane 0 data, AutoCorrect Mode = 0
	 */
	u32 reg = DP0_SRCCTRL_NOTP | DP0_SRCCTRL_LANESKEW;

	if (tc->link.scrambler_dis)
		reg |= DP0_SRCCTRL_SCRMBLDIS;	/* Scrambler Disabled */
	if (tc->link.coding8b10b)
		/* Enable 8/10B Encoder (TxData[19:16] not used) */
		reg |= DP0_SRCCTRL_EN810B;
	if (tc->link.spread)
		reg |= DP0_SRCCTRL_SSCG;	/* Spread Spectrum Enable */
	if (tc->link.lanes == 2)
		reg |= DP0_SRCCTRL_LANES_2;	/* Two Main Channel Lanes */
	if (tc->link.rate != 0x06)
		reg |= DP0_SRCCTRL_BW27;	/* 2.7 Gbps link */
	return reg;
}

static void tc_wait_pll_lock(struct tc_data *tc)
{
	/* Wait for PLL to lock: up to 2.09 ms, depending on refclk */
	mdelay(100);
}

static int tc_stream_clock_calc(struct tc_data *tc)
{
	int ret;
	/*
	 * If the Stream clock and Link Symbol clock are
	 * asynchronous with each other, the value of M changes over
	 * time. This way of generating link clock and stream
	 * clock is called Asynchronous Clock mode. The value M
	 * must change while the value N stays constant. The
	 * value of N in this Asynchronous Clock mode must be set
	 * to 2^15 or 32,768.
	 *
	 * LSCLK = 1/10 of high speed link clock
	 *
	 * f_STRMCLK = M/N * f_LSCLK
	 * M/N = f_STRMCLK / f_LSCLK
	 *
	 */
	tc_write(DP0_VIDMNGEN1, 32768);

	return 0;
err:
	return ret;
}

static int tc_aux_link_setup(struct tc_data *tc)
{
	unsigned long rate;
	u32 value;
	int ret;
	int timeout;

	rate = clk_get_rate(tc->refclk);
	switch (rate) {
	case 38400000:
		value = REF_FREQ_38M4;
		break;
	case 26000000:
		value = REF_FREQ_26M;
		break;
	case 19200000:
		value = REF_FREQ_19M2;
		break;
	case 13000000:
		value = REF_FREQ_13M;
		break;
	default:
		dev_err(tc->dev, "Invalid refclk rate: %lu Hz\n", rate);
		return -EINVAL;
	}

	/* Setup DP-PHY / PLL */
	value |= SYSCLK_SEL_LSCLK | LSCLK_DIV_2;
	tc_write(SYS_PLLPARAM, value);

	tc_write(DP_PHY_CTRL, BGREN | PWR_SW_EN | BIT(2) | PHY_A0_EN);

	/*
	 * Initially PLLs are in bypass. Force PLL parameter update,
	 * disable PLL bypass, enable PLL
	 */
	tc_write(DP0_PLLCTRL, PLLUPDATE | PLLEN);
	tc_wait_pll_lock(tc);

	tc_write(DP1_PLLCTRL, PLLUPDATE | PLLEN);
	tc_wait_pll_lock(tc);

	timeout = 1000;
	do {
		tc_read(DP_PHY_CTRL, &value);
		udelay(1);
	} while ((!(value & (1 << 16))) && (--timeout));

	if (timeout == 0) {
		dev_err(tc->dev, "Timeout waiting for PHY to become ready");
		return -ETIMEDOUT;
	}

	/* Setup AUX link */
	tc_write(DP0_AUXCFG1, AUX_RX_FILTER_EN |
		 (0x06 << 8) |	/* Aux Bit Period Calculator Threshold */
		 (0x3f << 0));	/* Aux Response Timeout Timer */

	return 0;
err:
	dev_err(tc->dev, "tc_aux_link_setup failed: %d\n", ret);
	return ret;
}

static int tc_get_display_props(struct tc_data *tc)
{
	int ret;
	/* temp buffer */
	u8 tmp[8];

	/* Read DP Rx Link Capability */
	ret = tc_aux_read(tc, 0x000, tmp, 8);
	if (ret)
		goto err_dpcd_read;

	tc->assr = !(tc->rev & 0x02);

	/* check DPCD rev */
	if (tmp[0] < 0x10) {
		dev_err(tc->dev, "Too low DPCD revision 0x%02x\n", tmp[0]);
		goto err_dpcd_inval;
	}
	if ((tmp[0] != 0x10) && (tmp[0] != 0x11))
		dev_warn(tc->dev, "Unknown DPCD revision 0x%02x\n", tmp[0]);
	tc->link.rev = tmp[0];

	/* check rate */
	if ((tmp[1] == 0x06) || (tmp[1] == 0x0a)) {
		tc->link.rate = tmp[1];
	} else {
		dev_warn(tc->dev, "Unknown link rate 0x%02x, falling to 2.7Gbps\n", tmp[1]);
		tc->link.rate = 0x0a;
	}
	tc->link.lanes = tmp[2] & 0x0f;
	if (tc->link.lanes > 2) {
		dev_dbg(tc->dev, "Display supports %d lanes, host only 2 max. "
			"Tryin 2-lane mode.\n",
			tc->link.lanes);
		tc->link.lanes = 2;
	}
	tc->link.enhanced = !!(tmp[2] & 0x80);
	tc->link.spread = tmp[3] & 0x01;
	tc->link.coding8b10b = tmp[6] & 0x01;
	tc->link.scrambler_dis = 0;
	/* read assr */
	ret = tc_aux_read(tc, DP_EDP_CONFIGURATION_SET, tmp, 1);
	if (ret)
		goto err_dpcd_read;
	tc->link.assr = tmp[0] & DP_ALTERNATE_SCRAMBLER_RESET_ENABLE;

	dev_dbg(tc->dev, "DPCD rev: %d.%d, rate: %s, lanes: %d, framing: %s\n",
		tc->link.rev >> 4,
		tc->link.rev & 0x0f,
		(tc->link.rate == 0x06) ? "1.62Gbps" : "2.7Gbps",
		tc->link.lanes,
		tc->link.enhanced ? "enhanced" : "non-enhanced");
	dev_dbg(tc->dev, "ANSI 8B/10B: %d\n", tc->link.coding8b10b);
	dev_dbg(tc->dev, "Display ASSR: %d, TC358767 ASSR: %d\n",
		tc->link.assr, tc->assr);

	return 0;

err_dpcd_read:
	dev_err(tc->dev, "failed to read DPCD: %d\n", ret);
	return ret;
err_dpcd_inval:
	dev_err(tc->dev, "invalid DPCD\n");
	return -EINVAL;
}

static int tc_set_video_mode(struct tc_data *tc, struct fb_videomode *mode)
{
	int ret;
	u32 reg;
	int htotal;
	int vtotal;
	int vid_sync_dly;
	int max_tu_symbol = TU_SIZE_RECOMMENDED - 1;

	htotal = mode->hsync_len + mode->left_margin + mode->xres +
		mode->right_margin;
	vtotal = mode->vsync_len + mode->upper_margin + mode->yres +
		mode->lower_margin;

	dev_dbg(tc->dev, "set mode %dx%d\n", mode->xres, mode->yres);
	dev_dbg(tc->dev, "H margin %d,%d sync %d\n",
		mode->left_margin, mode->right_margin, mode->hsync_len);
	dev_dbg(tc->dev, "V margin %d,%d sync %d\n",
		mode->upper_margin, mode->lower_margin, mode->vsync_len);
	dev_dbg(tc->dev, "total: %dx%d\n", htotal, vtotal);

	/*
	 * Datasheet is not clear of vsdelay in case of DPI
	 * assume we do not need any delay when DPI is a source of
	 * sync signals
	 */
	tc_write(VPCTRL0, (0 << 20) /* VSDELAY */ |
		 OPXLFMT_RGB888 | FRMSYNC_DISABLED | MSF_DISABLED);
	/* LCD Ctl Frame Size */
	tc_write(HTIM01, (ALIGN(mode->left_margin, 2) << 16) |	/* H back porch */
			 (ALIGN(mode->hsync_len, 2) << 0));	/* Hsync */
	tc_write(HTIM02, (ALIGN(mode->right_margin, 2) << 16) |	/* H front porch */
			 (ALIGN(mode->xres, 2) << 0));		/* width */
	tc_write(VTIM01, (mode->upper_margin << 16) |		/* V back porch */
			 (mode->vsync_len << 0));		/* Vsync */
	tc_write(VTIM02, (mode->lower_margin << 16) |		/* V front porch */
			 (mode->yres << 0));			/* height */
	tc_write(VFUEN0, VFUEN);		/* update settings */

	/* Test pattern settings */
	tc_write(TSTCTL,
		 (120 << 24) |	/* Red Color component value */
		 (20 << 16) |	/* Green Color component value */
		 (99 << 8) |	/* Blue Color component value */
		 (1 << 4) |	/* Enable I2C Filter */
		 (2 << 0) |	/* Color bar Mode */
		 0);

	/* DP Main Stream Attributes */
	vid_sync_dly = mode->hsync_len + mode->left_margin + mode->xres;
	tc_write(DP0_VIDSYNCDELAY,
		 (max_tu_symbol << 16) |	/* thresh_dly */
		 (vid_sync_dly << 0));

	tc_write(DP0_TOTALVAL, (vtotal << 16) | (htotal));

	tc_write(DP0_STARTVAL,
		 ((mode->upper_margin + mode->vsync_len) << 16) |
		 ((mode->left_margin + mode->hsync_len) << 0));

	tc_write(DP0_ACTIVEVAL, (mode->yres << 16) | (mode->xres));

	tc_write(DP0_SYNCVAL, (mode->vsync_len << 16) | (mode->hsync_len << 0));

	reg = DE_POL_ACTIVE_HIGH | SUB_CFG_TYPE_CONFIG1 | DPI_BPP_RGB888;
	if (!(mode->sync & FB_SYNC_VERT_HIGH_ACT))
		reg |= VS_POL_ACTIVE_LOW;
	if (!(mode->sync & FB_SYNC_HOR_HIGH_ACT))
		reg |= HS_POL_ACTIVE_LOW;
	tc_write(DPIPXLFMT, reg);

	/*
	 * Recommended maximum number of symbols transferred in a transfer unit:
	 * DIV_ROUND_UP((input active video bandwidth in bytes) * tu_size,
	 *              (output active video bandwidth in bytes))
	 * Must be less than tu_size.
	 */
	tc_write(DP0_MISC, (max_tu_symbol << 23) | (TU_SIZE_RECOMMENDED << 16) | BPC_8);

	return 0;
err:
	return ret;
}

static int tc_link_training(struct tc_data *tc, int pattern)
{
	const char * const *errors;
	u32 srcctrl = tc_srcctrl(tc) | DP0_SRCCTRL_SCRMBLDIS |
		      DP0_SRCCTRL_AUTOCORRECT;
	int timeout;
	int retry;
	u32 value;
	int ret;

	if (pattern == DP_TRAINING_PATTERN_1) {
		srcctrl |= DP0_SRCCTRL_TP1;
		errors = training_pattern1_errors;
	} else {
		srcctrl |= DP0_SRCCTRL_TP2;
		errors = training_pattern2_errors;
	}

	/* Set DPCD 0x102 for Training Part 1 or 2 */
	tc_write(DP0_SNKLTCTRL, DP_LINK_SCRAMBLING_DISABLE | pattern);

	tc_write(DP0_LTLOOPCTRL,
		 (0x0f << 28) |	/* Defer Iteration Count */
		 (0x0f << 24) |	/* Loop Iteration Count */
		 (0x0d << 0));	/* Loop Timer Delay */

	retry = 5;
	do {
		/* Set DP0 Training Pattern */
		tc_write(DP0_SRCCTRL, srcctrl);

		/* Enable DP0 to start Link Training */
		tc_write(DP0CTL, DP_EN);

		/* wait */
		timeout = 1000;
		do {
			tc_read(DP0_LTSTAT, &value);
			udelay(1);
		} while ((!(value & LT_LOOPDONE)) && (--timeout));
		if (timeout == 0) {
			dev_err(tc->dev, "Link training timeout!\n");
		} else {
			int pattern = (value >> 11) & 0x3;
			int error = (value >> 8) & 0x7;

			dev_dbg(tc->dev,
				"Link training phase %d done after %d uS: %s\n",
				pattern, 1000 - timeout, errors[error]);
			if (pattern == DP_TRAINING_PATTERN_1 && error == 0)
				break;
			if (pattern == DP_TRAINING_PATTERN_2) {
				value &= LT_CHANNEL1_EQ_BITS |
					 LT_INTERLANE_ALIGN_DONE |
					 LT_CHANNEL0_EQ_BITS;
				/* in case of two lanes */
				if ((tc->link.lanes == 2) &&
				    (value == (LT_CHANNEL1_EQ_BITS |
					       LT_INTERLANE_ALIGN_DONE |
					       LT_CHANNEL0_EQ_BITS)))
					break;
				/* in case of one line */
				if ((tc->link.lanes == 1) &&
				    (value == (LT_INTERLANE_ALIGN_DONE |
					       LT_CHANNEL0_EQ_BITS)))
					break;
			}
		}
		/* restart */
		tc_write(DP0CTL, 0);
		udelay(10);
	} while (--retry);
	if (retry == 0) {
		dev_err(tc->dev, "Failed to finish training phase %d\n",
			pattern);
	}

	return 0;
err:
	return ret;
}

static int tc_main_link_setup(struct tc_data *tc)
{
	struct device_d *dev = tc->dev;
	unsigned int rate;
	u32 dp_phy_ctrl;
	int timeout;
	bool aligned;
	bool ready;
	u32 value;
	int ret;
	u8 tmp[16];

	/* display mode should be set at this point */
	if (!tc->mode)
		return -EINVAL;

	/* from excel file - DP0_SrcCtrl */
	tc_write(DP0_SRCCTRL, DP0_SRCCTRL_SCRMBLDIS | DP0_SRCCTRL_EN810B |
		 DP0_SRCCTRL_LANESKEW | DP0_SRCCTRL_LANES_2 |
		 DP0_SRCCTRL_BW27 | DP0_SRCCTRL_AUTOCORRECT);
	/* from excel file - DP1_SrcCtrl */
	tc_write(0x07a0, 0x00003083);

	rate = clk_get_rate(tc->refclk);
	switch (rate) {
	case 38400000:
		value = REF_FREQ_38M4;
		break;
	case 26000000:
		value = REF_FREQ_26M;
		break;
	case 19200000:
		value = REF_FREQ_19M2;
		break;
	case 13000000:
		value = REF_FREQ_13M;
		break;
	default:
		return -EINVAL;
	}
	value |= SYSCLK_SEL_LSCLK | LSCLK_DIV_2;
	tc_write(SYS_PLLPARAM, value);
	/* Setup Main Link */
	dp_phy_ctrl = BGREN | PWR_SW_EN | BIT(2) | PHY_A0_EN |  PHY_M0_EN;
	tc_write(DP_PHY_CTRL, dp_phy_ctrl);
	mdelay(100);

	/* PLL setup */
	tc_write(DP0_PLLCTRL, PLLUPDATE | PLLEN);
	tc_wait_pll_lock(tc);

	tc_write(DP1_PLLCTRL, PLLUPDATE | PLLEN);
	tc_wait_pll_lock(tc);

	/* Reset/Enable Main Links */
	dp_phy_ctrl |= DP_PHY_RST | PHY_M1_RST | PHY_M0_RST;
	tc_write(DP_PHY_CTRL, dp_phy_ctrl);
	udelay(100);
	dp_phy_ctrl &= ~(DP_PHY_RST | PHY_M1_RST | PHY_M0_RST);
	tc_write(DP_PHY_CTRL, dp_phy_ctrl);

	timeout = 1000;
	do {
		tc_read(DP_PHY_CTRL, &value);
		udelay(1);
	} while ((!(value & PHY_RDY)) && (--timeout));

	if (timeout == 0) {
		dev_err(dev, "timeout waiting for phy become ready");
		return -ETIMEDOUT;
	}

	/* Set misc: 8 bits per color */
	tc_read(DP0_MISC, &value);
	value |= BPC_8;
	tc_write(DP0_MISC, value);

	/*
	 * ASSR mode
	 * on TC358767 side ASSR configured through strap pin
	 * seems there is no way to change this setting from SW
	 *
	 * check is tc configured for same mode
	 */
	if (tc->assr != tc->link.assr) {
		dev_dbg(dev, "Trying to set display to ASSR: %d\n",
			tc->assr);
		/* try to set ASSR on display side */
		tmp[0] = tc->assr;
		ret = tc_aux_write(tc, DP_EDP_CONFIGURATION_SET, tmp, 1);
		if (ret)
			goto err_dpcd_read;
		/* read back */
		ret = tc_aux_read(tc, DP_EDP_CONFIGURATION_SET, tmp, 1);
		if (ret)
			goto err_dpcd_read;

		if (tmp[0] != tc->assr) {
			dev_dbg(dev, "Failed to switch display ASSR to %d, falling back to unscrambled mode\n",
				 tc->assr);
			/* trying with disabled scrambler */
			tc->link.scrambler_dis = 1;
		}
	}

	/* Setup Link & DPRx Config for Training */
	/* LINK_BW_SET */
	tmp[0] = tc->link.rate;
	/* LANE_COUNT_SET */
	tmp[1] = tc->link.lanes;
	if (tc->link.enhanced)
		tmp[1] |= DP_ENHANCED_FRAME_EN;

	/* TRAINING_PATTERN_SET */
	tmp[2] = 0x00;
	/* TRAINING_LANE0_SET .. TRAINING_LANE3_SET */
	tmp[3] = 0x00;
	tmp[4] = 0x00;
	tmp[5] = 0x00;
	tmp[6] = 0x00;
	/* DOWNSPREAD_CTRL */
	tmp[7] = tc->link.spread ? DP_SPREAD_AMP_0_5 : 0x00;
	/* MAIN_LINK_CHANNEL_CODING_SET */
	tmp[8] = tc->link.coding8b10b ? DP_SET_ANSI_8B10B : 0x00;

	/* DP_LINK_BW_SET .. MAIN_LINK_CHANNEL_CODING_SET */
	ret = tc_aux_write(tc, DP_LINK_BW_SET, tmp, 9);
	if (ret)
		goto err_dpcd_write;

	/* check lanes */
	ret =  tc_aux_read(tc, DP_LINK_BW_SET, tmp, 2);
	if (ret)
		goto err_dpcd_read;

	if ((tmp[1] & 0x0f) != tc->link.lanes) {
		dev_err(dev, "Failed to set lanes = %d on display side\n",
			tc->link.lanes);
		goto err;
	}

	ret = tc_link_training(tc, DP_TRAINING_PATTERN_1);
	if (ret)
		goto err;

	ret = tc_link_training(tc, DP_TRAINING_PATTERN_2);
	if (ret)
		goto err;

	/* Clear DPCD 0x102 */
	/* Note: Can Not use DP0_SNKLTCTRL (0x06E4) short cut */
	tmp[0] = tc->link.scrambler_dis ? DP_LINK_SCRAMBLING_DISABLE : 0x00;
	ret = tc_aux_write(tc, DP_TRAINING_PATTERN_SET, tmp, 1);
	if (ret)
		goto err_dpcd_write;

	/* Clear Training Pattern, set AutoCorrect Mode = 1 */
	tc_write(DP0_SRCCTRL, tc_srcctrl(tc) | DP0_SRCCTRL_AUTOCORRECT);

	/* Wait */
	timeout = 100;
	do {
		udelay(1);
		/* Read DPCD 0x200-0x206 */
		ret = tc_aux_read(tc, 0x200, tmp, 7);
		if (ret)
			goto err_dpcd_read;
		if (tc->link.lanes == 1)
			ready = (tmp[2] == DP_CHANNEL_EQ_BITS);         /* Lane0 */
		else
			ready = (tmp[2] == ((DP_CHANNEL_EQ_BITS << 4) | /* Lane1 */
					     DP_CHANNEL_EQ_BITS));      /* Lane0 */
		aligned = tmp[4] & DP_INTERLANE_ALIGN_DONE;
	} while ((--timeout) && !(ready && aligned));

	if (timeout == 0) {
		dev_info(dev, "0x0200 SINK_COUNT: 0x%02x\n", tmp[0]);
		dev_info(dev, "0x0201 DEVICE_SERVICE_IRQ_VECTOR: 0x%02x\n",
			 tmp[1]);
		dev_info(dev, "0x0202 LANE0_1_STATUS: 0x%02x\n", tmp[2]);
		dev_info(dev, "0x0204 LANE_ALIGN_STATUS_UPDATED: 0x%02x\n",
			 tmp[4]);
		dev_info(dev, "0x0205 SINK_STATUS: 0x%02x\n", tmp[5]);
		dev_info(dev, "0x0206 ADJUST_REQUEST_LANE0_1: 0x%02x\n",
			 tmp[6]);

		if (!ready)
			dev_err(dev, "Lane0/1 not ready\n");
		if (!aligned)
			dev_err(dev, "Lane0/1 not aligned\n");
		return -EAGAIN;
	}

	ret = tc_set_video_mode(tc, tc->mode);
	if (ret)
		goto err;

	/* Set M/N */
	ret = tc_stream_clock_calc(tc);
	if (ret)
		goto err;

	return 0;
err_dpcd_read:
	dev_err(tc->dev, "Failed to read DPCD: %d\n", ret);
	return ret;
err_dpcd_write:
	dev_err(tc->dev, "Failed to write DPCD: %d\n", ret);
err:
	return ret;
}

static int tc_main_link_stream(struct tc_data *tc, int state)
{
	int ret;
	u32 value;

	dev_dbg(tc->dev, "stream: %d\n", state);

	if (state) {
		value = VID_MN_GEN | DP_EN;
		if (tc->link.enhanced)
			value |= EF_EN;
		tc_write(DP0CTL, value);
		/*
		 * VID_EN assertion should be delayed by at least N * LSCLK
		 * cycles from the time VID_MN_GEN is enabled in order to
		 * generate stable values for VID_M. LSCLK is 270 MHz or
		 * 162 MHz, VID_N is set to 32768 in  tc_stream_clock_calc(),
		 * so a delay of at least 203 us should suffice.
		 */
		mdelay(1);
		value |= VID_EN;
		tc_write(DP0CTL, value);
		/* Set input interface, currently DPI only */
		value = DP0_AUDSRC_NO_INPUT | DP0_VIDSRC_DPI_RX;
		tc_write(SYSCTRL, value);
	} else {
		tc_write(DP0CTL, 0);
	}

	return 0;
err:
	return ret;
}

#define DDC_BLOCK_READ		5
#define DDC_SEGMENT_ADDR	0x30
#define DDC_ADDR		0x50
#define EDID_LENGTH		0x80

static int tc_read_edid(struct tc_data *tc)
{
	int i = 0;
	int ret;
	int block;
	unsigned char start = 0;
	unsigned char segment = 0;

	struct i2c_msg msg_segment[] = {
		{
			.addr	= DDC_SEGMENT_ADDR,
			.flags	= 0,
			.len	= 1,
			.buf	= &segment,
		}
	};
	struct i2c_msg msgs[] = {
		{
			.addr	= DDC_ADDR,
			.flags	= 0,
			.len	= 1,
			.buf	= &start,
		}, {
			.addr	= DDC_ADDR,
			.flags	= I2C_M_RD,
		}
	};
	tc->edid = xmalloc(EDID_LENGTH);

	/*
	 * Some displays supports E-EDID some not
	 * Just reset segment address to 0x0
	 * This transfer can fail on non E-DCC displays
	 * Ignore error
	 */
	i2c_transfer(&tc->adapter, msg_segment, 1);

	do {
		block = min(DDC_BLOCK_READ, EDID_LENGTH - i);

		msgs[1].buf = tc->edid + i;
		msgs[1].len = block;

		ret = i2c_transfer(&tc->adapter, msgs, 2);
		if (ret < 0)
			goto err;

		i += DDC_BLOCK_READ;
		start = i;
	} while (i < EDID_LENGTH);

#ifdef DEBUG
	printk(KERN_DEBUG "eDP display EDID:\n");
	print_hex_dump(KERN_DEBUG, "", DUMP_PREFIX_OFFSET, 16, 1, tc->edid,
		       EDID_LENGTH, true);
#endif

	return 0;
err:
	free(tc->edid);
	tc->edid = NULL;
	dev_err(tc->dev, "tc_read_edid failed: %d\n", ret);
	return ret;
}

static int tc_filter_videomodes(struct tc_data *tc, struct display_timings *timings)
{
	int i;
	int num_modes = 0;
	struct fb_videomode *mode, *valid_modes;

	valid_modes = xzalloc(timings->num_modes * sizeof(struct fb_videomode));

	/* first filter modes with too high pclock */
	for (i = 0; i < timings->num_modes; i++) {
		mode = &timings->modes[i];

		/* minimum Pixel Clock Period for DPI is 6.5 nS = 6500 pS */
		if (mode->pixclock < 6500) {
			dev_dbg(tc->dev, "%dx%d@%d (%d KHz, flags 0x%08x, sync 0x%08x) skipped\n",
				mode->xres, mode->yres, mode->refresh,
				(int)PICOS2KHZ(mode->pixclock), mode->display_flags,
				mode->sync);
			/* remove from list */
			mode->xres = mode->yres = 0;
		}
	}

	/* then sort from hi to low */
	do {
		int index = -1;

		/* find higest resolution */
		for (i = 0; i < timings->num_modes; i++) {
			mode = &timings->modes[i];
			if (!(mode->xres && mode->yres))
				continue;
			if (index == -1) {
				index = i;
			} else {
				/* compare square */
				if (timings->modes[index].xres * timings->modes[index].yres <
				    mode->xres * mode->yres)
					index = i;
			}
		}

		/* nothing left */
		if (index == -1)
			break;

		/* copy to output list */
		mode = &timings->modes[index];
		memcpy(&valid_modes[num_modes], mode, sizeof(struct fb_videomode));
		mode->xres = mode->yres = 0;
		num_modes++;
	} while (1);

	free(timings->modes);
	timings->modes = NULL;

	if (!num_modes) {
		free(valid_modes);
		return -EINVAL;
	}

	timings->num_modes = num_modes;
	timings->modes = valid_modes;

	dev_dbg(tc->dev, "Valid modes (%d):\n", num_modes);
	for (i = 0; i < timings->num_modes; i++) {
		mode = &timings->modes[i];
		dev_dbg(tc->dev, "%dx%d@%d (%d KHz, flags 0x%08x, sync 0x%08x)\n",
			mode->xres, mode->yres, mode->refresh,
			(int)PICOS2KHZ(mode->pixclock), mode->display_flags,
			mode->sync);
	}

	return 0;
}

static int tc_get_videomodes(struct tc_data *tc, struct display_timings *timings)
{
	int ret;

	/* edid_read_i2c does not work due to limitation of eDP i2c */
	if (!tc->edid) {
		ret = tc_read_edid(tc);
		if (ret) {
			dev_err(tc->dev, "EDID read error: %d\n", ret);
			return ret;
		}
	}

	ret = edid_to_display_timings(timings, tc->edid);
	if (ret < 0) {
		dev_err(tc->dev, "Failed to parse EDID: %d\n", ret);
		return ret;
	}

	/* filter out unsupported due to high pixelxlock */
	ret = tc_filter_videomodes(tc, timings);
	if (ret < 0) {
		dev_err(tc->dev, "No supported modes found\n");
		return ret;
	}

	return ret;
}

static int tc_ioctl(struct vpl *vpl, unsigned int port,
			unsigned int cmd, void *ptr)
{
	struct tc_data *tc = container_of(vpl, struct tc_data, vpl);
	u32 *bus_format;
	int ret = 0;

	switch (cmd) {
	case VPL_PREPARE:
		tc->mode = ptr;
		break;
	case VPL_ENABLE:
		ret = tc_main_link_setup(tc);
		if (ret < 0)
			break;

		ret = tc_main_link_stream(tc, 1);
		break;
	case VPL_DISABLE:
		ret = tc_main_link_stream(tc, 0);
		break;
	case VPL_GET_VIDEOMODES:
		ret = tc_get_videomodes(tc, ptr);
		break;
	case VPL_GET_BUS_FORMAT:
		bus_format = ptr;
		*bus_format = MEDIA_BUS_FMT_RGB888_1X24;
		break;
	default:
		break;
	}

	return ret;
}

static int tc_probe(struct device_d *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct tc_data *tc;
	enum of_gpio_flags flags;
	int ret;

	tc = xzalloc(sizeof(struct tc_data));

	tc->client = client;
	tc->dev = dev;

	/* Shut down GPIO is optional */
	tc->sd_gpio = of_get_named_gpio_flags(dev->device_node,
			"shutdown-gpios", 0, &flags);
	if (gpio_is_valid(tc->sd_gpio)) {
		if (!(flags & OF_GPIO_ACTIVE_LOW))
			tc->sd_active_high = 1;
	}

	/* Reset GPIO is optional */
	tc->reset_gpio = of_get_named_gpio_flags(dev->device_node,
			"reset-gpios", 0, &flags);
	if (gpio_is_valid(tc->reset_gpio)) {
		if (!(flags & OF_GPIO_ACTIVE_LOW))
			tc->reset_active_high = 1;
	}

	if (gpio_is_valid(tc->sd_gpio)) {
		ret = gpio_request(tc->sd_gpio, "tc358767");
		if (ret) {
			dev_err(tc->dev, "SD (%d) can not be requested\n", tc->sd_gpio);
			return ret;
		}
		gpio_direction_output(tc->sd_gpio, 0);
	}

	tc->refclk = of_clk_get_by_name(dev->device_node, "ref");
	if (IS_ERR(tc->refclk)) {
		ret = PTR_ERR(tc->refclk);
		dev_err(dev, "Failed to get refclk: %d\n", ret);
		goto err;
	}

	ret = tc_read_reg(tc, TC_IDREG, &tc->rev);
	if (ret) {
		dev_err(tc->dev, "can not read device ID\n");
		goto err;
	}

	if ((tc->rev != 0x6601) && (tc->rev != 0x6603)) {
		dev_err(tc->dev, "invalid device ID: 0x%08x\n", tc->rev);
		ret = -EINVAL;
		goto err;
	}

	ret = tc_aux_link_setup(tc);
	if (ret)
		goto err;

	/* Register DP AUX channel */
	tc->adapter.master_xfer = tc_aux_i2c_xfer;
	tc->adapter.nr = -1; /* any free */
	tc->adapter.dev.parent = dev;
	tc->adapter.dev.device_node = dev->device_node;
	/* Add I2C adapter */
	ret = i2c_add_numbered_adapter(&tc->adapter);
	if (ret < 0) {
		dev_err(tc->dev, "registration failed\n");
		goto err;
	}

	ret = tc_get_display_props(tc);
	if (ret)
		goto err;

	/* add vlp */
	tc->vpl.node = dev->device_node;
	tc->vpl.ioctl = tc_ioctl;
	return vpl_register(&tc->vpl);

err:
	free(tc);
	return ret;
}

static struct driver_d tc_driver = {
	.name		= "tc358767",
	.probe		= tc_probe,
};

static int tc_init(void)
{
	return i2c_driver_register(&tc_driver);
}
device_initcall(tc_init);
