// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (c) 2015 Realtek Semiconductor Corp. All rights reserved. */

#include <common.h>
#include <dma.h>
#include <errno.h>
#include <init.h>
#include <linux/ethtool.h>
#include <linux/mii.h>
#include <linux/phy.h>
#include <linux/usb/usb.h>
#include <linux/usb/usbnet.h>
#include "r8152.h"

#define R8152_TX_BURST_SIZE	512
#define R8152_RX_BURST_SIZE	64

struct r8152_version {
	unsigned short tcr;
	unsigned short version;
	bool           gmii;
};

static const struct r8152_version r8152_versions[] = {
	{ 0x4c00, RTL_VER_01, 0 },
	{ 0x4c10, RTL_VER_02, 0 },
	{ 0x5c00, RTL_VER_03, 1 },
	{ 0x5c10, RTL_VER_04, 1 },
	{ 0x5c20, RTL_VER_05, 1 },
	{ 0x5c30, RTL_VER_06, 1 },
	{ 0x4800, RTL_VER_07, 0 },
	{ 0x6000, RTL_VER_08, 1 },
	{ 0x6010, RTL_VER_09, 1 },
};

static inline struct r8152 *r8152_get_priv(struct usbnet *dev)
{
	return (struct r8152 *)dev->driver_priv;
}

static int r8152_get_registers(struct r8152 *tp, u16 value, u16 index, u16 size,
			       void *data)
{
	int ret;

	if (WARN_ON(size > R8152_RX_BURST_SIZE))
		return -EINVAL;

	ret = usb_control_msg(tp->udev, usb_rcvctrlpipe(tp->udev, 0),
			      RTL8152_REQ_GET_REGS, RTL8152_REQT_READ,
			      value, index, tp->rxbuf, size, 500);
	memcpy(data, tp->rxbuf, size);

	return ret;
}

static int r8152_set_registers(struct r8152 *tp, u16 value, u16 index, u16 size,
			       const void *data)
{
	int ret;

	if (WARN_ON(size > R8152_TX_BURST_SIZE))
		return -EINVAL;

	memcpy(tp->txbuf, data, size);
	ret = usb_control_msg(tp->udev, usb_sndctrlpipe(tp->udev, 0),
			      RTL8152_REQ_SET_REGS, RTL8152_REQT_WRITE,
			      value, index, tp->txbuf, size, 500);

	return ret;
}

static int r8152_generic_ocp_read(struct r8152 *tp, u16 index, u16 size,
				  void *data, u16 type)
{
	u16 burst_size = R8152_RX_BURST_SIZE;
	int txsize;
	int ret;

	/* both size and index must be 4 bytes align */
	if ((size & 3) || !size || (index & 3) || !data)
		return -EINVAL;

	if (index + size > 0xffff)
		return -EINVAL;

	while (size) {
		txsize = min(size, burst_size);
		ret = r8152_get_registers(tp, index, type, txsize, data);
		if (ret < 0)
			break;

		index += txsize;
		data += txsize;
		size -= txsize;
	}

	return ret;
}

int r8152_generic_ocp_write(struct r8152 *tp, u16 index, u16 byteen,
			    u16 size, const void *data, u16 type)
{
	u16 byteen_start, byteen_end, byte_en_to_hw;
	u16 burst_size = R8152_TX_BURST_SIZE;
	int txsize;
	int ret;

	/* both size and index must be 4 bytes align */
	if ((size & 3) || !size || (index & 3) || !data)
		return -EINVAL;

	if (index + size > 0xffff)
		return -EINVAL;

	byteen_start = byteen & BYTE_EN_START_MASK;
	byteen_end = byteen & BYTE_EN_END_MASK;

	byte_en_to_hw = byteen_start | (byteen_start << 4);
	ret = r8152_set_registers(tp, index, type | byte_en_to_hw, 4, data);
	if (ret < 0)
		return ret;

	index += 4;
	data += 4;
	size -= 4;

	if (size) {
		size -= 4;

		while (size) {
			txsize = min(size, burst_size);

			ret = r8152_set_registers(tp, index,
					    type | BYTE_EN_DWORD,
					    txsize, data);
			if (ret < 0)
				return ret;

			index += txsize;
			data += txsize;
			size -= txsize;
		}

		byte_en_to_hw = byteen_end | (byteen_end >> 4);
		ret = r8152_set_registers(tp, index, type | byte_en_to_hw, 4,
					  data);
		if (ret < 0)
			return ret;
	}

	return ret;
}

static int r8152_pla_ocp_read(struct r8152 *tp, u16 index, u16 size, void *data)
{
	return r8152_generic_ocp_read(tp, index, size, data, MCU_TYPE_PLA);
}

static int r8152_pla_ocp_write(struct r8152 *tp, u16 index, u16 byteen,
			       u16 size, const void *data)
{
	return r8152_generic_ocp_write(tp, index, byteen, size, data,
				       MCU_TYPE_PLA);
}

static int r8152_usb_ocp_write(struct r8152 *tp, u16 index, u16 byteen,
			       u16 size, const void *data)
{
	return r8152_generic_ocp_write(tp, index, byteen, size, data,
				       MCU_TYPE_USB);
}

static u32 r8152_ocp_read_dword(struct r8152 *tp, u16 type, u16 index)
{
	__le32 data;

	r8152_generic_ocp_read(tp, index, sizeof(data), &data, type);

	return __le32_to_cpu(data);
}

static void r8152_ocp_write_dword(struct r8152 *tp, u16 type, u16 index, u32 data)
{
	__le32 tmp = __cpu_to_le32(data);

	r8152_generic_ocp_write(tp, index, BYTE_EN_DWORD, sizeof(tmp), &tmp,
				type);
}

u16 r8152_ocp_read_word(struct r8152 *tp, u16 type, u16 index)
{
	u32 data;
	__le32 tmp;
	u8 shift = index & 2;

	index &= ~3;

	r8152_generic_ocp_read(tp, index, sizeof(tmp), &tmp, type);

	data = __le32_to_cpu(tmp);
	data >>= (shift * 8);
	data &= 0xffff;

	return data;
}

void r8152_ocp_write_word(struct r8152 *tp, u16 type, u16 index, u32 data)
{
	u32 mask = 0xffff;
	__le32 tmp;
	u16 byen = BYTE_EN_WORD;
	u8 shift = index & 2;

	data &= mask;

	if (index & 2) {
		byen <<= shift;
		mask <<= (shift * 8);
		data <<= (shift * 8);
		index &= ~3;
	}

	tmp = __cpu_to_le32(data);

	r8152_generic_ocp_write(tp, index, byen, sizeof(tmp), &tmp, type);
}

u8 r8152_ocp_read_byte(struct r8152 *tp, u16 type, u16 index)
{
	u32 data;
	__le32 tmp;
	u8 shift = index & 3;

	index &= ~3;

	r8152_generic_ocp_read(tp, index, sizeof(tmp), &tmp, type);

	data = __le32_to_cpu(tmp);
	data >>= (shift * 8);
	data &= 0xff;

	return data;
}

void r8152_ocp_write_byte(struct r8152 *tp, u16 type, u16 index, u32 data)
{
	u32 mask = 0xff;
	__le32 tmp;
	u16 byen = BYTE_EN_BYTE;
	u8 shift = index & 3;

	data &= mask;

	if (index & 3) {
		byen <<= shift;
		mask <<= (shift * 8);
		data <<= (shift * 8);
		index &= ~3;
	}

	tmp = __cpu_to_le32(data);

	r8152_generic_ocp_write(tp, index, byen, sizeof(tmp), &tmp, type);
}

u16 r8152_ocp_reg_read(struct r8152 *tp, u16 addr)
{
	u16 ocp_base, ocp_index;

	ocp_base = addr & 0xf000;
	if (ocp_base != tp->ocp_base) {
		r8152_ocp_write_word(tp, MCU_TYPE_PLA, PLA_OCP_GPHY_BASE,
				     ocp_base);
		tp->ocp_base = ocp_base;
	}

	ocp_index = (addr & 0x0fff) | 0xb000;
	return r8152_ocp_read_word(tp, MCU_TYPE_PLA, ocp_index);
}

void r8152_ocp_reg_write(struct r8152 *tp, u16 addr, u16 data)
{
	u16 ocp_base, ocp_index;

	ocp_base = addr & 0xf000;
	if (ocp_base != tp->ocp_base) {
		r8152_ocp_write_word(tp, MCU_TYPE_PLA, PLA_OCP_GPHY_BASE,
				     ocp_base);
		tp->ocp_base = ocp_base;
	}

	ocp_index = (addr & 0x0fff) | 0xb000;
	r8152_ocp_write_word(tp, MCU_TYPE_PLA, ocp_index, data);
}

static void r8152_mdio_write(struct r8152 *tp, u32 reg_addr, u32 value)
{
	r8152_ocp_reg_write(tp, OCP_BASE_MII + reg_addr * 2, value);
}

static int r8152_mdio_read(struct r8152 *tp, u32 reg_addr)
{
	return r8152_ocp_reg_read(tp, OCP_BASE_MII + reg_addr * 2);
}

void r8152_sram_write(struct r8152 *tp, u16 addr, u16 data)
{
	r8152_ocp_reg_write(tp, OCP_SRAM_ADDR, addr);
	r8152_ocp_reg_write(tp, OCP_SRAM_DATA, data);
}

static u16 r8152_sram_read(struct r8152 *tp, u16 addr)
{
	r8152_ocp_reg_write(tp, OCP_SRAM_ADDR, addr);
	return r8152_ocp_reg_read(tp, OCP_SRAM_DATA);
}

static int r8152_wait_for_bit(struct r8152 *tp, bool ocp_reg, u16 type,
			      u16 index, const u32 mask, bool set,
			      unsigned int timeout)
{
	u32 val;
	u64 start;

	start = get_time_ns();
	do {
		if (ocp_reg)
			val = r8152_ocp_reg_read(tp, index);
		else
			val = r8152_ocp_read_dword(tp, type, index);

		if (!set)
			val = ~val;

		if ((val & mask) == mask)
			return 0;

		mdelay(2);
	} while (!is_timeout(start, timeout * MSECOND));

	dev_dbg(&tp->dev->edev.dev, "%s: Timeout (index=%04x mask=%08x timeout=%d)\n",
		__func__, index, mask, timeout);

	return -ETIMEDOUT;
}

static void r8152b_reset_packet_filter(struct r8152 *tp)
{
	u32 ocp_data;

	ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_PLA, PLA_FMC);
	ocp_data &= ~FMC_FCR_MCU_EN;
	r8152_ocp_write_word(tp, MCU_TYPE_PLA, PLA_FMC, ocp_data);
	ocp_data |= FMC_FCR_MCU_EN;
	r8152_ocp_write_word(tp, MCU_TYPE_PLA, PLA_FMC, ocp_data);
}

static void r8152_wait_fifo_empty(struct r8152 *tp)
{
	int ret;

	ret = r8152_wait_for_bit(tp, 0, MCU_TYPE_PLA, PLA_PHY_PWR,
				 PLA_PHY_PWR_TXEMP, 1, R8152_WAIT_TIMEOUT);
	if (ret)
		dev_dbg(&tp->dev->edev.dev, "Timeout waiting for FIFO empty\n");

	ret = r8152_wait_for_bit(tp, 0, MCU_TYPE_PLA, PLA_TCR0,
				 TCR0_TX_EMPTY, 1, R8152_WAIT_TIMEOUT);
	if (ret)
		dev_dbg(&tp->dev->edev.dev, "Timeout waiting for TX empty\n");
}

static void r8152_nic_reset(struct r8152 *tp)
{
	int ret;
	u32 ocp_data;

	ocp_data = r8152_ocp_read_dword(tp, MCU_TYPE_PLA, BIST_CTRL);
	ocp_data |= BIST_CTRL_SW_RESET;
	r8152_ocp_write_dword(tp, MCU_TYPE_PLA, BIST_CTRL, ocp_data);

	ret = r8152_wait_for_bit(tp, 0, MCU_TYPE_PLA, BIST_CTRL,
				 BIST_CTRL_SW_RESET, 0, R8152_WAIT_TIMEOUT);
	if (ret)
		dev_dbg(&tp->dev->edev.dev, "Timeout waiting for NIC reset\n");
}

static u8 r8152_get_speed(struct r8152 *tp)
{
	return r8152_ocp_read_byte(tp, MCU_TYPE_PLA, PLA_PHYSTATUS);
}

static void r8152_set_eee_plus(struct r8152 *tp)
{
	u32 ocp_data;

	ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_PLA, PLA_EEEP_CR);
	ocp_data &= ~EEEP_CR_EEEP_TX;
	r8152_ocp_write_word(tp, MCU_TYPE_PLA, PLA_EEEP_CR, ocp_data);
}

static void rxdy_gated_en(struct r8152 *tp, bool enable)
{
	u32 ocp_data;

	ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_PLA, PLA_MISC_1);
	if (enable)
		ocp_data |= RXDY_GATED_EN;
	else
		ocp_data &= ~RXDY_GATED_EN;
	r8152_ocp_write_word(tp, MCU_TYPE_PLA, PLA_MISC_1, ocp_data);
}

static void rtl8152_set_rx_mode(struct r8152 *tp)
{
	u32 ocp_data;
	__le32 tmp[2];

	tmp[0] = 0xffffffff;
	tmp[1] = 0xffffffff;

	r8152_pla_ocp_write(tp, PLA_MAR, BYTE_EN_DWORD, sizeof(tmp), tmp);

	ocp_data = r8152_ocp_read_dword(tp, MCU_TYPE_PLA, PLA_RCR);
	ocp_data |= RCR_APM | RCR_AM | RCR_AB;
	r8152_ocp_write_dword(tp, MCU_TYPE_PLA, PLA_RCR, ocp_data);
}

static inline void r8153b_rx_agg_chg_indicate(struct r8152 *tp)
{
	r8152_ocp_write_byte(tp, MCU_TYPE_USB, USB_UPT_RXDMA_OWN,
		       OWN_UPDATE | OWN_CLEAR);
}

static int rtl_enable(struct r8152 *tp)
{
	u32 ocp_data;

	r8152b_reset_packet_filter(tp);

	ocp_data = r8152_ocp_read_byte(tp, MCU_TYPE_PLA, PLA_CR);
	ocp_data |= PLA_CR_RE | PLA_CR_TE;
	r8152_ocp_write_byte(tp, MCU_TYPE_PLA, PLA_CR, ocp_data);

	switch (tp->version) {
	case RTL_VER_08:
	case RTL_VER_09:
		r8153b_rx_agg_chg_indicate(tp);
		break;
	default:
		break;
	}

	rxdy_gated_en(tp, false);

	rtl8152_set_rx_mode(tp);

	return 0;
}

static int rtl8152_enable(struct r8152 *tp)
{
	r8152_set_eee_plus(tp);

	return rtl_enable(tp);
}

static void r8153_set_rx_early_timeout(struct r8152 *tp)
{
	u32 ocp_data = tp->coalesce / 8;

	switch (tp->version) {
	case RTL_VER_03:
	case RTL_VER_04:
	case RTL_VER_05:
	case RTL_VER_06:
		r8152_ocp_write_word(tp, MCU_TYPE_USB, USB_RX_EARLY_TIMEOUT,
				     ocp_data);
		break;

	case RTL_VER_08:
	case RTL_VER_09:
		/* The RTL8153B uses USB_RX_EXTRA_AGGR_TMR for rx timeout
		 * primarily. For USB_RX_EARLY_TIMEOUT, we fix it to 1264ns.
		 */
		r8152_ocp_write_word(tp, MCU_TYPE_USB, USB_RX_EARLY_TIMEOUT,
				     RX_AUXILIARY_TIMER / 8);
		r8152_ocp_write_word(tp, MCU_TYPE_USB, USB_RX_EXTRA_AGGR_TMR,
				     ocp_data);
		break;

	default:
		dev_dbg(&tp->dev->edev.dev, "** %s Invalid Device\n", __func__);
		break;
	}
}

static void r8153_set_rx_early_size(struct r8152 *tp)
{
	u32 ocp_data = (RTL8152_AGG_BUF_SZ - RTL8153_RMS -
			sizeof(struct rx_desc));

	switch (tp->version) {
	case RTL_VER_03:
	case RTL_VER_04:
	case RTL_VER_05:
	case RTL_VER_06:
		r8152_ocp_write_word(tp, MCU_TYPE_USB, USB_RX_EARLY_SIZE,
				     ocp_data / 4);
		break;

	case RTL_VER_08:
	case RTL_VER_09:
		r8152_ocp_write_word(tp, MCU_TYPE_USB, USB_RX_EARLY_SIZE,
				     ocp_data / 8);
		break;

	default:
		dev_dbg(&tp->dev->edev.dev, "** %s Invalid Device\n", __func__);
		break;
	}
}

static int rtl8153_enable(struct r8152 *tp)
{
	r8152_set_eee_plus(tp);
	r8153_set_rx_early_timeout(tp);
	r8153_set_rx_early_size(tp);

	return rtl_enable(tp);
}

static void rtl_disable(struct r8152 *tp)
{
	u32 ocp_data;

	ocp_data = r8152_ocp_read_dword(tp, MCU_TYPE_PLA, PLA_RCR);
	ocp_data &= ~RCR_ACPT_ALL;
	r8152_ocp_write_dword(tp, MCU_TYPE_PLA, PLA_RCR, ocp_data);

	rxdy_gated_en(tp, true);

	r8152_wait_fifo_empty(tp);
	r8152_nic_reset(tp);
}

static void r8152_power_cut_en(struct r8152 *tp, bool enable)
{
	u32 ocp_data;

	ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_USB, USB_UPS_CTRL);
	if (enable)
		ocp_data |= POWER_CUT;
	else
		ocp_data &= ~POWER_CUT;
	r8152_ocp_write_word(tp, MCU_TYPE_USB, USB_UPS_CTRL, ocp_data);

	ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_USB, USB_PM_CTRL_STATUS);
	ocp_data &= ~RESUME_INDICATE;
	r8152_ocp_write_word(tp, MCU_TYPE_USB, USB_PM_CTRL_STATUS, ocp_data);
}

static void rtl_rx_vlan_en(struct r8152 *tp, bool enable)
{
	u32 ocp_data;

	ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_PLA, PLA_CPCR);
	if (enable)
		ocp_data |= CPCR_RX_VLAN;
	else
		ocp_data &= ~CPCR_RX_VLAN;
	r8152_ocp_write_word(tp, MCU_TYPE_PLA, PLA_CPCR, ocp_data);
}

static void r8153_u1u2en(struct r8152 *tp, bool enable)
{
	u8 u1u2[8];

	if (enable)
		memset(u1u2, 0xff, sizeof(u1u2));
	else
		memset(u1u2, 0x00, sizeof(u1u2));

	r8152_usb_ocp_write(tp, USB_TOLERANCE, BYTE_EN_SIX_BYTES, sizeof(u1u2),
			    u1u2);
}

static void r8153b_u1u2en(struct r8152 *tp, bool enable)
{
	u16 ocp_data;

	ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_USB, USB_LPM_CONFIG);
	if (enable)
		ocp_data |= LPM_U1U2_EN;
	else
		ocp_data &= ~LPM_U1U2_EN;

	r8152_ocp_write_word(tp, MCU_TYPE_USB, USB_LPM_CONFIG, ocp_data);
}

static void r8153_u2p3en(struct r8152 *tp, bool enable)
{
	u32 ocp_data;

	ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_USB, USB_U2P3_CTRL);
	if (enable && tp->version != RTL_VER_03 && tp->version != RTL_VER_04)
		ocp_data |= U2P3_ENABLE;
	else
		ocp_data &= ~U2P3_ENABLE;
	r8152_ocp_write_word(tp, MCU_TYPE_USB, USB_U2P3_CTRL, ocp_data);
}

static void r8153_power_cut_en(struct r8152 *tp, bool enable)
{
	u32 ocp_data;

	ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_USB, USB_POWER_CUT);
	if (enable)
		ocp_data |= PWR_EN | PHASE2_EN;
	else
		ocp_data &= ~(PWR_EN | PHASE2_EN);
	r8152_ocp_write_word(tp, MCU_TYPE_USB, USB_POWER_CUT, ocp_data);

	ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_USB, USB_MISC_0);
	ocp_data &= ~PCUT_STATUS;
	r8152_ocp_write_word(tp, MCU_TYPE_USB, USB_MISC_0, ocp_data);
}

static void rtl_reset_bmu(struct r8152 *tp)
{
	u8 ocp_data;

	ocp_data = r8152_ocp_read_byte(tp, MCU_TYPE_USB, USB_BMU_RESET);
	ocp_data &= ~(BMU_RESET_EP_IN | BMU_RESET_EP_OUT);
	r8152_ocp_write_byte(tp, MCU_TYPE_USB, USB_BMU_RESET, ocp_data);
	ocp_data |= BMU_RESET_EP_IN | BMU_RESET_EP_OUT;
	r8152_ocp_write_byte(tp, MCU_TYPE_USB, USB_BMU_RESET, ocp_data);
}

static int r8152_read_mac(struct r8152 *tp, unsigned char *macaddr)
{
	int ret;
	unsigned char enetaddr[8] = {0};

	ret = r8152_pla_ocp_read(tp, PLA_IDR, 8, enetaddr);
	if (ret < 0)
		return ret;

	memcpy(macaddr, enetaddr, ETH_ALEN);
	return 0;
}

static void r8152b_disable_aldps(struct r8152 *tp)
{
	r8152_ocp_reg_write(tp, OCP_ALDPS_CONFIG,
			    ENPDNPS | LINKENA | DIS_SDSAVE);
	mdelay(20);
}

static void r8152b_enable_aldps(struct r8152 *tp)
{
	r8152_ocp_reg_write(tp, OCP_ALDPS_CONFIG, ENPWRSAVE | ENPDNPS |
			    LINKENA | DIS_SDSAVE);
}

static void rtl8152_disable(struct r8152 *tp)
{
	r8152b_disable_aldps(tp);
	rtl_disable(tp);
	r8152b_enable_aldps(tp);
}

static void r8152b_hw_phy_cfg(struct r8152 *tp)
{
	u16 data;

	data = r8152_mdio_read(tp, MII_BMCR);
	if (data & BMCR_PDOWN) {
		data &= ~BMCR_PDOWN;
		r8152_mdio_write(tp, MII_BMCR, data);
	}

	r8152b_firmware(tp);
}

static void rtl8152_reinit_ll(struct r8152 *tp)
{
	u32 ocp_data;
	int ret;

	ret = r8152_wait_for_bit(tp, 0, MCU_TYPE_PLA, PLA_PHY_PWR,
				 PLA_PHY_PWR_LLR, 1, R8152_WAIT_TIMEOUT);
	if (ret)
		dev_dbg(&tp->dev->edev.dev, "Timeout waiting for link list ready\n");

	ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_PLA, PLA_SFF_STS_7);
	ocp_data |= RE_INIT_LL;
	r8152_ocp_write_word(tp, MCU_TYPE_PLA, PLA_SFF_STS_7, ocp_data);

	ret = r8152_wait_for_bit(tp, 0, MCU_TYPE_PLA, PLA_PHY_PWR,
				 PLA_PHY_PWR_LLR, 1, R8152_WAIT_TIMEOUT);
	if (ret)
		dev_dbg(&tp->dev->edev.dev, "Timeout waiting for link list ready\n");
}

static void r8152b_exit_oob(struct r8152 *tp)
{
	u32 ocp_data;

	ocp_data = r8152_ocp_read_dword(tp, MCU_TYPE_PLA, PLA_RCR);
	ocp_data &= ~RCR_ACPT_ALL;
	r8152_ocp_write_dword(tp, MCU_TYPE_PLA, PLA_RCR, ocp_data);

	rxdy_gated_en(tp, true);
	r8152b_hw_phy_cfg(tp);

	r8152_ocp_write_byte(tp, MCU_TYPE_PLA, PLA_CRWECR, CRWECR_NORAML);
	r8152_ocp_write_byte(tp, MCU_TYPE_PLA, PLA_CR, 0x00);

	ocp_data = r8152_ocp_read_byte(tp, MCU_TYPE_PLA, PLA_OOB_CTRL);
	ocp_data &= ~NOW_IS_OOB;
	r8152_ocp_write_byte(tp, MCU_TYPE_PLA, PLA_OOB_CTRL, ocp_data);

	ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_PLA, PLA_SFF_STS_7);
	ocp_data &= ~MCU_BORW_EN;
	r8152_ocp_write_word(tp, MCU_TYPE_PLA, PLA_SFF_STS_7, ocp_data);

	rtl8152_reinit_ll(tp);
	r8152_nic_reset(tp);

	/* rx share fifo credit full threshold */
	r8152_ocp_write_dword(tp, MCU_TYPE_PLA, PLA_RXFIFO_CTRL0,
			      RXFIFO_THR1_NORMAL);

	if (tp->udev->speed == USB_SPEED_FULL ||
	    tp->udev->speed == USB_SPEED_LOW) {
		/* rx share fifo credit near full threshold */
		r8152_ocp_write_dword(tp, MCU_TYPE_PLA, PLA_RXFIFO_CTRL1,
				      RXFIFO_THR2_FULL);
		r8152_ocp_write_dword(tp, MCU_TYPE_PLA, PLA_RXFIFO_CTRL2,
				      RXFIFO_THR3_FULL);
	} else {
		/* rx share fifo credit near full threshold */
		r8152_ocp_write_dword(tp, MCU_TYPE_PLA, PLA_RXFIFO_CTRL1,
				      RXFIFO_THR2_HIGH);
		r8152_ocp_write_dword(tp, MCU_TYPE_PLA, PLA_RXFIFO_CTRL2,
				      RXFIFO_THR3_HIGH);
	}

	/* TX share fifo free credit full threshold */
	r8152_ocp_write_dword(tp, MCU_TYPE_PLA, PLA_TXFIFO_CTRL,
			      TXFIFO_THR_NORMAL);

	r8152_ocp_write_byte(tp, MCU_TYPE_USB, USB_TX_AGG, TX_AGG_MAX_THRESHOLD);
	r8152_ocp_write_dword(tp, MCU_TYPE_USB, USB_RX_BUF_TH, RX_THR_HIGH);
	r8152_ocp_write_dword(tp, MCU_TYPE_USB, USB_TX_DMA,
			      TEST_MODE_DISABLE | TX_SIZE_ADJUST1);

	r8152_ocp_write_word(tp, MCU_TYPE_PLA, PLA_RMS, RTL8152_RMS);

	ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_PLA, PLA_TCR0);
	ocp_data |= TCR0_AUTO_FIFO;
	r8152_ocp_write_word(tp, MCU_TYPE_PLA, PLA_TCR0, ocp_data);
}

static void r8153_hw_phy_cfg(struct r8152 *tp)
{
	u32 ocp_data;
	u16 data;

	if (tp->version == RTL_VER_03 || tp->version == RTL_VER_04 ||
	    tp->version == RTL_VER_05)
		r8152_ocp_reg_write(tp, OCP_ADC_CFG,
				    CKADSEL_L | ADC_EN | EN_EMI_L);

	data = r8152_mdio_read(tp, MII_BMCR);
	if (data & BMCR_PDOWN) {
		data &= ~BMCR_PDOWN;
		r8152_mdio_write(tp, MII_BMCR, data);
	}

	r8153_firmware(tp);

	if (tp->version == RTL_VER_03) {
		data = r8152_ocp_reg_read(tp, OCP_EEE_CFG);
		data &= ~CTAP_SHORT_EN;
		r8152_ocp_reg_write(tp, OCP_EEE_CFG, data);
	}

	data = r8152_ocp_reg_read(tp, OCP_POWER_CFG);
	data |= EEE_CLKDIV_EN;
	r8152_ocp_reg_write(tp, OCP_POWER_CFG, data);

	data = r8152_ocp_reg_read(tp, OCP_DOWN_SPEED);
	data |= EN_10M_BGOFF;
	r8152_ocp_reg_write(tp, OCP_DOWN_SPEED, data);
	data = r8152_ocp_reg_read(tp, OCP_POWER_CFG);
	data |= EN_10M_PLLOFF;
	r8152_ocp_reg_write(tp, OCP_POWER_CFG, data);
	r8152_sram_write(tp, SRAM_IMPEDANCE, 0x0b13);

	ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_PLA, PLA_PHY_PWR);
	ocp_data |= PFM_PWM_SWITCH;
	r8152_ocp_write_word(tp, MCU_TYPE_PLA, PLA_PHY_PWR, ocp_data);

	/* Enable LPF corner auto tune */
	r8152_sram_write(tp, SRAM_LPF_CFG, 0xf70f);

	/* Adjust 10M Amplitude */
	r8152_sram_write(tp, SRAM_10M_AMP1, 0x00af);
	r8152_sram_write(tp, SRAM_10M_AMP2, 0x0208);
}

static u32 r8152_efuse_read(struct r8152 *tp, u8 addr)
{
	u32 ocp_data;

	r8152_ocp_write_word(tp, MCU_TYPE_PLA, PLA_EFUSE_CMD,
			     EFUSE_READ_CMD | addr);
	ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_PLA, PLA_EFUSE_CMD);
	ocp_data = (ocp_data & EFUSE_DATA_BIT16) << 9;	/* data of bit16 */
	ocp_data |= r8152_ocp_read_word(tp, MCU_TYPE_PLA, PLA_EFUSE_DATA);

	return ocp_data;
}

static void r8153b_hw_phy_cfg(struct r8152 *tp)
{
	u32 ocp_data;
	u16 data;

	data = r8152_mdio_read(tp, MII_BMCR);
	if (data & BMCR_PDOWN) {
		data &= ~BMCR_PDOWN;
		r8152_mdio_write(tp, MII_BMCR, data);
	}

	/* U1/U2/L1 idle timer. 500 us */
	r8152_ocp_write_word(tp, MCU_TYPE_USB, USB_U1U2_TIMER, 500);

	r8153b_firmware(tp);

	data = r8152_sram_read(tp, SRAM_GREEN_CFG);
	data |= R_TUNE_EN;
	r8152_sram_write(tp, SRAM_GREEN_CFG, data);
	data = r8152_ocp_reg_read(tp, OCP_NCTL_CFG);
	data |= PGA_RETURN_EN;
	r8152_ocp_reg_write(tp, OCP_NCTL_CFG, data);

	/* ADC Bias Calibration:
	 * read efuse offset 0x7d to get a 17-bit data. Remove the dummy/fake
	 * bit (bit3) to rebuild the real 16-bit data. Write the data to the
	 * ADC ioffset.
	 */
	ocp_data = r8152_efuse_read(tp, 0x7d);
	ocp_data = ((ocp_data & 0x1fff0) >> 1) | (ocp_data & 0x7);
	if (ocp_data != 0xffff)
		r8152_ocp_reg_write(tp, OCP_ADC_IOFFSET, ocp_data);

	/* ups mode tx-link-pulse timing adjustment:
	 * rg_saw_cnt = OCP reg 0xC426 Bit[13:0]
	 * swr_cnt_1ms_ini = 16000000 / rg_saw_cnt
	 */
	ocp_data = r8152_ocp_reg_read(tp, 0xc426);
	ocp_data &= 0x3fff;
	if (ocp_data) {
		u32 swr_cnt_1ms_ini;

		swr_cnt_1ms_ini = (16000000 / ocp_data) & SAW_CNT_1MS_MASK;
		ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_USB, USB_UPS_CFG);
		ocp_data = (ocp_data & ~SAW_CNT_1MS_MASK) | swr_cnt_1ms_ini;
		r8152_ocp_write_word(tp, MCU_TYPE_USB, USB_UPS_CFG, ocp_data);
	}

	ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_PLA, PLA_PHY_PWR);
	ocp_data |= PFM_PWM_SWITCH;
	r8152_ocp_write_word(tp, MCU_TYPE_PLA, PLA_PHY_PWR, ocp_data);
}

static void r8153_first_init(struct r8152 *tp)
{
	u32 ocp_data;

	rxdy_gated_en(tp, true);

	ocp_data = r8152_ocp_read_dword(tp, MCU_TYPE_PLA, PLA_RCR);
	ocp_data &= ~RCR_ACPT_ALL;
	r8152_ocp_write_dword(tp, MCU_TYPE_PLA, PLA_RCR, ocp_data);

	r8153_hw_phy_cfg(tp);

	r8152_nic_reset(tp);
	rtl_reset_bmu(tp);

	ocp_data = r8152_ocp_read_byte(tp, MCU_TYPE_PLA, PLA_OOB_CTRL);
	ocp_data &= ~NOW_IS_OOB;
	r8152_ocp_write_byte(tp, MCU_TYPE_PLA, PLA_OOB_CTRL, ocp_data);

	ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_PLA, PLA_SFF_STS_7);
	ocp_data &= ~MCU_BORW_EN;
	r8152_ocp_write_word(tp, MCU_TYPE_PLA, PLA_SFF_STS_7, ocp_data);

	rtl8152_reinit_ll(tp);

	rtl_rx_vlan_en(tp, false);

	ocp_data = RTL8153_RMS;
	r8152_ocp_write_word(tp, MCU_TYPE_PLA, PLA_RMS, ocp_data);
	r8152_ocp_write_byte(tp, MCU_TYPE_PLA, PLA_MTPS, MTPS_JUMBO);

	ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_PLA, PLA_TCR0);
	ocp_data |= TCR0_AUTO_FIFO;
	r8152_ocp_write_word(tp, MCU_TYPE_PLA, PLA_TCR0, ocp_data);

	r8152_nic_reset(tp);

	/* rx share fifo credit full threshold */
	r8152_ocp_write_dword(tp, MCU_TYPE_PLA, PLA_RXFIFO_CTRL0,
			      RXFIFO_THR1_NORMAL);
	r8152_ocp_write_word(tp, MCU_TYPE_PLA, PLA_RXFIFO_CTRL1,
			     RXFIFO_THR2_NORMAL);
	r8152_ocp_write_word(tp, MCU_TYPE_PLA, PLA_RXFIFO_CTRL2,
			     RXFIFO_THR3_NORMAL);
	/* TX share fifo free credit full threshold */
	r8152_ocp_write_dword(tp, MCU_TYPE_PLA, PLA_TXFIFO_CTRL,
			      TXFIFO_THR_NORMAL2);

	/* Disable rx aggregation */
	ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_USB, USB_USB_CTRL);
	ocp_data |= (RX_AGG_DISABLE | RX_ZERO_EN);
	r8152_ocp_write_word(tp, MCU_TYPE_USB, USB_USB_CTRL, ocp_data);
}

static void r8153_disable_aldps(struct r8152 *tp)
{
	u16 data;

	data = r8152_ocp_reg_read(tp, OCP_POWER_CFG);
	data &= ~EN_ALDPS;
	r8152_ocp_reg_write(tp, OCP_POWER_CFG, data);
	mdelay(20);
}

static void rtl8153_disable(struct r8152 *tp)
{
	r8153_disable_aldps(tp);
	rtl_disable(tp);
	rtl_reset_bmu(tp);
}

static int rtl8152_set_speed(struct r8152 *tp, u8 autoneg, u16 speed, u8 duplex)
{
	u16 bmcr, anar, gbcr;

	anar = r8152_mdio_read(tp, MII_ADVERTISE);
	anar &= ~(ADVERTISE_10HALF | ADVERTISE_10FULL |
		  ADVERTISE_100HALF | ADVERTISE_100FULL);
	if (tp->supports_gmii) {
		gbcr = r8152_mdio_read(tp, MII_CTRL1000);
		gbcr &= ~(ADVERTISE_1000FULL | ADVERTISE_1000HALF);
	} else {
		gbcr = 0;
	}

	if (autoneg == AUTONEG_DISABLE) {
		if (speed == SPEED_10) {
			bmcr = 0;
			anar |= ADVERTISE_10HALF | ADVERTISE_10FULL;
		} else if (speed == SPEED_100) {
			bmcr = BMCR_SPEED100;
			anar |= ADVERTISE_100HALF | ADVERTISE_100FULL;
		} else if (speed == SPEED_1000 && tp->supports_gmii) {
			bmcr = BMCR_SPEED1000;
			gbcr |= ADVERTISE_1000FULL | ADVERTISE_1000HALF;
		} else {
			return -EINVAL;
		}

		if (duplex == DUPLEX_FULL)
			bmcr |= BMCR_FULLDPLX;
	} else {
		if (speed == SPEED_10) {
			if (duplex == DUPLEX_FULL)
				anar |= ADVERTISE_10HALF | ADVERTISE_10FULL;
			else
				anar |= ADVERTISE_10HALF;
		} else if (speed == SPEED_100) {
			if (duplex == DUPLEX_FULL) {
				anar |= ADVERTISE_10HALF | ADVERTISE_10FULL;
				anar |= ADVERTISE_100HALF | ADVERTISE_100FULL;
			} else {
				anar |= ADVERTISE_10HALF;
				anar |= ADVERTISE_100HALF;
			}
		} else if (speed == SPEED_1000 && tp->supports_gmii) {
			if (duplex == DUPLEX_FULL) {
				anar |= ADVERTISE_10HALF | ADVERTISE_10FULL;
				anar |= ADVERTISE_100HALF | ADVERTISE_100FULL;
				gbcr |= ADVERTISE_1000FULL | ADVERTISE_1000HALF;
			} else {
				anar |= ADVERTISE_10HALF;
				anar |= ADVERTISE_100HALF;
				gbcr |= ADVERTISE_1000HALF;
			}
		} else {
			return -EINVAL;
		}

		bmcr = BMCR_ANENABLE | BMCR_ANRESTART | BMCR_RESET;
	}

	if (tp->supports_gmii)
		r8152_mdio_write(tp, MII_CTRL1000, gbcr);

	r8152_mdio_write(tp, MII_ADVERTISE, anar);
	r8152_mdio_write(tp, MII_BMCR, bmcr);

	return 0;
}

static void rtl8152_up(struct r8152 *tp)
{
	r8152b_disable_aldps(tp);
	r8152b_exit_oob(tp);
	r8152b_enable_aldps(tp);
}

static void rtl8153_up(struct r8152 *tp)
{
	r8153_u1u2en(tp, false);
	r8153_disable_aldps(tp);
	r8153_first_init(tp);
	r8153_u2p3en(tp, false);
}

static void rtl8153b_up(struct r8152 *tp)
{
	r8153_first_init(tp);
}

static void r8152_get_version(struct r8152 *tp)
{
	u32 ocp_data;
	u16 tcr;
	int i;

	ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_PLA, PLA_TCR1);
	tcr = (u16)(ocp_data & VERSION_MASK);

	for (i = 0; i < ARRAY_SIZE(r8152_versions); i++) {
		if (tcr == r8152_versions[i].tcr) {
			/* Found a supported version */
			tp->version = r8152_versions[i].version;
			tp->supports_gmii = r8152_versions[i].gmii;
			break;
		}
	}

	if (tp->version == RTL_VER_UNKNOWN)
		dev_dbg(&tp->dev->edev.dev,
			"r8152 Unknown tcr version 0x%04x\n", tcr);
}

static void r8152b_enable_fc(struct r8152 *tp)
{
	u16 anar;

	anar = r8152_mdio_read(tp, MII_ADVERTISE);
	anar |= ADVERTISE_PAUSE_CAP | ADVERTISE_PAUSE_ASYM;
	r8152_mdio_write(tp, MII_ADVERTISE, anar);
}

static void rtl_tally_reset(struct r8152 *tp)
{
	u32 ocp_data;

	ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_PLA, PLA_RSTTALLY);
	ocp_data |= TALLY_RESET;
	r8152_ocp_write_word(tp, MCU_TYPE_PLA, PLA_RSTTALLY, ocp_data);
}

static void rtl8152b_init(struct r8152 *tp)
{
	u32 ocp_data;

	r8152b_disable_aldps(tp);

	if (tp->version == RTL_VER_01) {
		ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_PLA,
					       PLA_LED_FEATURE);
		ocp_data &= ~LED_MODE_MASK;
		r8152_ocp_write_word(tp, MCU_TYPE_PLA, PLA_LED_FEATURE, ocp_data);
	}

	r8152_power_cut_en(tp, false);

	ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_PLA, PLA_PHY_PWR);
	ocp_data |= TX_10M_IDLE_EN | PFM_PWM_SWITCH;
	r8152_ocp_write_word(tp, MCU_TYPE_PLA, PLA_PHY_PWR, ocp_data);
	ocp_data = r8152_ocp_read_dword(tp, MCU_TYPE_PLA, PLA_MAC_PWR_CTRL);
	ocp_data &= ~MCU_CLK_RATIO_MASK;
	ocp_data |= MCU_CLK_RATIO | D3_CLK_GATED_EN;
	r8152_ocp_write_dword(tp, MCU_TYPE_PLA, PLA_MAC_PWR_CTRL, ocp_data);
	ocp_data = GPHY_STS_MSK | SPEED_DOWN_MSK |
		   SPDWN_RXDV_MSK | SPDWN_LINKCHG_MSK;
	r8152_ocp_write_word(tp, MCU_TYPE_PLA, PLA_GPHY_INTR_IMR, ocp_data);

	ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_USB, USB_USB_TIMER);
	ocp_data |= BIT(15);
	r8152_ocp_write_word(tp, MCU_TYPE_USB, USB_USB_TIMER, ocp_data);
	r8152_ocp_write_word(tp, MCU_TYPE_USB, 0xcbfc, 0x03e8);
	ocp_data &= ~BIT(15);
	r8152_ocp_write_word(tp, MCU_TYPE_USB, USB_USB_TIMER, ocp_data);

	r8152b_enable_fc(tp);
	rtl_tally_reset(tp);

	/* Disable rx aggregation */
	ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_USB, USB_USB_CTRL);
	ocp_data |= (RX_AGG_DISABLE | RX_ZERO_EN);
	r8152_ocp_write_word(tp, MCU_TYPE_USB, USB_USB_CTRL, ocp_data);
}

static void rtl8153_init(struct r8152 *tp)
{
	int i;
	u32 ocp_data;

	r8153_disable_aldps(tp);
	r8153_u1u2en(tp, false);

	r8152_wait_for_bit(tp, 0, MCU_TYPE_PLA, PLA_BOOT_CTRL,
			   AUTOLOAD_DONE, 1, R8152_WAIT_TIMEOUT);

	for (i = 0; i < R8152_WAIT_TIMEOUT; i++) {
		ocp_data = r8152_ocp_reg_read(tp, OCP_PHY_STATUS) &
			PHY_STAT_MASK;
		if (ocp_data == PHY_STAT_LAN_ON || ocp_data == PHY_STAT_PWRDN)
			break;

		mdelay(1);
	}

	r8153_u2p3en(tp, false);

	if (tp->version == RTL_VER_04) {
		ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_USB,
					       USB_SSPHYLINK2);
		ocp_data &= ~pwd_dn_scale_mask;
		ocp_data |= pwd_dn_scale(96);
		r8152_ocp_write_word(tp, MCU_TYPE_USB, USB_SSPHYLINK2,
				     ocp_data);

		ocp_data = r8152_ocp_read_byte(tp, MCU_TYPE_USB, USB_USB2PHY);
		ocp_data |= USB2PHY_L1 | USB2PHY_SUSPEND;
		r8152_ocp_write_byte(tp, MCU_TYPE_USB, USB_USB2PHY, ocp_data);
	} else if (tp->version == RTL_VER_05) {
		ocp_data = r8152_ocp_read_byte(tp, MCU_TYPE_PLA, PLA_DMY_REG0);
		ocp_data &= ~ECM_ALDPS;
		r8152_ocp_write_byte(tp, MCU_TYPE_PLA, PLA_DMY_REG0, ocp_data);

		ocp_data = r8152_ocp_read_byte(tp, MCU_TYPE_USB,
					       USB_CSR_DUMMY1);
		if (r8152_ocp_read_word(tp, MCU_TYPE_USB, USB_BURST_SIZE) == 0)
			ocp_data &= ~DYNAMIC_BURST;
		else
			ocp_data |= DYNAMIC_BURST;
		r8152_ocp_write_byte(tp, MCU_TYPE_USB, USB_CSR_DUMMY1,
				     ocp_data);
	} else if (tp->version == RTL_VER_06) {
		ocp_data = r8152_ocp_read_byte(tp, MCU_TYPE_USB,
					       USB_CSR_DUMMY1);
		if (r8152_ocp_read_word(tp, MCU_TYPE_USB, USB_BURST_SIZE) == 0)
			ocp_data &= ~DYNAMIC_BURST;
		else
			ocp_data |= DYNAMIC_BURST;
		r8152_ocp_write_byte(tp, MCU_TYPE_USB, USB_CSR_DUMMY1,
				     ocp_data);
	}

	ocp_data = r8152_ocp_read_byte(tp, MCU_TYPE_USB, USB_CSR_DUMMY2);
	ocp_data |= EP4_FULL_FC;
	r8152_ocp_write_byte(tp, MCU_TYPE_USB, USB_CSR_DUMMY2, ocp_data);

	ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_USB, USB_WDT11_CTRL);
	ocp_data &= ~TIMER11_EN;
	r8152_ocp_write_word(tp, MCU_TYPE_USB, USB_WDT11_CTRL, ocp_data);

	ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_PLA, PLA_LED_FEATURE);
	ocp_data &= ~LED_MODE_MASK;
	r8152_ocp_write_word(tp, MCU_TYPE_PLA, PLA_LED_FEATURE, ocp_data);

	ocp_data = FIFO_EMPTY_1FB | ROK_EXIT_LPM;
	if (tp->version == RTL_VER_04 && tp->udev->speed != USB_SPEED_SUPER)
		ocp_data |= LPM_TIMER_500MS;
	else
		ocp_data |= LPM_TIMER_500US;
	r8152_ocp_write_byte(tp, MCU_TYPE_USB, USB_LPM_CTRL, ocp_data);

	ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_USB, USB_AFE_CTRL2);
	ocp_data &= ~SEN_VAL_MASK;
	ocp_data |= SEN_VAL_NORMAL | SEL_RXIDLE;
	r8152_ocp_write_word(tp, MCU_TYPE_USB, USB_AFE_CTRL2, ocp_data);

	r8152_ocp_write_word(tp, MCU_TYPE_USB, USB_CONNECT_TIMER, 0x0001);

	r8153_power_cut_en(tp, false);

	r8152b_enable_fc(tp);
	rtl_tally_reset(tp);
}

static void r8153b_init(struct r8152 *tp)
{
	u32 ocp_data;
	int i;

	r8153_disable_aldps(tp);
	r8153b_u1u2en(tp, false);

	r8152_wait_for_bit(tp, 0, MCU_TYPE_PLA, PLA_BOOT_CTRL,
			   AUTOLOAD_DONE, 1, R8152_WAIT_TIMEOUT);

	for (i = 0; i < R8152_WAIT_TIMEOUT; i++) {
		ocp_data = r8152_ocp_reg_read(tp, OCP_PHY_STATUS) & PHY_STAT_MASK;
		if (ocp_data == PHY_STAT_LAN_ON || ocp_data == PHY_STAT_PWRDN)
			break;

		mdelay(1);
	}

	r8153_u2p3en(tp, false);

	/* MSC timer = 0xfff * 8ms = 32760 ms */
	r8152_ocp_write_word(tp, MCU_TYPE_USB, USB_MSC_TIMER, 0x0fff);

	r8153_power_cut_en(tp, false);

	/* MAC clock speed down */
	ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_PLA, PLA_MAC_PWR_CTRL2);
	ocp_data |= MAC_CLK_SPDWN_EN;
	r8152_ocp_write_word(tp, MCU_TYPE_PLA, PLA_MAC_PWR_CTRL2, ocp_data);

	ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_PLA, PLA_MAC_PWR_CTRL3);
	ocp_data &= ~PLA_MCU_SPDWN_EN;
	r8152_ocp_write_word(tp, MCU_TYPE_PLA, PLA_MAC_PWR_CTRL3, ocp_data);

	if (tp->version == RTL_VER_09) {
		/* Disable Test IO for 32QFN */
		if (r8152_ocp_read_byte(tp, MCU_TYPE_PLA, 0xdc00) & BIT(5)) {
			ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_PLA,
						       PLA_PHY_PWR);
			ocp_data |= TEST_IO_OFF;
			r8152_ocp_write_word(tp, MCU_TYPE_PLA, PLA_PHY_PWR,
					     ocp_data);
		}
	}

	/* Disable rx aggregation */
	ocp_data = r8152_ocp_read_word(tp, MCU_TYPE_USB, USB_USB_CTRL);
	ocp_data |= (RX_AGG_DISABLE | RX_ZERO_EN);
	r8152_ocp_write_word(tp, MCU_TYPE_USB, USB_USB_CTRL, ocp_data);

	rtl_tally_reset(tp);
	r8153b_hw_phy_cfg(tp);
	r8152b_enable_fc(tp);
}

static int r8152_ops_init(struct r8152 *tp)
{
	struct rtl_ops *ops = &tp->rtl_ops;
	int ret = 0;

	switch (tp->version) {
	case RTL_VER_01:
	case RTL_VER_02:
	case RTL_VER_07:
		ops->init		= rtl8152b_init;
		ops->enable		= rtl8152_enable;
		ops->disable		= rtl8152_disable;
		ops->up			= rtl8152_up;
		break;

	case RTL_VER_03:
	case RTL_VER_04:
	case RTL_VER_05:
	case RTL_VER_06:
		ops->init		= rtl8153_init;
		ops->enable		= rtl8153_enable;
		ops->disable		= rtl8153_disable;
		ops->up			= rtl8153_up;
		break;

	case RTL_VER_08:
	case RTL_VER_09:
		ops->init		= r8153b_init;
		ops->enable		= rtl8153_enable;
		ops->disable		= rtl8153_disable;
		ops->up			= rtl8153b_up;
		break;

	default:
		ret = -ENODEV;
		dev_warn(&tp->dev->edev.dev, "r8152 Unknown Device\n");
		break;
	}

	return ret;
}

static int r8152_init_common(struct r8152 *tp)
{
	int link_detected;
	u64 start;
	u8 speed;

	dev_dbg(&tp->dev->edev.dev, "** %s()\n", __func__);

	dev_info(&tp->dev->edev.dev, "Waiting for Ethernet connection...\n");
	start = get_time_ns();
	while (1) {
		speed = r8152_get_speed(tp);

		link_detected = speed & LINK_STATUS;
		if (link_detected) {
			tp->rtl_ops.enable(tp);
			dev_info(&tp->dev->edev.dev, "done.\n");
			break;
		}

		mdelay(TIMEOUT_RESOLUTION);
		if (is_timeout(start, PHY_CONNECT_TIMEOUT * MSECOND)) {
			dev_warn(&tp->dev->edev.dev, "unable to connect.\n");
			return -ETIMEDOUT;
		}
	};

	return 0;
}

static int r8152_tx_fixup(struct usbnet *dev, void *buf, int len, void *nbuf,
			  int *nlen)
{
	struct tx_desc *tx_desc = (struct tx_desc *)nbuf;
	u32 opts1;

	dev_dbg(&dev->edev.dev, "** %s(), len %d\n", __func__, len);

	opts1 = len | TX_FS | TX_LS;

	tx_desc->opts1 = cpu_to_le32(opts1);
	tx_desc->opts2 = 0;

	memcpy(nbuf + sizeof(struct tx_desc), buf, len);

	*nlen = len + sizeof(struct tx_desc);

	return 0;
}

static int r8152_rx_fixup(struct usbnet *dev, void *buf, int len)
{
	struct rx_desc *rx_desc;
	unsigned char *packet;
	u16 packet_len;

	rx_desc = (struct rx_desc *)buf;
	packet_len = le32_to_cpu(rx_desc->opts1) & RX_LEN_MASK;
	packet_len -= CRC_SIZE;

	dev_dbg(&dev->edev.dev, "%s: buf len=%d, packet len=%d\n", __func__,
		len, packet_len);

	if (packet_len > len - (sizeof(struct rx_desc) + CRC_SIZE)) {
		dev_dbg(&dev->edev.dev, "Rx: too large packet: %d\n",
			packet_len);
		return -EIO;
	}

	packet = buf + sizeof(struct rx_desc);
	net_receive(&dev->edev, packet, len - sizeof(struct rx_desc));

	return 0;
}

static int r8152_eth_reset(struct usbnet *dev)
{
	struct r8152 *tp = r8152_get_priv(dev);

	dev_dbg(&tp->dev->edev.dev, "** %s (%d)\n", __func__, __LINE__);

	tp->rtl_ops.disable(tp);
	return r8152_init_common(tp);
}

static int r8152_common_mdio_read(struct mii_bus *bus, int phy_id, int idx)
{
	struct usbnet *dev = bus->priv;
	struct r8152 *tp = r8152_get_priv(dev);
	u32 val;

	/* No phy_id is supported, so fake support of address 0 */
	if (phy_id)
		return 0xffff;

	val = r8152_mdio_read(tp, idx);

	return val & 0xffff;
}

static int r8152_common_mdio_write(struct mii_bus *bus, int phy_id, int idx,
				   u16 regval)
{
	struct usbnet *dev = bus->priv;
	struct r8152 *tp = r8152_get_priv(dev);

	/* No phy_id is supported, so fake support of address 0 */
	if (phy_id)
		return -EIO;

	r8152_mdio_write(tp, idx, regval);

	return 0;
}

static int r8152_init_mii(struct usbnet *dev)
{
	dev->miibus.read = r8152_common_mdio_read;
	dev->miibus.write = r8152_common_mdio_write;
	dev->phy_addr = 0;
	dev->miibus.priv = dev;
	dev->miibus.parent = &dev->udev->dev;

	return mdiobus_register(&dev->miibus);
}

static int r8152_write_hwaddr(struct eth_device *edev, const unsigned char *adr)
{
	struct usbnet *dev = container_of(edev, struct usbnet, edev);
	struct r8152 *tp = r8152_get_priv(dev);

	dev_dbg(&tp->dev->edev.dev, "** %s (%d)\n", __func__, __LINE__);

	r8152_ocp_write_byte(tp, MCU_TYPE_PLA, PLA_CRWECR, CRWECR_CONFIG);
	r8152_pla_ocp_write(tp, PLA_IDR, BYTE_EN_SIX_BYTES, ETH_ALEN, adr);
	r8152_ocp_write_byte(tp, MCU_TYPE_PLA, PLA_CRWECR, CRWECR_NORAML);

	dev_dbg(&tp->dev->edev.dev, "MAC %pM\n", adr);
	return 0;
}

static int r8152_read_rom_hwaddr(struct eth_device *edev, unsigned char *adr)
{
	struct usbnet *dev = container_of(edev, struct usbnet, edev);
	struct r8152 *tp = r8152_get_priv(dev);

	dev_dbg(&tp->dev->edev.dev, "** %s (%d)\n", __func__, __LINE__);
	return r8152_read_mac(tp, adr);
}

static int r8152_eth_bind(struct usbnet *dev)
{
	struct r8152 *tp;
	int ret;

	usbnet_get_endpoints(dev);

	tp = xzalloc(sizeof(*tp));
	if (!tp)
		return -ENOMEM;

	tp->txbuf = dma_alloc(R8152_TX_BURST_SIZE);
	if (!tp->txbuf)
		return -ENOMEM;

	tp->rxbuf = dma_alloc(R8152_RX_BURST_SIZE);
	if (!tp->rxbuf)
		return -ENOMEM;

	dev->driver_priv = tp;

	dev->edev.set_ethaddr = r8152_write_hwaddr;
	dev->edev.get_ethaddr = r8152_read_rom_hwaddr;

	r8152_init_mii(dev);

	tp->udev = dev->udev;
	tp->dev = dev;

	r8152_get_version(tp);

	ret = r8152_ops_init(tp);
	if (ret)
		return ret;

	tp->rtl_ops.init(tp);
	tp->rtl_ops.up(tp);

	dev->rx_urb_size = RTL8152_AGG_BUF_SZ;
	return rtl8152_set_speed(tp, AUTONEG_ENABLE,
				 tp->supports_gmii ? SPEED_1000 : SPEED_100,
				 DUPLEX_FULL);
}

static void r8152_unbind(struct usbnet *dev)
{
	struct r8152 *tp = r8152_get_priv(dev);

	tp->rtl_ops.disable(tp);
	mdiobus_unregister(&dev->miibus);
	free(tp->txbuf);
	free(tp->rxbuf);
	free(tp);
}

static struct driver_info r8152_info = {
	.description = R8152_BASE_NAME,
	.bind = r8152_eth_bind,
	.reset = r8152_eth_reset,
	.unbind = r8152_unbind,
	.flags = FLAG_ETHER | FLAG_FRAMING_AX,
	.rx_fixup = r8152_rx_fixup,
	.tx_fixup = r8152_tx_fixup,
};

static const struct usb_device_id products[] = {
{
	/* Realtek */
	USB_DEVICE(0x0bda, 0x8050),
	.driver_info = &r8152_info,
}, {
	USB_DEVICE(0x0bda, 0x8152),
	.driver_info = &r8152_info,
}, {
	USB_DEVICE(0x0bda, 0x8153),
	.driver_info = &r8152_info,
}, {
	/* Samsung */
	USB_DEVICE(0x04e8, 0xa101),
	.driver_info = &r8152_info,
}, {
	/* Lenovo */
	USB_DEVICE(0x17ef, 0x304f),
	.driver_info = &r8152_info,
}, {
	USB_DEVICE(0x17ef, 0x3052),
	.driver_info = &r8152_info,
}, {
	USB_DEVICE(0x17ef, 0x3054),
	.driver_info = &r8152_info,
}, {
	USB_DEVICE(0x17ef, 0x3057),
	.driver_info = &r8152_info,
}, {
	USB_DEVICE(0x17ef, 0x7205),
	.driver_info = &r8152_info,
}, {
	USB_DEVICE(0x17ef, 0x720a),
	.driver_info = &r8152_info,
}, {
	USB_DEVICE(0x17ef, 0x720b),
	.driver_info = &r8152_info,
}, {
	USB_DEVICE(0x17ef, 0x720c),
	.driver_info = &r8152_info,
}, {
	/* TP-LINK */
	USB_DEVICE(0x2357, 0x0601),
	.driver_info = &r8152_info,
}, {
	/* Nvidia */
	USB_DEVICE(0x0955, 0x09ff),
	.driver_info = &r8152_info,
},

	{ }		/* Terminating entry */
};

static struct usb_driver r8152_driver = {
	.name =		R8152_BASE_NAME,
	.id_table =	products,
	.probe =	usbnet_probe,
	.disconnect =	usbnet_disconnect,
};

static int __init r8152_init(void)
{
	return usb_driver_register(&r8152_driver);
}
device_initcall(r8152_init);
