/**************************************************************************
Intel Pro 1000 for ppcboot/das-u-boot
Drivers are port from Intel's Linux driver e1000-4.3.15
and from Etherboot pro 1000 driver by mrakes at vivato dot net
tested on both gig copper and gig fiber boards
***************************************************************************/
/*******************************************************************************


  Copyright(c) 1999 - 2002 Intel Corporation. All rights reserved.

 * SPDX-License-Identifier:	GPL-2.0+

  Contact Information:
  Linux NICS <linux.nics@intel.com>
  Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497

*******************************************************************************/
/*
 *  Copyright (C) Archway Digital Solutions.
 *
 *  written by Chrsitopher Li <cli at arcyway dot com> or <chrisl at gnuchina dot org>
 *  2/9/2002
 *
 *  Copyright (C) Linux Networx.
 *  Massive upgrade to work with the new intel gigabit NICs.
 *  <ebiederman at lnxi dot com>
 *
 *  Copyright 2011 Freescale Semiconductor, Inc.
 */

#include <common.h>
#include <init.h>
#include <malloc.h>
#include <linux/pci.h>
#include <dma.h>
#include "e1000.h"

static u32 inline virt_to_bus(struct pci_dev *pdev, void *adr)
{
	return (u32)adr;
}

#define PCI_VENDOR_ID_INTEL	0x8086


/* Function forward declarations */
static int e1000_setup_link(struct e1000_hw *hw);
static int e1000_setup_fiber_link(struct e1000_hw *hw);
static int e1000_setup_copper_link(struct e1000_hw *hw);
static int e1000_phy_setup_autoneg(struct e1000_hw *hw);
static void e1000_config_collision_dist(struct e1000_hw *hw);
static int e1000_config_mac_to_phy(struct e1000_hw *hw);
static int e1000_config_fc_after_link_up(struct e1000_hw *hw);
static int e1000_wait_autoneg(struct e1000_hw *hw);
static int e1000_get_speed_and_duplex(struct e1000_hw *hw, uint16_t *speed,
				       uint16_t *duplex);
static int e1000_read_phy_reg(struct e1000_hw *hw, uint32_t reg_addr,
			      uint16_t *phy_data);
static int e1000_write_phy_reg(struct e1000_hw *hw, uint32_t reg_addr,
			       uint16_t phy_data);
static int32_t e1000_phy_hw_reset(struct e1000_hw *hw);
static int e1000_phy_reset(struct e1000_hw *hw);
static int e1000_detect_gig_phy(struct e1000_hw *hw);
static void e1000_set_media_type(struct e1000_hw *hw);


static int32_t e1000_check_phy_reset_block(struct e1000_hw *hw);

static void e1000_put_hw_eeprom_semaphore(struct e1000_hw *hw);

static bool e1000_media_copper(struct e1000_hw *hw)
{
	if (!IS_ENABLED(CONFIG_DRIVER_NET_E1000_FIBER))
		return 1;

	return hw->media_type == e1000_media_type_copper;
}

static bool e1000_media_fiber(struct e1000_hw *hw)
{
	if (!IS_ENABLED(CONFIG_DRIVER_NET_E1000_FIBER))
		return 0;

	return hw->media_type == e1000_media_type_fiber;
}

static bool e1000_media_fiber_serdes(struct e1000_hw *hw)
{
	if (!IS_ENABLED(CONFIG_DRIVER_NET_E1000_FIBER))
		return 0;

	return hw->media_type == e1000_media_type_fiber ||
		hw->media_type == e1000_media_type_internal_serdes;
}

/*****************************************************************************
 * Set PHY to class A mode
 * Assumes the following operations will follow to enable the new class mode.
 *  1. Do a PHY soft reset
 *  2. Restart auto-negotiation or force link.
 *
 * hw - Struct containing variables accessed by shared code
 ****************************************************************************/
static int32_t e1000_set_phy_mode(struct e1000_hw *hw)
{
	int32_t ret_val;
	uint16_t eeprom_data;

	DEBUGFUNC();

	if ((hw->mac_type == e1000_82545_rev_3) && e1000_media_copper(hw)) {
		ret_val = e1000_read_eeprom(hw, EEPROM_PHY_CLASS_WORD,
				1, &eeprom_data);
		if (ret_val)
			return ret_val;

		if ((eeprom_data != EEPROM_RESERVED_WORD) &&
			(eeprom_data & EEPROM_PHY_CLASS_A)) {
			ret_val = e1000_write_phy_reg(hw,
					M88E1000_PHY_PAGE_SELECT, 0x000B);
			if (ret_val)
				return ret_val;
			ret_val = e1000_write_phy_reg(hw,
					M88E1000_PHY_GEN_CONTROL, 0x8104);
			if (ret_val)
				return ret_val;
		}
	}
	return E1000_SUCCESS;
}

/***************************************************************************
 *
 * Obtaining software semaphore bit (SMBI) before resetting PHY.
 *
 * hw: Struct containing variables accessed by shared code
 *
 * returns: - E1000_ERR_RESET if fail to obtain semaphore.
 *            E1000_SUCCESS at any other case.
 *
 ***************************************************************************/
static int32_t e1000_get_software_semaphore(struct e1000_hw *hw)
{
	int32_t timeout = 2049;
	uint32_t swsm;

	DEBUGFUNC();

	swsm = e1000_read_reg(hw, E1000_SWSM);
	swsm &= ~E1000_SWSM_SMBI;
	e1000_write_reg(hw, E1000_SWSM, swsm);

	if (hw->mac_type != e1000_80003es2lan)
		return E1000_SUCCESS;

	while (timeout) {
		swsm = e1000_read_reg(hw, E1000_SWSM);
		/* If SMBI bit cleared, it is now set and we hold
		 * the semaphore */
		if (!(swsm & E1000_SWSM_SMBI))
			return 0;
		mdelay(1);
		timeout--;
	}

	dev_dbg(hw->dev, "Driver can't access device - SMBI bit is set.\n");
	return -E1000_ERR_RESET;
}

/***************************************************************************
 * This function clears HW semaphore bits.
 *
 * hw: Struct containing variables accessed by shared code
 *
 * returns: - None.
 *
 ***************************************************************************/
static void e1000_put_hw_eeprom_semaphore(struct e1000_hw *hw)
{
	uint32_t swsm;

	swsm = e1000_read_reg(hw, E1000_SWSM);

	if (hw->mac_type == e1000_80003es2lan)
		/* Release both semaphores. */
		swsm &= ~(E1000_SWSM_SMBI | E1000_SWSM_SWESMBI);
	else
		swsm &= ~(E1000_SWSM_SWESMBI);

	e1000_write_reg(hw, E1000_SWSM, swsm);
}

/***************************************************************************
 *
 * Using the combination of SMBI and SWESMBI semaphore bits when resetting
 * adapter or Eeprom access.
 *
 * hw: Struct containing variables accessed by shared code
 *
 * returns: - E1000_ERR_EEPROM if fail to access EEPROM.
 *            E1000_SUCCESS at any other case.
 *
 ***************************************************************************/
static int32_t e1000_get_hw_eeprom_semaphore(struct e1000_hw *hw)
{
	int32_t timeout;
	uint32_t swsm;

	if (hw->mac_type == e1000_80003es2lan) {
		/* Get the SW semaphore. */
		if (e1000_get_software_semaphore(hw) != E1000_SUCCESS)
			return -E1000_ERR_EEPROM;
	}

	/* Get the FW semaphore. */
	timeout = 2049;
	while (timeout) {
		swsm = e1000_read_reg(hw, E1000_SWSM);
		swsm |= E1000_SWSM_SWESMBI;
		e1000_write_reg(hw, E1000_SWSM, swsm);
		/* if we managed to set the bit we got the semaphore. */
		swsm = e1000_read_reg(hw, E1000_SWSM);
		if (swsm & E1000_SWSM_SWESMBI)
			break;

		udelay(50);
		timeout--;
	}

	if (!timeout) {
		/* Release semaphores */
		e1000_put_hw_eeprom_semaphore(hw);
		dev_err(hw->dev, "Driver can't access the Eeprom - "
				"SWESMBI bit is set.\n");
		return -E1000_ERR_EEPROM;
	}
	return E1000_SUCCESS;
}

int32_t e1000_swfw_sync_acquire(struct e1000_hw *hw, uint16_t mask)
{
	uint32_t swfw_sync = 0;
	uint32_t swmask = mask;
	uint32_t fwmask = mask << 16;
	int32_t timeout = 200;

	DEBUGFUNC();
	while (timeout) {
		if (e1000_get_hw_eeprom_semaphore(hw))
			return -E1000_ERR_SWFW_SYNC;

		swfw_sync = e1000_read_reg(hw, E1000_SW_FW_SYNC);
		if (!(swfw_sync & (fwmask | swmask)))
			break;

		/* firmware currently using resource (fwmask) */
		/* or other software thread currently using resource (swmask) */
		e1000_put_hw_eeprom_semaphore(hw);
		mdelay(5);
		timeout--;
	}

	if (!timeout) {
		dev_err(hw->dev, "Driver can't access resource, SW_FW_SYNC timeout.\n");
		return -E1000_ERR_SWFW_SYNC;
	}

	swfw_sync |= swmask;
	e1000_write_reg(hw, E1000_SW_FW_SYNC, swfw_sync);

	e1000_put_hw_eeprom_semaphore(hw);
	return E1000_SUCCESS;
}

int32_t e1000_swfw_sync_release(struct e1000_hw *hw, uint16_t mask)
{
	uint32_t swfw_sync;

	if (e1000_get_hw_eeprom_semaphore(hw))
		return -E1000_ERR_SWFW_SYNC;

	swfw_sync = e1000_read_reg(hw, E1000_SW_FW_SYNC);
	swfw_sync &= ~mask;
	e1000_write_reg(hw, E1000_SW_FW_SYNC, swfw_sync);

	e1000_put_hw_eeprom_semaphore(hw);
	return E1000_SUCCESS;
}

static bool e1000_is_second_port(struct e1000_hw *hw)
{
	switch (hw->mac_type) {
	case e1000_80003es2lan:
	case e1000_82546:
	case e1000_82571:
		if (e1000_read_reg(hw, E1000_STATUS) & E1000_STATUS_FUNC_1)
			return true;
		/* Fallthrough */
	default:
		return false;
	}
}

/******************************************************************************
 * Reads the adapter's MAC address from the EEPROM and inverts the LSB for the
 * second function of dual function devices
 *
 * edev - Struct containing variables accessed by shared code
 *****************************************************************************/
static int e1000_get_ethaddr(struct eth_device *edev, unsigned char *adr)
{
	struct e1000_hw *hw = edev->priv;
	uint16_t eeprom_data;
	uint32_t reg_data = 0;
	int i;

	DEBUGFUNC();

	if (hw->mac_type == e1000_igb) {
		/* i210 preloads MAC address into RAL/RAH registers */
		reg_data = e1000_read_reg_array(hw, E1000_RA, 0);
		adr[0] = reg_data & 0xff;
		adr[1] = (reg_data >> 8) & 0xff;
		adr[2] = (reg_data >> 16) & 0xff;
		adr[3] = (reg_data >> 24) & 0xff;
		reg_data = e1000_read_reg_array(hw, E1000_RA, 1);
		adr[4] = reg_data & 0xff;
		adr[5] = (reg_data >> 8) & 0xff;
		return 0;
	}

	for (i = 0; i < NODE_ADDRESS_SIZE; i += 2) {
		if (e1000_read_eeprom(hw, i >> 1, 1, &eeprom_data) < 0) {
			dev_err(hw->dev, "EEPROM Read Error\n");
			return -E1000_ERR_EEPROM;
		}
		adr[i] = eeprom_data & 0xff;
		adr[i + 1] = (eeprom_data >> 8) & 0xff;
	}

	/* Invert the last bit if this is the second device */
	if (e1000_is_second_port(hw))
		adr[5] ^= 1;

	return 0;
}

static int e1000_set_ethaddr(struct eth_device *edev, const unsigned char *adr)
{
	struct e1000_hw *hw = edev->priv;
	uint32_t addr_low;
	uint32_t addr_high;

	DEBUGFUNC();

	dev_dbg(hw->dev, "Programming MAC Address into RAR[0]\n");

	addr_low = (adr[0] | (adr[1] << 8) | (adr[2] << 16) | (adr[3] << 24));
	addr_high = (adr[4] | (adr[5] << 8) | E1000_RAH_AV);

	e1000_write_reg_array(hw, E1000_RA, 0, addr_low);
	e1000_write_reg_array(hw, E1000_RA, 1, addr_high);

	return 0;
}

/******************************************************************************
 * Clears the VLAN filter table
 *
 * hw - Struct containing variables accessed by shared code
 *****************************************************************************/
static void e1000_clear_vfta(struct e1000_hw *hw)
{
	uint32_t offset;

	for (offset = 0; offset < E1000_VLAN_FILTER_TBL_SIZE; offset++)
		e1000_write_reg_array(hw, E1000_VFTA, offset, 0);
}

/******************************************************************************
 * Set the mac type member in the hw struct.
 *
 * hw - Struct containing variables accessed by shared code
 *****************************************************************************/
static int32_t e1000_set_mac_type(struct e1000_hw *hw)
{
	DEBUGFUNC();

	switch (hw->device_id) {
	case E1000_DEV_ID_82542:
		switch (hw->revision_id) {
		case E1000_82542_2_0_REV_ID:
			hw->mac_type = e1000_82542_rev2_0;
			break;
		case E1000_82542_2_1_REV_ID:
			hw->mac_type = e1000_82542_rev2_1;
			break;
		default:
			/* Invalid 82542 revision ID */
			return -E1000_ERR_MAC_TYPE;
		}
		break;
	case E1000_DEV_ID_82543GC_FIBER:
	case E1000_DEV_ID_82543GC_COPPER:
		hw->mac_type = e1000_82543;
		break;
	case E1000_DEV_ID_82544EI_COPPER:
	case E1000_DEV_ID_82544EI_FIBER:
	case E1000_DEV_ID_82544GC_COPPER:
	case E1000_DEV_ID_82544GC_LOM:
		hw->mac_type = e1000_82544;
		break;
	case E1000_DEV_ID_82540EM:
	case E1000_DEV_ID_82540EM_LOM:
	case E1000_DEV_ID_82540EP:
	case E1000_DEV_ID_82540EP_LOM:
	case E1000_DEV_ID_82540EP_LP:
		hw->mac_type = e1000_82540;
		break;
	case E1000_DEV_ID_82545EM_COPPER:
	case E1000_DEV_ID_82545EM_FIBER:
		hw->mac_type = e1000_82545;
		break;
	case E1000_DEV_ID_82545GM_COPPER:
	case E1000_DEV_ID_82545GM_FIBER:
	case E1000_DEV_ID_82545GM_SERDES:
		hw->mac_type = e1000_82545_rev_3;
		break;
	case E1000_DEV_ID_82546EB_COPPER:
	case E1000_DEV_ID_82546EB_FIBER:
	case E1000_DEV_ID_82546EB_QUAD_COPPER:
		hw->mac_type = e1000_82546;
		break;
	case E1000_DEV_ID_82546GB_COPPER:
	case E1000_DEV_ID_82546GB_FIBER:
	case E1000_DEV_ID_82546GB_SERDES:
	case E1000_DEV_ID_82546GB_PCIE:
	case E1000_DEV_ID_82546GB_QUAD_COPPER:
	case E1000_DEV_ID_82546GB_QUAD_COPPER_KSP3:
		hw->mac_type = e1000_82546_rev_3;
		break;
	case E1000_DEV_ID_82541EI:
	case E1000_DEV_ID_82541EI_MOBILE:
	case E1000_DEV_ID_82541ER_LOM:
		hw->mac_type = e1000_82541;
		break;
	case E1000_DEV_ID_82541ER:
	case E1000_DEV_ID_82541GI:
	case E1000_DEV_ID_82541GI_LF:
	case E1000_DEV_ID_82541GI_MOBILE:
		hw->mac_type = e1000_82541_rev_2;
		break;
	case E1000_DEV_ID_82547EI:
	case E1000_DEV_ID_82547EI_MOBILE:
		hw->mac_type = e1000_82547;
		break;
	case E1000_DEV_ID_82547GI:
		hw->mac_type = e1000_82547_rev_2;
		break;
	case E1000_DEV_ID_82571EB_COPPER:
	case E1000_DEV_ID_82571EB_FIBER:
	case E1000_DEV_ID_82571EB_SERDES:
	case E1000_DEV_ID_82571EB_SERDES_DUAL:
	case E1000_DEV_ID_82571EB_SERDES_QUAD:
	case E1000_DEV_ID_82571EB_QUAD_COPPER:
	case E1000_DEV_ID_82571PT_QUAD_COPPER:
	case E1000_DEV_ID_82571EB_QUAD_FIBER:
	case E1000_DEV_ID_82571EB_QUAD_COPPER_LOWPROFILE:
		hw->mac_type = e1000_82571;
		break;
	case E1000_DEV_ID_82572EI_COPPER:
	case E1000_DEV_ID_82572EI_FIBER:
	case E1000_DEV_ID_82572EI_SERDES:
	case E1000_DEV_ID_82572EI:
		hw->mac_type = e1000_82572;
		break;
	case E1000_DEV_ID_82573E:
	case E1000_DEV_ID_82573E_IAMT:
	case E1000_DEV_ID_82573L:
		hw->mac_type = e1000_82573;
		break;
	case E1000_DEV_ID_82574L:
		hw->mac_type = e1000_82574;
		break;
	case E1000_DEV_ID_80003ES2LAN_COPPER_SPT:
	case E1000_DEV_ID_80003ES2LAN_SERDES_SPT:
	case E1000_DEV_ID_80003ES2LAN_COPPER_DPT:
	case E1000_DEV_ID_80003ES2LAN_SERDES_DPT:
		hw->mac_type = e1000_80003es2lan;
		break;
	case E1000_DEV_ID_ICH8_IGP_M_AMT:
	case E1000_DEV_ID_ICH8_IGP_AMT:
	case E1000_DEV_ID_ICH8_IGP_C:
	case E1000_DEV_ID_ICH8_IFE:
	case E1000_DEV_ID_ICH8_IFE_GT:
	case E1000_DEV_ID_ICH8_IFE_G:
	case E1000_DEV_ID_ICH8_IGP_M:
		hw->mac_type = e1000_ich8lan;
		break;
	case E1000_DEV_ID_I350_COPPER:
	case E1000_DEV_ID_I210_UNPROGRAMMED:
	case E1000_DEV_ID_I211_UNPROGRAMMED:
	case E1000_DEV_ID_I210_COPPER:
	case E1000_DEV_ID_I211_COPPER:
	case E1000_DEV_ID_I210_COPPER_FLASHLESS:
	case E1000_DEV_ID_I210_SERDES:
	case E1000_DEV_ID_I210_SERDES_FLASHLESS:
	case E1000_DEV_ID_I210_1000BASEKX:
		hw->mac_type = e1000_igb;
		break;
	default:
		/* Should never have loaded on this device */
		return -E1000_ERR_MAC_TYPE;
	}
	return E1000_SUCCESS;
}

/******************************************************************************
 * Reset the transmit and receive units; mask and clear all interrupts.
 *
 * hw - Struct containing variables accessed by shared code
 *****************************************************************************/
static void e1000_reset_hw(struct e1000_hw *hw)
{
	uint32_t ctrl;
	uint32_t reg;

	DEBUGFUNC();

	/* For 82542 (rev 2.0), disable MWI before issuing a device reset */
	if (hw->mac_type == e1000_82542_rev2_0) {
		dev_dbg(hw->dev, "Disabling MWI on 82542 rev 2.0\n");
		pci_write_config_word(hw->pdev, PCI_COMMAND,
				hw->pci_cmd_word & ~PCI_COMMAND_INVALIDATE);
	}

	/* Disable the Transmit and Receive units.  Then delay to allow
	 * any pending transactions to complete before we hit the MAC with
	 * the global reset.
	 */
	e1000_write_reg(hw, E1000_RCTL, 0);
	e1000_write_reg(hw, E1000_TCTL, E1000_TCTL_PSP);
	e1000_write_flush(hw);

	/* Delay to allow any outstanding PCI transactions to complete before
	 * resetting the device
	 */
	mdelay(10);

	/* Issue a global reset to the MAC.  This will reset the chip's
	 * transmit, receive, DMA, and link units.  It will not effect
	 * the current PCI configuration.  The global reset bit is self-
	 * clearing, and should clear within a microsecond.
	 */
	dev_dbg(hw->dev, "Issuing a global reset to MAC\n");
	ctrl = e1000_read_reg(hw, E1000_CTRL);

	e1000_write_reg(hw, E1000_CTRL, (ctrl | E1000_CTRL_RST));

	/* Force a reload from the EEPROM if necessary */
	if (hw->mac_type == e1000_igb) {
		mdelay(20);
		reg = e1000_read_reg(hw, E1000_STATUS);
		if (reg & E1000_STATUS_PF_RST_DONE)
			dev_dbg(hw->dev, "PF OK\n");
		reg = e1000_read_reg(hw, E1000_EECD);
		if (reg & E1000_EECD_AUTO_RD)
			dev_dbg(hw->dev, "EEC OK\n");
	} else if (hw->mac_type < e1000_82540) {
		uint32_t ctrl_ext;

		/* Wait for reset to complete */
		udelay(10);
		ctrl_ext = e1000_read_reg(hw, E1000_CTRL_EXT);
		ctrl_ext |= E1000_CTRL_EXT_EE_RST;
		e1000_write_reg(hw, E1000_CTRL_EXT, ctrl_ext);
		e1000_write_flush(hw);
		/* Wait for EEPROM reload */
		mdelay(2);
	} else {
		uint32_t manc;

		/* Wait for EEPROM reload (it happens automatically) */
		mdelay(4);
		/* Dissable HW ARPs on ASF enabled adapters */
		manc = e1000_read_reg(hw, E1000_MANC);
		manc &= ~(E1000_MANC_ARP_EN);
		e1000_write_reg(hw, E1000_MANC, manc);
	}

	/* Clear interrupt mask to stop board from generating interrupts */
	if (hw->mac_type == e1000_igb)
		e1000_write_reg(hw, E1000_I210_IAM, 0);

	e1000_write_reg(hw, E1000_IMC, 0xffffffff);

	/* Clear any pending interrupt events. */
	e1000_read_reg(hw, E1000_ICR);

	/* If MWI was previously enabled, reenable it. */
	if (hw->mac_type == e1000_82542_rev2_0)
		pci_write_config_word(hw->pdev, PCI_COMMAND, hw->pci_cmd_word);

	if (hw->mac_type != e1000_igb) {
		if (hw->mac_type < e1000_82571)
			e1000_write_reg(hw, E1000_PBA, 0x00000030);
		else
			e1000_write_reg(hw, E1000_PBA, 0x000a0026);
	}
}

/******************************************************************************
 *
 * Initialize a number of hardware-dependent bits
 *
 * hw: Struct containing variables accessed by shared code
 *
 * This function contains hardware limitation workarounds for PCI-E adapters
 *
 *****************************************************************************/
static void e1000_initialize_hardware_bits(struct e1000_hw *hw)
{
	uint32_t reg_ctrl, reg_ctrl_ext;
	uint32_t reg_tarc0, reg_tarc1;
	uint32_t reg_txdctl, reg_txdctl1;

	if (hw->mac_type < e1000_82571)
		return;

	/* Settings common to all PCI-express silicon */

	/* link autonegotiation/sync workarounds */
	reg_tarc0 = e1000_read_reg(hw, E1000_TARC0);
	reg_tarc0 &= ~((1 << 30) | (1 << 29) | (1 << 28) | (1 << 27));

	/* Enable not-done TX descriptor counting */
	reg_txdctl = e1000_read_reg(hw, E1000_TXDCTL);
	reg_txdctl |= E1000_TXDCTL_COUNT_DESC;
	e1000_write_reg(hw, E1000_TXDCTL, reg_txdctl);

	reg_txdctl1 = e1000_read_reg(hw, E1000_TXDCTL1);
	reg_txdctl1 |= E1000_TXDCTL_COUNT_DESC;
	e1000_write_reg(hw, E1000_TXDCTL1, reg_txdctl1);

	switch (hw->mac_type) {
	case e1000_82571:
	case e1000_82572:
		/* Clear PHY TX compatible mode bits */
		reg_tarc1 = e1000_read_reg(hw, E1000_TARC1);
		reg_tarc1 &= ~((1 << 30) | (1 << 29));

		/* link autonegotiation/sync workarounds */
		reg_tarc0 |= (1 << 26) | (1 << 25) | (1 << 24) | (1 << 23);

		/* TX ring control fixes */
		reg_tarc1 |= (1 << 26) | (1 << 25) | (1 << 24);

		/* Multiple read bit is reversed polarity */
		if (e1000_read_reg(hw, E1000_TCTL) & E1000_TCTL_MULR)
			reg_tarc1 &= ~(1 << 28);
		else
			reg_tarc1 |= (1 << 28);

		e1000_write_reg(hw, E1000_TARC1, reg_tarc1);
		break;
	case e1000_82573:
	case e1000_82574:
		reg_ctrl_ext = e1000_read_reg(hw, E1000_CTRL_EXT);
		reg_ctrl_ext &= ~(1 << 23);
		reg_ctrl_ext |= (1 << 22);

		/* TX byte count fix */
		reg_ctrl = e1000_read_reg(hw, E1000_CTRL);
		reg_ctrl &= ~(1 << 29);

		e1000_write_reg(hw, E1000_CTRL_EXT, reg_ctrl_ext);
		e1000_write_reg(hw, E1000_CTRL, reg_ctrl);
		break;
	case e1000_80003es2lan:
		/* improve small packet performace for fiber/serdes */
		if (e1000_media_fiber_serdes(hw))
			reg_tarc0 &= ~(1 << 20);

		/* Multiple read bit is reversed polarity */
		reg_tarc1 = e1000_read_reg(hw, E1000_TARC1);
		if (e1000_read_reg(hw, E1000_TCTL) & E1000_TCTL_MULR)
			reg_tarc1 &= ~(1 << 28);
		else
			reg_tarc1 |= (1 << 28);

		e1000_write_reg(hw, E1000_TARC1, reg_tarc1);
		break;
	case e1000_ich8lan:
		/* Reduce concurrent DMA requests to 3 from 4 */
		if ((hw->revision_id < 3) ||
		   ((hw->device_id != E1000_DEV_ID_ICH8_IGP_M_AMT) &&
		    (hw->device_id != E1000_DEV_ID_ICH8_IGP_M)))
			reg_tarc0 |= (1 << 29) | (1 << 28);

		reg_ctrl_ext = e1000_read_reg(hw, E1000_CTRL_EXT);
		reg_ctrl_ext |= (1 << 22);
		e1000_write_reg(hw, E1000_CTRL_EXT, reg_ctrl_ext);

		/* workaround TX hang with TSO=on */
		reg_tarc0 |= (1 << 27) | (1 << 26) | (1 << 24) | (1 << 23);

		/* Multiple read bit is reversed polarity */
		reg_tarc1 = e1000_read_reg(hw, E1000_TARC1);
		if (e1000_read_reg(hw, E1000_TCTL) & E1000_TCTL_MULR)
			reg_tarc1 &= ~(1 << 28);
		else
			reg_tarc1 |= (1 << 28);

		/* workaround TX hang with TSO=on */
		reg_tarc1 |= (1 << 30) | (1 << 26) | (1 << 24);

		e1000_write_reg(hw, E1000_TARC1, reg_tarc1);
		break;
	case e1000_igb:
		return;
	default:
		break;
	}

	e1000_write_reg(hw, E1000_TARC0, reg_tarc0);
}

static int e1000_open(struct eth_device *edev)
{
	struct e1000_hw *hw = edev->priv;
	uint32_t ctrl_ext;
	int32_t ret_val;
	uint32_t ctrl;
	uint32_t reg_data;

	/* Call a subroutine to configure the link and setup flow control. */
	ret_val = e1000_setup_link(hw);
	if (ret_val)
		return ret_val;

	/* Set the transmit descriptor write-back policy */
	if (hw->mac_type > e1000_82544) {
		ctrl = e1000_read_reg(hw, E1000_TXDCTL);
		ctrl &= ~E1000_TXDCTL_WTHRESH;
		ctrl |= E1000_TXDCTL_FULL_TX_DESC_WB;
		e1000_write_reg(hw, E1000_TXDCTL, ctrl);
	}

	/* Set the receive descriptor write back policy */
	if (hw->mac_type >= e1000_82571) {
		ctrl = e1000_read_reg(hw, E1000_RXDCTL);
		ctrl &= ~E1000_RXDCTL_WTHRESH;
		ctrl |= E1000_RXDCTL_FULL_RX_DESC_WB;
		e1000_write_reg(hw, E1000_RXDCTL, ctrl);
	}

	switch (hw->mac_type) {
	case e1000_80003es2lan:
		/* Enable retransmit on late collisions */
		reg_data = e1000_read_reg(hw, E1000_TCTL);
		reg_data |= E1000_TCTL_RTLC;
		e1000_write_reg(hw, E1000_TCTL, reg_data);

		/* Configure Gigabit Carry Extend Padding */
		reg_data = e1000_read_reg(hw, E1000_TCTL_EXT);
		reg_data &= ~E1000_TCTL_EXT_GCEX_MASK;
		reg_data |= DEFAULT_80003ES2LAN_TCTL_EXT_GCEX;
		e1000_write_reg(hw, E1000_TCTL_EXT, reg_data);

		/* Configure Transmit Inter-Packet Gap */
		reg_data = e1000_read_reg(hw, E1000_TIPG);
		reg_data &= ~E1000_TIPG_IPGT_MASK;
		reg_data |= DEFAULT_80003ES2LAN_TIPG_IPGT_1000;
		e1000_write_reg(hw, E1000_TIPG, reg_data);

		reg_data = e1000_read_reg_array(hw, E1000_FFLT, 1);
		reg_data &= ~0x00100000;
		e1000_write_reg_array(hw, E1000_FFLT, 1, reg_data);
		/* Fall through */
	case e1000_82571:
	case e1000_82572:
	case e1000_ich8lan:
		ctrl = e1000_read_reg(hw, E1000_TXDCTL1);
		ctrl &= ~E1000_TXDCTL_WTHRESH;
		ctrl |= E1000_TXDCTL_FULL_TX_DESC_WB;
		e1000_write_reg(hw, E1000_TXDCTL1, ctrl);
		break;
	case e1000_82573:
	case e1000_82574:
		reg_data = e1000_read_reg(hw, E1000_GCR);
		reg_data |= E1000_GCR_L1_ACT_WITHOUT_L0S_RX;
		e1000_write_reg(hw, E1000_GCR, reg_data);
	case e1000_igb:
	default:
		break;
	}

	if (hw->device_id == E1000_DEV_ID_82546GB_QUAD_COPPER ||
	    hw->device_id == E1000_DEV_ID_82546GB_QUAD_COPPER_KSP3) {
		ctrl_ext = e1000_read_reg(hw, E1000_CTRL_EXT);
		/* Relaxed ordering must be disabled to avoid a parity
		 * error crash in a PCI slot. */
		ctrl_ext |= E1000_CTRL_EXT_RO_DIS;
		e1000_write_reg(hw, E1000_CTRL_EXT, ctrl_ext);
	}

	return 0;
}

/******************************************************************************
 * Configures flow control and link settings.
 *
 * hw - Struct containing variables accessed by shared code
 *
 * Determines which flow control settings to use. Calls the apropriate media-
 * specific link configuration function. Configures the flow control settings.
 * Assuming the adapter has a valid link partner, a valid link should be
 * established. Assumes the hardware has previously been reset and the
 * transmitter and receiver are not enabled.
 *****************************************************************************/
static int e1000_setup_link(struct e1000_hw *hw)
{
	int32_t ret_val;
	uint32_t ctrl_ext;
	uint16_t eeprom_data;

	DEBUGFUNC();

	/* In the case of the phy reset being blocked, we already have a link.
	 * We do not have to set it up again. */
	if (e1000_check_phy_reset_block(hw))
		return E1000_SUCCESS;

	switch (hw->mac_type) {
	case e1000_ich8lan:
	case e1000_82573:
	case e1000_82574:
	case e1000_igb:
		hw->fc = e1000_fc_full;
		break;
	default:
		/* Read and store word 0x0F of the EEPROM. This word
		 * contains bits that determine the hardware's default
		 * PAUSE (flow control) mode, a bit that determines
		 * whether the HW defaults to enabling or disabling
		 * auto-negotiation, and the direction of the SW
		 * defined pins. If there is no SW over-ride of the
		 * flow control setting, then the variable hw->fc will
		 * be initialized based on a value in the EEPROM.
		 */
		ret_val = e1000_read_eeprom(hw, EEPROM_INIT_CONTROL2_REG, 1,
					    &eeprom_data);
		if (ret_val < 0) {
			dev_err(hw->dev, "EEPROM Read Error\n");
			return ret_val;
		}

		if ((eeprom_data & EEPROM_WORD0F_PAUSE_MASK) == 0)
			hw->fc = e1000_fc_none;
		else if ((eeprom_data & EEPROM_WORD0F_PAUSE_MASK) == EEPROM_WORD0F_ASM_DIR)
			hw->fc = e1000_fc_tx_pause;
		else
			hw->fc = e1000_fc_full;
		break;
	}

	/* We want to save off the original Flow Control configuration just
	 * in case we get disconnected and then reconnected into a different
	 * hub or switch with different Flow Control capabilities.
	 */
	if (hw->mac_type == e1000_82542_rev2_0)
		hw->fc &= ~e1000_fc_tx_pause;

	hw->original_fc = hw->fc;

	dev_dbg(hw->dev, "After fix-ups FlowControl is now = %x\n", hw->fc);

	/* Take the 4 bits from EEPROM word 0x0F that determine the initial
	 * polarity value for the SW controlled pins, and setup the
	 * Extended Device Control reg with that info.
	 * This is needed because one of the SW controlled pins is used for
	 * signal detection.  So this should be done before e1000_setup_pcs_link()
	 * or e1000_phy_setup() is called.
	 */
	if (hw->mac_type == e1000_82543) {
		ctrl_ext = ((eeprom_data & EEPROM_WORD0F_SWPDIO_EXT) <<
			    SWDPIO__EXT_SHIFT);
		e1000_write_reg(hw, E1000_CTRL_EXT, ctrl_ext);
	}

	/* Call the necessary subroutine to configure the link. */
	if (e1000_media_fiber(hw))
		ret_val = e1000_setup_fiber_link(hw);
	else
		ret_val = e1000_setup_copper_link(hw);

	if (ret_val < 0)
		return ret_val;

	/* Initialize the flow control address, type, and PAUSE timer
	 * registers to their default values.  This is done even if flow
	 * control is disabled, because it does not hurt anything to
	 * initialize these registers.
	 */
	dev_dbg(hw->dev, "Initializing Flow Control address, type and timer regs\n");

	/* FCAL/H and FCT are hardcoded to standard values in e1000_ich8lan. */
	if (hw->mac_type != e1000_ich8lan) {
		e1000_write_reg(hw, E1000_FCT, FLOW_CONTROL_TYPE);
		e1000_write_reg(hw, E1000_FCAH, FLOW_CONTROL_ADDRESS_HIGH);
		e1000_write_reg(hw, E1000_FCAL, FLOW_CONTROL_ADDRESS_LOW);
	}

	e1000_write_reg(hw, E1000_FCTTV, E1000_FC_PAUSE_TIME);

	/* Set the flow control receive threshold registers.  Normally,
	 * these registers will be set to a default threshold that may be
	 * adjusted later by the driver's runtime code.  However, if the
	 * ability to transmit pause frames in not enabled, then these
	 * registers will be set to 0.
	 */
	if (hw->fc & e1000_fc_tx_pause) {
		/* We need to set up the Receive Threshold high and low water marks
		 * as well as (optionally) enabling the transmission of XON frames.
		 */
		e1000_write_reg(hw, E1000_FCRTL, E1000_FC_LOW_THRESH | E1000_FCRTL_XONE);
		e1000_write_reg(hw, E1000_FCRTH, E1000_FC_HIGH_THRESH);
	} else {
		e1000_write_reg(hw, E1000_FCRTL, 0);
		e1000_write_reg(hw, E1000_FCRTH, 0);
	}

	return ret_val;
}

/******************************************************************************
 * Sets up link for a fiber based adapter
 *
 * hw - Struct containing variables accessed by shared code
 *
 * Manipulates Physical Coding Sublayer functions in order to configure
 * link. Assumes the hardware has been previously reset and the transmitter
 * and receiver are not enabled.
 *****************************************************************************/
static int e1000_setup_fiber_link(struct e1000_hw *hw)
{
	uint32_t ctrl;
	uint32_t status;
	uint32_t txcw = 0;
	uint32_t i;
	uint32_t signal;

	DEBUGFUNC();

	/* On adapters with a MAC newer that 82544, SW Defineable pin 1 will be
	 * set when the optics detect a signal. On older adapters, it will be
	 * cleared when there is a signal
	 */
	ctrl = e1000_read_reg(hw, E1000_CTRL);
	if ((hw->mac_type > e1000_82544) && !(ctrl & E1000_CTRL_ILOS))
		signal = E1000_CTRL_SWDPIN1;
	else
		signal = 0;

	/* Take the link out of reset */
	ctrl &= ~E1000_CTRL_LRST;

	e1000_config_collision_dist(hw);

	/* Check for a software override of the flow control settings, and setup
	 * the device accordingly.  If auto-negotiation is enabled, then software
	 * will have to set the "PAUSE" bits to the correct value in the Tranmsit
	 * Config Word Register (TXCW) and re-start auto-negotiation.  However, if
	 * auto-negotiation is disabled, then software will have to manually
	 * configure the two flow control enable bits in the CTRL register.
	 *
	 * The possible values of the "fc" parameter are:
	 *	0:  Flow control is completely disabled
	 *	1:  Rx flow control is enabled (we can receive pause frames, but
	 *	    not send pause frames).
	 *	2:  Tx flow control is enabled (we can send pause frames but we do
	 *	    not support receiving pause frames).
	 *	3:  Both Rx and TX flow control (symmetric) are enabled.
	 */
	switch (hw->fc) {
	case e1000_fc_none:
		/* Flow control is completely disabled by a software over-ride. */
		txcw = E1000_TXCW_ANE | E1000_TXCW_FD;
		break;
	case e1000_fc_rx_pause:
		/* RX Flow control is enabled and TX Flow control is disabled by a
		 * software over-ride. Since there really isn't a way to advertise
		 * that we are capable of RX Pause ONLY, we will advertise that we
		 * support both symmetric and asymmetric RX PAUSE. Later, we will
		 *  disable the adapter's ability to send PAUSE frames.
		 */
		txcw = E1000_TXCW_ANE | E1000_TXCW_FD | E1000_TXCW_PAUSE_MASK;
		break;
	case e1000_fc_tx_pause:
		/* TX Flow control is enabled, and RX Flow control is disabled, by a
		 * software over-ride.
		 */
		txcw = E1000_TXCW_ANE | E1000_TXCW_FD | E1000_TXCW_ASM_DIR;
		break;
	case e1000_fc_full:
		/* Flow control (both RX and TX) is enabled by a software over-ride. */
		txcw = E1000_TXCW_ANE | E1000_TXCW_FD | E1000_TXCW_PAUSE_MASK;
		break;
	default:
		dev_err(hw->dev, "Flow control param set incorrectly\n");
		return -E1000_ERR_CONFIG;
		break;
	}

	/* Since auto-negotiation is enabled, take the link out of reset (the link
	 * will be in reset, because we previously reset the chip). This will
	 * restart auto-negotiation.  If auto-neogtiation is successful then the
	 * link-up status bit will be set and the flow control enable bits (RFCE
	 * and TFCE) will be set according to their negotiated value.
	 */
	dev_dbg(hw->dev, "Auto-negotiation enabled (%#x)\n", txcw);

	e1000_write_reg(hw, E1000_TXCW, txcw);
	e1000_write_reg(hw, E1000_CTRL, ctrl);
	e1000_write_flush(hw);

	mdelay(1);

	/* If we have a signal (the cable is plugged in) then poll for a "Link-Up"
	 * indication in the Device Status Register.  Time-out if a link isn't
	 * seen in 500 milliseconds seconds (Auto-negotiation should complete in
	 * less than 500 milliseconds even if the other end is doing it in SW).
	 */
	if ((e1000_read_reg(hw, E1000_CTRL) & E1000_CTRL_SWDPIN1) == signal) {
		dev_dbg(hw->dev, "Looking for Link\n");
		for (i = 0; i < (LINK_UP_TIMEOUT / 10); i++) {
			mdelay(10);
			status = e1000_read_reg(hw, E1000_STATUS);
			if (status & E1000_STATUS_LU)
				break;
		}
		if (i == (LINK_UP_TIMEOUT / 10)) {
			/* AutoNeg failed to achieve a link, so we'll call
			 * e1000_check_for_link. This routine will force the link up if we
			 * detect a signal. This will allow us to communicate with
			 * non-autonegotiating link partners.
			 */
			dev_err(hw->dev, "Never got a valid link from auto-neg!!!\n");
			hw->autoneg_failed = 1;
			return -E1000_ERR_NOLINK;
		} else {
			hw->autoneg_failed = 0;
			dev_dbg(hw->dev, "Valid Link Found\n");
		}
	} else {
		dev_err(hw->dev, "No Signal Detected\n");
		return -E1000_ERR_NOLINK;
	}
	return 0;
}

/******************************************************************************
* Make sure we have a valid PHY and change PHY mode before link setup.
*
* hw - Struct containing variables accessed by shared code
******************************************************************************/
static int32_t e1000_copper_link_preconfig(struct e1000_hw *hw)
{
	uint32_t ctrl;
	int32_t ret_val;
	uint16_t phy_data;

	DEBUGFUNC();

	ctrl = e1000_read_reg(hw, E1000_CTRL);
	/* With 82543, we need to force speed and duplex on the MAC equal to what
	 * the PHY speed and duplex configuration is. In addition, we need to
	 * perform a hardware reset on the PHY to take it out of reset.
	 */
	if (hw->mac_type > e1000_82543) {
		ctrl |= E1000_CTRL_SLU;
		ctrl &= ~(E1000_CTRL_FRCSPD | E1000_CTRL_FRCDPX);
		e1000_write_reg(hw, E1000_CTRL, ctrl);
	} else {
		ctrl |= (E1000_CTRL_FRCSPD | E1000_CTRL_FRCDPX
				| E1000_CTRL_SLU);
		e1000_write_reg(hw, E1000_CTRL, ctrl);
		ret_val = e1000_phy_hw_reset(hw);
		if (ret_val)
			return ret_val;
	}

	/* Make sure we have a valid PHY */
	ret_val = e1000_detect_gig_phy(hw);
	if (ret_val) {
		dev_err(hw->dev, "Error, did not detect valid phy.\n");
		return ret_val;
	}
	dev_dbg(hw->dev, "Phy ID = %x \n", hw->phy_id);

	/* Set PHY to class A mode (if necessary) */
	ret_val = e1000_set_phy_mode(hw);
	if (ret_val)
		return ret_val;

	if ((hw->mac_type == e1000_82545_rev_3) ||
	    (hw->mac_type == e1000_82546_rev_3)) {
		ret_val = e1000_read_phy_reg(hw, M88E1000_PHY_SPEC_CTRL, &phy_data);
		phy_data |= 0x00000008;
		ret_val = e1000_write_phy_reg(hw, M88E1000_PHY_SPEC_CTRL, phy_data);
	}

	return E1000_SUCCESS;
}

/*****************************************************************************
 *
 * This function sets the lplu state according to the active flag.  When
 * activating lplu this function also disables smart speed and vise versa.
 * lplu will not be activated unless the device autonegotiation advertisment
 * meets standards of either 10 or 10/100 or 10/100/1000 at all duplexes.
 * hw: Struct containing variables accessed by shared code
 * active - true to enable lplu false to disable lplu.
 *
 * returns: - E1000_ERR_PHY if fail to read/write the PHY
 *            E1000_SUCCESS at any other case.
 *
 ****************************************************************************/

static int32_t e1000_set_d3_lplu_state_off(struct e1000_hw *hw)
{
	uint32_t phy_ctrl = 0;
	int32_t ret_val;
	uint16_t phy_data = 0;
	DEBUGFUNC();

	/* During driver activity LPLU should not be used or it will attain link
	 * from the lowest speeds starting from 10Mbps. The capability is used
	 * for Dx transitions and states */
	if (hw->mac_type == e1000_82541_rev_2
			|| hw->mac_type == e1000_82547_rev_2) {
		ret_val = e1000_read_phy_reg(hw, IGP01E1000_GMII_FIFO,
				&phy_data);
		if (ret_val)
			return ret_val;
	} else if (hw->mac_type == e1000_ich8lan) {
		/* MAC writes into PHY register based on the state transition
		 * and start auto-negotiation. SW driver can overwrite the
		 * settings in CSR PHY power control E1000_PHY_CTRL register. */
		phy_ctrl = e1000_read_reg(hw, E1000_PHY_CTRL);
	} else {
		ret_val = e1000_read_phy_reg(hw, IGP02E1000_PHY_POWER_MGMT, &phy_data);
		if (ret_val)
			return ret_val;
	}

	if (hw->mac_type == e1000_82541_rev_2 ||
	    hw->mac_type == e1000_82547_rev_2) {
		phy_data &= ~IGP01E1000_GMII_FLEX_SPD;
		ret_val = e1000_write_phy_reg(hw, IGP01E1000_GMII_FIFO, phy_data);
		if (ret_val)
			return ret_val;
	} else {
		if (hw->mac_type == e1000_ich8lan) {
			phy_ctrl &= ~E1000_PHY_CTRL_NOND0A_LPLU;
			e1000_write_reg(hw, E1000_PHY_CTRL, phy_ctrl);
		} else {
			phy_data &= ~IGP02E1000_PM_D3_LPLU;
			ret_val = e1000_write_phy_reg(hw,
				IGP02E1000_PHY_POWER_MGMT, phy_data);
			if (ret_val)
				return ret_val;
		}
	}

	return E1000_SUCCESS;
}

/*****************************************************************************
 *
 * This function sets the lplu d0 state according to the active flag.  When
 * activating lplu this function also disables smart speed and vise versa.
 * lplu will not be activated unless the device autonegotiation advertisment
 * meets standards of either 10 or 10/100 or 10/100/1000 at all duplexes.
 * hw: Struct containing variables accessed by shared code
 * active - true to enable lplu false to disable lplu.
 *
 * returns: - E1000_ERR_PHY if fail to read/write the PHY
 *            E1000_SUCCESS at any other case.
 *
 ****************************************************************************/

static int32_t e1000_set_d0_lplu_state_off(struct e1000_hw *hw)
{
	uint32_t phy_ctrl = 0;
	int32_t ret_val;
	uint16_t phy_data;
	DEBUGFUNC();

	if (hw->mac_type <= e1000_82547_rev_2)
		return E1000_SUCCESS;

	if (hw->mac_type == e1000_ich8lan ||
	    hw->mac_type == e1000_igb) {
		phy_ctrl = e1000_read_reg(hw, E1000_PHY_CTRL);
		phy_ctrl &= ~E1000_PHY_CTRL_D0A_LPLU;
		e1000_write_reg(hw, E1000_PHY_CTRL, phy_ctrl);
	} else {
		ret_val = e1000_read_phy_reg(hw, IGP02E1000_PHY_POWER_MGMT,
				&phy_data);
		if (ret_val)
			return ret_val;

		phy_data &= ~IGP02E1000_PM_D0_LPLU;

		ret_val = e1000_write_phy_reg(hw,
				IGP02E1000_PHY_POWER_MGMT, phy_data);
		if (ret_val)
			return ret_val;
	}

	return E1000_SUCCESS;
}

/********************************************************************
* Copper link setup for e1000_phy_igp series.
*
* hw - Struct containing variables accessed by shared code
*********************************************************************/
static int32_t e1000_copper_link_igp_setup(struct e1000_hw *hw)
{
	uint32_t led_ctrl;
	int32_t ret_val;
	uint16_t phy_data;

	DEBUGFUNC();

	ret_val = e1000_phy_reset(hw);
	if (ret_val) {
		dev_err(hw->dev, "Error Resetting the PHY\n");
		return ret_val;
	}

	/* Wait 15ms for MAC to configure PHY from eeprom settings */
	mdelay(15);
	if (hw->mac_type != e1000_ich8lan) {
		/* Configure activity LED after PHY reset */
		led_ctrl = e1000_read_reg(hw, E1000_LEDCTL);
		led_ctrl &= IGP_ACTIVITY_LED_MASK;
		led_ctrl |= (IGP_ACTIVITY_LED_ENABLE | IGP_LED3_MODE);
		e1000_write_reg(hw, E1000_LEDCTL, led_ctrl);
	}

	/* The NVM settings will configure LPLU in D3 for IGP2 and IGP3 PHYs */
	if (hw->phy_type == e1000_phy_igp) {
		/* disable lplu d3 during driver init */
		ret_val = e1000_set_d3_lplu_state_off(hw);
		if (ret_val) {
			dev_err(hw->dev, "Error Disabling LPLU D3\n");
			return ret_val;
		}
	}

	/* disable lplu d0 during driver init */
	ret_val = e1000_set_d0_lplu_state_off(hw);
	if (ret_val) {
		dev_err(hw->dev, "Error Disabling LPLU D0\n");
		return ret_val;
	}

	/* Configure mdi-mdix settings */
	ret_val = e1000_read_phy_reg(hw, IGP01E1000_PHY_PORT_CTRL, &phy_data);
	if (ret_val)
		return ret_val;

	if ((hw->mac_type == e1000_82541) || (hw->mac_type == e1000_82547)) {
		/* Force MDI for earlier revs of the IGP PHY */
		phy_data &= ~(IGP01E1000_PSCR_AUTO_MDIX
				| IGP01E1000_PSCR_FORCE_MDI_MDIX);
	} else {
		phy_data |= IGP01E1000_PSCR_AUTO_MDIX;
	}
	ret_val = e1000_write_phy_reg(hw, IGP01E1000_PHY_PORT_CTRL, phy_data);
	if (ret_val)
		return ret_val;

	/* set auto-master slave resolution settings */
	/* when autonegotiation advertisment is only 1000Mbps then we
	  * should disable SmartSpeed and enable Auto MasterSlave
	  * resolution as hardware default. */
	if (hw->autoneg_advertised == ADVERTISE_1000_FULL) {
		/* Disable SmartSpeed */
		ret_val = e1000_read_phy_reg(hw,
				IGP01E1000_PHY_PORT_CONFIG, &phy_data);
		if (ret_val)
			return ret_val;
		phy_data &= ~IGP01E1000_PSCFR_SMART_SPEED;
		ret_val = e1000_write_phy_reg(hw,
				IGP01E1000_PHY_PORT_CONFIG, phy_data);
		if (ret_val)
			return ret_val;
		/* Set auto Master/Slave resolution process */
		ret_val = e1000_read_phy_reg(hw, PHY_1000T_CTRL,
				&phy_data);
		if (ret_val)
			return ret_val;
		phy_data &= ~CR_1000T_MS_ENABLE;
		ret_val = e1000_write_phy_reg(hw, PHY_1000T_CTRL,
				phy_data);
		if (ret_val)
			return ret_val;
	}

	ret_val = e1000_read_phy_reg(hw, PHY_1000T_CTRL, &phy_data);
	if (ret_val)
		return ret_val;

	ret_val = e1000_write_phy_reg(hw, PHY_1000T_CTRL, phy_data);
	if (ret_val)
		return ret_val;

	return E1000_SUCCESS;
}

/*****************************************************************************
 * This function checks the mode of the firmware.
 *
 * returns  - true when the mode is IAMT or false.
 ****************************************************************************/
static bool e1000_check_mng_mode(struct e1000_hw *hw)
{
	uint32_t fwsm;

	DEBUGFUNC();

	fwsm = e1000_read_reg(hw, E1000_FWSM);

	if (hw->mac_type == e1000_ich8lan) {
		if ((fwsm & E1000_FWSM_MODE_MASK) ==
		    (E1000_MNG_ICH_IAMT_MODE << E1000_FWSM_MODE_SHIFT))
			return true;
	} else if ((fwsm & E1000_FWSM_MODE_MASK) ==
			(E1000_MNG_IAMT_MODE << E1000_FWSM_MODE_SHIFT))
			return true;

	return false;
}

static int32_t e1000_write_kmrn_reg(struct e1000_hw *hw, uint32_t reg_addr, uint16_t data)
{
	uint16_t swfw = E1000_SWFW_PHY0_SM;
	uint32_t reg_val;
	DEBUGFUNC();

	if (e1000_is_second_port(hw))
		swfw = E1000_SWFW_PHY1_SM;

	if (e1000_swfw_sync_acquire(hw, swfw))
		return -E1000_ERR_SWFW_SYNC;

	reg_val = ((reg_addr << E1000_KUMCTRLSTA_OFFSET_SHIFT)
			& E1000_KUMCTRLSTA_OFFSET) | data;
	e1000_write_reg(hw, E1000_KUMCTRLSTA, reg_val);
	udelay(2);

	if (e1000_swfw_sync_release(hw, swfw) < 0)
		dev_warn(hw->dev,
			 "Timeout while releasing SWFW_SYNC bits (0x%08x)\n",
			 swfw);

	return E1000_SUCCESS;
}

static int32_t e1000_read_kmrn_reg(struct e1000_hw *hw, uint32_t reg_addr, uint16_t *data)
{
	uint16_t swfw = E1000_SWFW_PHY0_SM;
	uint32_t reg_val;
	DEBUGFUNC();

	if (e1000_is_second_port(hw))
		swfw = E1000_SWFW_PHY1_SM;

	if (e1000_swfw_sync_acquire(hw, swfw)) {
		debug("%s[%i]\n", __func__, __LINE__);
		return -E1000_ERR_SWFW_SYNC;
	}

	/* Write register address */
	reg_val = ((reg_addr << E1000_KUMCTRLSTA_OFFSET_SHIFT) &
			E1000_KUMCTRLSTA_OFFSET) | E1000_KUMCTRLSTA_REN;
	e1000_write_reg(hw, E1000_KUMCTRLSTA, reg_val);
	udelay(2);

	/* Read the data returned */
	reg_val = e1000_read_reg(hw, E1000_KUMCTRLSTA);
	*data = (uint16_t)reg_val;

	if (e1000_swfw_sync_release(hw, swfw) < 0)
		dev_warn(hw->dev,
			 "Timeout while releasing SWFW_SYNC bits (0x%08x)\n",
			 swfw);

	return E1000_SUCCESS;
}

/********************************************************************
* Copper link setup for e1000_phy_gg82563 series.
*
* hw - Struct containing variables accessed by shared code
*********************************************************************/
static int32_t e1000_copper_link_ggp_setup(struct e1000_hw *hw)
{
	int32_t ret_val;
	uint16_t phy_data;
	uint32_t reg_data;

	DEBUGFUNC();

	/* Enable CRS on TX for half-duplex operation. */
	ret_val = e1000_read_phy_reg(hw,
			GG82563_PHY_MAC_SPEC_CTRL, &phy_data);
	if (ret_val)
		return ret_val;

	phy_data |= GG82563_MSCR_ASSERT_CRS_ON_TX;
	/* Use 25MHz for both link down and 1000BASE-T for Tx clock */
	phy_data |= GG82563_MSCR_TX_CLK_1000MBPS_25MHZ;

	ret_val = e1000_write_phy_reg(hw,
			GG82563_PHY_MAC_SPEC_CTRL, phy_data);
	if (ret_val)
		return ret_val;

	/* Options:
	 *   MDI/MDI-X = 0 (default)
	 *   0 - Auto for all speeds
	 *   1 - MDI mode
	 *   2 - MDI-X mode
	 *   3 - Auto for 1000Base-T only (MDI-X for 10/100Base-T modes)
	 */
	ret_val = e1000_read_phy_reg(hw, GG82563_PHY_SPEC_CTRL, &phy_data);
	if (ret_val)
		return ret_val;

	phy_data |= GG82563_PSCR_CROSSOVER_MODE_AUTO;

	/* Options:
	 *   disable_polarity_correction = 0 (default)
	 *       Automatic Correction for Reversed Cable Polarity
	 *   0 - Disabled
	 *   1 - Enabled
	 */
	phy_data &= ~GG82563_PSCR_POLARITY_REVERSAL_DISABLE;
	ret_val = e1000_write_phy_reg(hw, GG82563_PHY_SPEC_CTRL, phy_data);
	if (ret_val)
		return ret_val;

	/* SW Reset the PHY so all changes take effect */
	ret_val = e1000_phy_reset(hw);
	if (ret_val) {
		dev_err(hw->dev, "Error Resetting the PHY\n");
		return ret_val;
	}

	/* Bypass RX and TX FIFO's */
	ret_val = e1000_write_kmrn_reg(hw,
			E1000_KUMCTRLSTA_OFFSET_FIFO_CTRL,
			E1000_KUMCTRLSTA_FIFO_CTRL_RX_BYPASS
			| E1000_KUMCTRLSTA_FIFO_CTRL_TX_BYPASS);
	if (ret_val)
		return ret_val;

	ret_val = e1000_read_phy_reg(hw, GG82563_PHY_SPEC_CTRL_2, &phy_data);
	if (ret_val)
		return ret_val;

	phy_data &= ~GG82563_PSCR2_REVERSE_AUTO_NEG;
	ret_val = e1000_write_phy_reg(hw, GG82563_PHY_SPEC_CTRL_2, phy_data);
	if (ret_val)
		return ret_val;

	reg_data = e1000_read_reg(hw, E1000_CTRL_EXT);
	reg_data &= ~(E1000_CTRL_EXT_LINK_MODE_MASK);
	e1000_write_reg(hw, E1000_CTRL_EXT, reg_data);

	ret_val = e1000_read_phy_reg(hw, GG82563_PHY_PWR_MGMT_CTRL, &phy_data);
	if (ret_val)
		return ret_val;

	/* Do not init these registers when the HW is in IAMT mode, since the
	 * firmware will have already initialized them.  We only initialize
	 * them if the HW is not in IAMT mode.
	 */
	if (e1000_check_mng_mode(hw) == false) {
		/* Enable Electrical Idle on the PHY */
		phy_data |= GG82563_PMCR_ENABLE_ELECTRICAL_IDLE;
		ret_val = e1000_write_phy_reg(hw,
				GG82563_PHY_PWR_MGMT_CTRL, phy_data);
		if (ret_val)
			return ret_val;

		ret_val = e1000_read_phy_reg(hw,
				GG82563_PHY_KMRN_MODE_CTRL, &phy_data);
		if (ret_val)
			return ret_val;

		phy_data &= ~GG82563_KMCR_PASS_FALSE_CARRIER;
		ret_val = e1000_write_phy_reg(hw,
				GG82563_PHY_KMRN_MODE_CTRL, phy_data);

		if (ret_val)
			return ret_val;
	}

	/* Workaround: Disable padding in Kumeran interface in the MAC
	 * and in the PHY to avoid CRC errors.
	 */
	ret_val = e1000_read_phy_reg(hw, GG82563_PHY_INBAND_CTRL, &phy_data);
	if (ret_val)
		return ret_val;
	phy_data |= GG82563_ICR_DIS_PADDING;
	ret_val = e1000_write_phy_reg(hw, GG82563_PHY_INBAND_CTRL, phy_data);
	if (ret_val)
		return ret_val;

	return E1000_SUCCESS;
}

/********************************************************************
* Copper link setup for e1000_phy_m88 series.
*
* hw - Struct containing variables accessed by shared code
*********************************************************************/
static int32_t e1000_copper_link_mgp_setup(struct e1000_hw *hw)
{
	int32_t ret_val;
	uint16_t phy_data;

	DEBUGFUNC();

	/* Enable CRS on TX. This must be set for half-duplex operation. */
	ret_val = e1000_read_phy_reg(hw, M88E1000_PHY_SPEC_CTRL, &phy_data);
	if (ret_val)
		return ret_val;

	phy_data |= M88E1000_PSCR_ASSERT_CRS_ON_TX;
	phy_data |= M88E1000_PSCR_AUTO_X_MODE;
	phy_data &= ~M88E1000_PSCR_POLARITY_REVERSAL;

	ret_val = e1000_write_phy_reg(hw, M88E1000_PHY_SPEC_CTRL, phy_data);
	if (ret_val)
		return ret_val;

	if (hw->phy_revision < M88E1011_I_REV_4) {
		/* Force TX_CLK in the Extended PHY Specific Control Register
		 * to 25MHz clock.
		 */
		ret_val = e1000_read_phy_reg(hw,
				M88E1000_EXT_PHY_SPEC_CTRL, &phy_data);
		if (ret_val)
			return ret_val;

		phy_data |= M88E1000_EPSCR_TX_CLK_25;

		if ((hw->phy_revision == E1000_REVISION_2) &&
			(hw->phy_id == M88E1111_I_PHY_ID)) {
			/* Vidalia Phy, set the downshift counter to 5x */
			phy_data &= ~(M88EC018_EPSCR_DOWNSHIFT_COUNTER_MASK);
			phy_data |= M88EC018_EPSCR_DOWNSHIFT_COUNTER_5X;
			ret_val = e1000_write_phy_reg(hw,
					M88E1000_EXT_PHY_SPEC_CTRL, phy_data);
			if (ret_val)
				return ret_val;
		} else {
			/* Configure Master and Slave downshift values */
			phy_data &= ~(M88E1000_EPSCR_MASTER_DOWNSHIFT_MASK
					| M88E1000_EPSCR_SLAVE_DOWNSHIFT_MASK);
			phy_data |= (M88E1000_EPSCR_MASTER_DOWNSHIFT_1X
					| M88E1000_EPSCR_SLAVE_DOWNSHIFT_1X);
			ret_val = e1000_write_phy_reg(hw,
					M88E1000_EXT_PHY_SPEC_CTRL, phy_data);
			if (ret_val)
				return ret_val;
		}
	}

	/* SW Reset the PHY so all changes take effect */
	ret_val = e1000_phy_reset(hw);
	if (ret_val) {
		dev_err(hw->dev, "Error Resetting the PHY\n");
		return ret_val;
	}

	return E1000_SUCCESS;
}

/********************************************************************
* Setup auto-negotiation and flow control advertisements,
* and then perform auto-negotiation.
*
* hw - Struct containing variables accessed by shared code
*********************************************************************/
static int32_t e1000_copper_link_autoneg(struct e1000_hw *hw)
{
	int32_t ret_val;
	uint16_t phy_data;

	DEBUGFUNC();

	hw->autoneg_advertised = AUTONEG_ADVERTISE_SPEED_DEFAULT;

	/* IFE phy only supports 10/100 */
	if (hw->phy_type == e1000_phy_ife)
		hw->autoneg_advertised &= AUTONEG_ADVERTISE_10_100_ALL;

	dev_dbg(hw->dev, "Reconfiguring auto-neg advertisement params\n");
	ret_val = e1000_phy_setup_autoneg(hw);
	if (ret_val) {
		dev_err(hw->dev, "Error Setting up Auto-Negotiation\n");
		return ret_val;
	}
	dev_dbg(hw->dev, "Restarting Auto-Neg\n");

	/* Restart auto-negotiation by setting the Auto Neg Enable bit and
	 * the Auto Neg Restart bit in the PHY control register.
	 */
	ret_val = e1000_read_phy_reg(hw, PHY_CTRL, &phy_data);
	if (ret_val)
		return ret_val;

	phy_data |= (MII_CR_AUTO_NEG_EN | MII_CR_RESTART_AUTO_NEG);
	ret_val = e1000_write_phy_reg(hw, PHY_CTRL, phy_data);
	if (ret_val)
		return ret_val;

	ret_val = e1000_wait_autoneg(hw);
	if (ret_val) {
		dev_err(hw->dev, "Error while waiting for autoneg to complete\n");
		return ret_val;
	}

	return E1000_SUCCESS;
}

/******************************************************************************
* Config the MAC and the PHY after link is up.
*   1) Set up the MAC to the current PHY speed/duplex
*      if we are on 82543.  If we
*      are on newer silicon, we only need to configure
*      collision distance in the Transmit Control Register.
*   2) Set up flow control on the MAC to that established with
*      the link partner.
*   3) Config DSP to improve Gigabit link quality for some PHY revisions.
*
* hw - Struct containing variables accessed by shared code
******************************************************************************/
static int32_t e1000_copper_link_postconfig(struct e1000_hw *hw)
{
	int32_t ret_val;
	DEBUGFUNC();

	if (hw->mac_type >= e1000_82544) {
		e1000_config_collision_dist(hw);
	} else {
		ret_val = e1000_config_mac_to_phy(hw);
		if (ret_val) {
			dev_err(hw->dev, "Error configuring MAC to PHY settings\n");
			return ret_val;
		}
	}

	ret_val = e1000_config_fc_after_link_up(hw);
	if (ret_val) {
		dev_err(hw->dev, "Error Configuring Flow Control\n");
		return ret_val;
	}

	return E1000_SUCCESS;
}

/******************************************************************************
* Detects which PHY is present and setup the speed and duplex
*
* hw - Struct containing variables accessed by shared code
******************************************************************************/
static int e1000_setup_copper_link(struct e1000_hw *hw)
{
	int32_t ret_val;
	uint16_t i;
	uint16_t phy_data;
	uint16_t reg_data;

	DEBUGFUNC();

	switch (hw->mac_type) {
	case e1000_80003es2lan:
	case e1000_ich8lan:
		/* Set the mac to wait the maximum time between each
		 * iteration and increase the max iterations when
		 * polling the phy; this fixes erroneous timeouts at 10Mbps. */
		ret_val = e1000_write_kmrn_reg(hw, GG82563_REG(0x34, 4), 0xFFFF);
		if (ret_val)
			return ret_val;

		ret_val = e1000_read_kmrn_reg(hw, GG82563_REG(0x34, 9), &reg_data);
		if (ret_val)
			return ret_val;

		reg_data |= 0x3F;

		ret_val = e1000_write_kmrn_reg(hw, GG82563_REG(0x34, 9), reg_data);
		if (ret_val)
			return ret_val;
	default:
		break;
	}

	/* Check if it is a valid PHY and set PHY mode if necessary. */
	ret_val = e1000_copper_link_preconfig(hw);
	if (ret_val)
		return ret_val;

	switch (hw->mac_type) {
	case e1000_80003es2lan:
		/* Kumeran registers are written-only */
		reg_data = E1000_KUMCTRLSTA_INB_CTRL_LINK_STATUS_TX_TIMEOUT_DEFAULT;
		reg_data |= E1000_KUMCTRLSTA_INB_CTRL_DIS_PADDING;
		ret_val = e1000_write_kmrn_reg(hw, E1000_KUMCTRLSTA_OFFSET_INB_CTRL,
				reg_data);
		if (ret_val)
			return ret_val;
		break;
	default:
		break;
	}

	if (hw->phy_type == e1000_phy_igp || hw->phy_type == e1000_phy_igp_3 ||
	    hw->phy_type == e1000_phy_igp_2) {
		ret_val = e1000_copper_link_igp_setup(hw);
		if (ret_val)
			return ret_val;
	} else if (hw->phy_type == e1000_phy_m88 || hw->phy_type == e1000_phy_igb) {
		ret_val = e1000_copper_link_mgp_setup(hw);
		if (ret_val)
			return ret_val;
	} else if (hw->phy_type == e1000_phy_gg82563) {
		ret_val = e1000_copper_link_ggp_setup(hw);
		if (ret_val)
			return ret_val;
	}

	ret_val = e1000_copper_link_autoneg(hw);
	if (ret_val)
		return ret_val;

	/* Check link status. Wait up to 100 microseconds for link to become
	 * valid.
	 */
	for (i = 0; i < 10; i++) {
		ret_val = e1000_read_phy_reg(hw, PHY_STATUS, &phy_data);
		if (ret_val)
			return ret_val;
		ret_val = e1000_read_phy_reg(hw, PHY_STATUS, &phy_data);
		if (ret_val)
			return ret_val;

		if (phy_data & MII_SR_LINK_STATUS) {
			/* Config the MAC and PHY after link is up */
			ret_val = e1000_copper_link_postconfig(hw);
			if (ret_val)
				return ret_val;

			dev_dbg(hw->dev, "Valid link established!!!\n");
			return E1000_SUCCESS;
		}
		udelay(10);
	}

	dev_dbg(hw->dev, "Unable to establish link!!!\n");
	return E1000_SUCCESS;
}

/******************************************************************************
* Configures PHY autoneg and flow control advertisement settings
*
* hw - Struct containing variables accessed by shared code
******************************************************************************/
static int32_t e1000_phy_setup_autoneg(struct e1000_hw *hw)
{
	int32_t ret_val;
	uint16_t mii_autoneg_adv_reg;
	uint16_t mii_1000t_ctrl_reg;

	DEBUGFUNC();

	/* Read the MII Auto-Neg Advertisement Register (Address 4). */
	ret_val = e1000_read_phy_reg(hw, PHY_AUTONEG_ADV, &mii_autoneg_adv_reg);
	if (ret_val)
		return ret_val;

	if (hw->phy_type != e1000_phy_ife) {
		/* Read the MII 1000Base-T Control Register (Address 9). */
		ret_val = e1000_read_phy_reg(hw, PHY_1000T_CTRL,
				&mii_1000t_ctrl_reg);
		if (ret_val)
			return ret_val;
	} else
		mii_1000t_ctrl_reg = 0;

	/* Need to parse both autoneg_advertised and fc and set up
	 * the appropriate PHY registers.  First we will parse for
	 * autoneg_advertised software override.  Since we can advertise
	 * a plethora of combinations, we need to check each bit
	 * individually.
	 */

	/* First we clear all the 10/100 mb speed bits in the Auto-Neg
	 * Advertisement Register (Address 4) and the 1000 mb speed bits in
	 * the  1000Base-T Control Register (Address 9).
	 */
	mii_autoneg_adv_reg &= ~REG4_SPEED_MASK;
	mii_1000t_ctrl_reg &= ~REG9_SPEED_MASK;

	dev_dbg(hw->dev, "autoneg_advertised %x\n", hw->autoneg_advertised);

	/* Do we want to advertise 10 Mb Half Duplex? */
	if (hw->autoneg_advertised & ADVERTISE_10_HALF) {
		dev_dbg(hw->dev, "Advertise 10mb Half duplex\n");
		mii_autoneg_adv_reg |= NWAY_AR_10T_HD_CAPS;
	}

	/* Do we want to advertise 10 Mb Full Duplex? */
	if (hw->autoneg_advertised & ADVERTISE_10_FULL) {
		dev_dbg(hw->dev, "Advertise 10mb Full duplex\n");
		mii_autoneg_adv_reg |= NWAY_AR_10T_FD_CAPS;
	}

	/* Do we want to advertise 100 Mb Half Duplex? */
	if (hw->autoneg_advertised & ADVERTISE_100_HALF) {
		dev_dbg(hw->dev, "Advertise 100mb Half duplex\n");
		mii_autoneg_adv_reg |= NWAY_AR_100TX_HD_CAPS;
	}

	/* Do we want to advertise 100 Mb Full Duplex? */
	if (hw->autoneg_advertised & ADVERTISE_100_FULL) {
		dev_dbg(hw->dev, "Advertise 100mb Full duplex\n");
		mii_autoneg_adv_reg |= NWAY_AR_100TX_FD_CAPS;
	}

	/* We do not allow the Phy to advertise 1000 Mb Half Duplex */
	if (hw->autoneg_advertised & ADVERTISE_1000_HALF) {
		pr_debug
		    ("Advertise 1000mb Half duplex requested, request denied!\n");
	}

	/* Do we want to advertise 1000 Mb Full Duplex? */
	if (hw->autoneg_advertised & ADVERTISE_1000_FULL) {
		dev_dbg(hw->dev, "Advertise 1000mb Full duplex\n");
		mii_1000t_ctrl_reg |= CR_1000T_FD_CAPS;
	}

	/* Check for a software override of the flow control settings, and
	 * setup the PHY advertisement registers accordingly.  If
	 * auto-negotiation is enabled, then software will have to set the
	 * "PAUSE" bits to the correct value in the Auto-Negotiation
	 * Advertisement Register (PHY_AUTONEG_ADV) and re-start auto-negotiation.
	 *
	 * The possible values of the "fc" parameter are:
	 *	0:  Flow control is completely disabled
	 *	1:  Rx flow control is enabled (we can receive pause frames
	 *	    but not send pause frames).
	 *	2:  Tx flow control is enabled (we can send pause frames
	 *	    but we do not support receiving pause frames).
	 *	3:  Both Rx and TX flow control (symmetric) are enabled.
	 *  other:  No software override.  The flow control configuration
	 *	    in the EEPROM is used.
	 */
	switch (hw->fc) {
	case e1000_fc_none:	/* 0 */
		/* Flow control (RX & TX) is completely disabled by a
		 * software over-ride.
		 */
		mii_autoneg_adv_reg &= ~(NWAY_AR_ASM_DIR | NWAY_AR_PAUSE);
		break;
	case e1000_fc_rx_pause:	/* 1 */
		/* RX Flow control is enabled, and TX Flow control is
		 * disabled, by a software over-ride.
		 */
		/* Since there really isn't a way to advertise that we are
		 * capable of RX Pause ONLY, we will advertise that we
		 * support both symmetric and asymmetric RX PAUSE.  Later
		 * (in e1000_config_fc_after_link_up) we will disable the
		 *hw's ability to send PAUSE frames.
		 */
		mii_autoneg_adv_reg |= (NWAY_AR_ASM_DIR | NWAY_AR_PAUSE);
		break;
	case e1000_fc_tx_pause:	/* 2 */
		/* TX Flow control is enabled, and RX Flow control is
		 * disabled, by a software over-ride.
		 */
		mii_autoneg_adv_reg |= NWAY_AR_ASM_DIR;
		mii_autoneg_adv_reg &= ~NWAY_AR_PAUSE;
		break;
	case e1000_fc_full:	/* 3 */
		/* Flow control (both RX and TX) is enabled by a software
		 * over-ride.
		 */
		mii_autoneg_adv_reg |= (NWAY_AR_ASM_DIR | NWAY_AR_PAUSE);
		break;
	default:
		dev_dbg(hw->dev, "Flow control param set incorrectly\n");
		return -E1000_ERR_CONFIG;
	}

	ret_val = e1000_write_phy_reg(hw, PHY_AUTONEG_ADV, mii_autoneg_adv_reg);
	if (ret_val)
		return ret_val;

	dev_dbg(hw->dev, "Auto-Neg Advertising %x\n", mii_autoneg_adv_reg);

	if (hw->phy_type != e1000_phy_ife) {
		ret_val = e1000_write_phy_reg(hw, PHY_1000T_CTRL,
				mii_1000t_ctrl_reg);
		if (ret_val)
			return ret_val;
	}

	return E1000_SUCCESS;
}

/******************************************************************************
* Sets the collision distance in the Transmit Control register
*
* hw - Struct containing variables accessed by shared code
*
* Link should have been established previously. Reads the speed and duplex
* information from the Device Status register.
******************************************************************************/
static void e1000_config_collision_dist(struct e1000_hw *hw)
{
	uint32_t tctl, coll_dist;

	DEBUGFUNC();

	if (hw->mac_type < e1000_82543)
		coll_dist = E1000_COLLISION_DISTANCE_82542;
	else
		coll_dist = E1000_COLLISION_DISTANCE;

	tctl = e1000_read_reg(hw, E1000_TCTL);

	tctl &= ~E1000_TCTL_COLD;
	tctl |= coll_dist << E1000_COLD_SHIFT;

	e1000_write_reg(hw, E1000_TCTL, tctl);
	e1000_write_flush(hw);
}

/******************************************************************************
* Sets MAC speed and duplex settings to reflect the those in the PHY
*
* hw - Struct containing variables accessed by shared code
* mii_reg - data to write to the MII control register
*
* The contents of the PHY register containing the needed information need to
* be passed in.
******************************************************************************/
static int e1000_config_mac_to_phy(struct e1000_hw *hw)
{
	uint32_t ctrl;
	uint16_t phy_data;

	DEBUGFUNC();

	/* Read the Device Control Register and set the bits to Force Speed
	 * and Duplex.
	 */
	ctrl = e1000_read_reg(hw, E1000_CTRL);
	ctrl |= (E1000_CTRL_FRCSPD | E1000_CTRL_FRCDPX);
	ctrl &= ~(E1000_CTRL_ILOS);
	ctrl |= (E1000_CTRL_SPD_SEL);

	/* Set up duplex in the Device Control and Transmit Control
	 * registers depending on negotiated values.
	 */
	if (e1000_read_phy_reg(hw, M88E1000_PHY_SPEC_STATUS, &phy_data) < 0) {
		dev_err(hw->dev, "PHY Read Error\n");
		return -E1000_ERR_PHY;
	}
	if (phy_data & M88E1000_PSSR_DPLX)
		ctrl |= E1000_CTRL_FD;
	else
		ctrl &= ~E1000_CTRL_FD;

	e1000_config_collision_dist(hw);

	/* Set up speed in the Device Control register depending on
	 * negotiated values.
	 */
	if ((phy_data & M88E1000_PSSR_SPEED) == M88E1000_PSSR_1000MBS)
		ctrl |= E1000_CTRL_SPD_1000;
	else if ((phy_data & M88E1000_PSSR_SPEED) == M88E1000_PSSR_100MBS)
		ctrl |= E1000_CTRL_SPD_100;
	/* Write the configured values back to the Device Control Reg. */
	e1000_write_reg(hw, E1000_CTRL, ctrl);
	return 0;
}

/******************************************************************************
 * Forces the MAC's flow control settings.
 *
 * hw - Struct containing variables accessed by shared code
 *
 * Sets the TFCE and RFCE bits in the device control register to reflect
 * the adapter settings. TFCE and RFCE need to be explicitly set by
 * software when a Copper PHY is used because autonegotiation is managed
 * by the PHY rather than the MAC. Software must also configure these
 * bits when link is forced on a fiber connection.
 *****************************************************************************/
static int e1000_force_mac_fc(struct e1000_hw *hw)
{
	uint32_t ctrl;

	DEBUGFUNC();

	/* Get the current configuration of the Device Control Register */
	ctrl = e1000_read_reg(hw, E1000_CTRL);

	/* Because we didn't get link via the internal auto-negotiation
	 * mechanism (we either forced link or we got link via PHY
	 * auto-neg), we have to manually enable/disable transmit an
	 * receive flow control.
	 *
	 * The "Case" statement below enables/disable flow control
	 * according to the "hw->fc" parameter.
	 *
	 * The possible values of the "fc" parameter are:
	 *	0:  Flow control is completely disabled
	 *	1:  Rx flow control is enabled (we can receive pause
	 *	    frames but not send pause frames).
	 *	2:  Tx flow control is enabled (we can send pause frames
	 *	    frames but we do not receive pause frames).
	 *	3:  Both Rx and TX flow control (symmetric) is enabled.
	 *  other:  No other values should be possible at this point.
	 */

	switch (hw->fc) {
	case e1000_fc_none:
		ctrl &= (~(E1000_CTRL_TFCE | E1000_CTRL_RFCE));
		break;
	case e1000_fc_rx_pause:
		ctrl &= (~E1000_CTRL_TFCE);
		ctrl |= E1000_CTRL_RFCE;
		break;
	case e1000_fc_tx_pause:
		ctrl &= (~E1000_CTRL_RFCE);
		ctrl |= E1000_CTRL_TFCE;
		break;
	case e1000_fc_full:
		ctrl |= (E1000_CTRL_TFCE | E1000_CTRL_RFCE);
		break;
	default:
		dev_err(hw->dev, "Flow control param set incorrectly\n");
		return -E1000_ERR_CONFIG;
	}

	/* Disable TX Flow Control for 82542 (rev 2.0) */
	if (hw->mac_type == e1000_82542_rev2_0)
		ctrl &= (~E1000_CTRL_TFCE);

	e1000_write_reg(hw, E1000_CTRL, ctrl);
	return 0;
}

/******************************************************************************
 * Configures flow control settings after link is established
 *
 * hw - Struct containing variables accessed by shared code
 *
 * Should be called immediately after a valid link has been established.
 * Forces MAC flow control settings if link was forced. When in MII/GMII mode
 * and autonegotiation is enabled, the MAC flow control settings will be set
 * based on the flow control negotiated by the PHY. In TBI mode, the TFCE
 * and RFCE bits will be automaticaly set to the negotiated flow control mode.
 *****************************************************************************/
static int32_t e1000_config_fc_after_link_up(struct e1000_hw *hw)
{
	int32_t ret_val;
	uint16_t mii_status_reg;
	uint16_t mii_nway_adv_reg;
	uint16_t mii_nway_lp_ability_reg;
	uint16_t speed;
	uint16_t duplex;

	DEBUGFUNC();

	/* Read the MII Status Register and check to see if AutoNeg
	 * has completed.  We read this twice because this reg has
	 * some "sticky" (latched) bits.
	 */
	if (e1000_read_phy_reg(hw, PHY_STATUS, &mii_status_reg) < 0) {
		dev_err(hw->dev, "PHY Read Error \n");
		return -E1000_ERR_PHY;
	}

	if (e1000_read_phy_reg(hw, PHY_STATUS, &mii_status_reg) < 0) {
		dev_err(hw->dev, "PHY Read Error \n");
		return -E1000_ERR_PHY;
	}

	if (!(mii_status_reg & MII_SR_AUTONEG_COMPLETE)) {
		dev_err(hw->dev, "Copper PHY and Auto Neg has not completed.\n");
		return 0;
	}

	/* The AutoNeg process has completed, so we now need to
	 * read both the Auto Negotiation Advertisement Register
	 * (Address 4) and the Auto_Negotiation Base Page Ability
	 * Register (Address 5) to determine how flow control was
	 * negotiated.
	 */
	if (e1000_read_phy_reg(hw, PHY_AUTONEG_ADV, &mii_nway_adv_reg) < 0) {
		dev_err(hw->dev, "PHY Read Error\n");
		return -E1000_ERR_PHY;
	}

	if (e1000_read_phy_reg(hw, PHY_LP_ABILITY, &mii_nway_lp_ability_reg) < 0) {
		dev_err(hw->dev, "PHY Read Error\n");
		return -E1000_ERR_PHY;
	}

	/* Two bits in the Auto Negotiation Advertisement Register
	 * (Address 4) and two bits in the Auto Negotiation Base
	 * Page Ability Register (Address 5) determine flow control
	 * for both the PHY and the link partner.  The following
	 * table, taken out of the IEEE 802.3ab/D6.0 dated March 25,
	 * 1999, describes these PAUSE resolution bits and how flow
	 * control is determined based upon these settings.
	 * NOTE:  DC = Don't Care
	 *
	 *   LOCAL DEVICE  |   LINK PARTNER
	 * PAUSE | ASM_DIR | PAUSE | ASM_DIR | NIC Resolution
	 *-------|---------|-------|---------|--------------------
	 *   0	 |    0    |  DC   |   DC    | e1000_fc_none
	 *   0	 |    1    |   0   |   DC    | e1000_fc_none
	 *   0	 |    1    |   1   |	0    | e1000_fc_none
	 *   0	 |    1    |   1   |	1    | e1000_fc_tx_pause
	 *   1	 |    0    |   0   |   DC    | e1000_fc_none
	 *   1	 |   DC    |   1   |   DC    | e1000_fc_full
	 *   1	 |    1    |   0   |	0    | e1000_fc_none
	 *   1	 |    1    |   0   |	1    | e1000_fc_rx_pause
	 *
	 */
	/* Are both PAUSE bits set to 1?  If so, this implies
	 * Symmetric Flow Control is enabled at both ends.  The
	 * ASM_DIR bits are irrelevant per the spec.
	 *
	 * For Symmetric Flow Control:
	 *
	 *   LOCAL DEVICE  |   LINK PARTNER
	 * PAUSE | ASM_DIR | PAUSE | ASM_DIR | Result
	 *-------|---------|-------|---------|--------------------
	 *   1	 |   DC    |   1   |   DC    | e1000_fc_full
	 *
	 */
	if ((mii_nway_adv_reg & NWAY_AR_PAUSE) &&
	    (mii_nway_lp_ability_reg & NWAY_LPAR_PAUSE)) {
		/* Now we need to check if the user selected RX ONLY
		 * of pause frames.  In this case, we had to advertise
		 * FULL flow control because we could not advertise RX
		 * ONLY. Hence, we must now check to see if we need to
		 * turn OFF  the TRANSMISSION of PAUSE frames.
		 */
		if (hw->original_fc == e1000_fc_full) {
			hw->fc = e1000_fc_full;
			dev_dbg(hw->dev, "Flow Control = FULL.\r\n");
		} else {
			hw->fc = e1000_fc_rx_pause;
			dev_dbg(hw->dev, "Flow Control = RX PAUSE frames only.\r\n");
		}
	}
	/* For receiving PAUSE frames ONLY.
	 *
	 *   LOCAL DEVICE  |   LINK PARTNER
	 * PAUSE | ASM_DIR | PAUSE | ASM_DIR | Result
	 *-------|---------|-------|---------|--------------------
	 *   0	 |    1    |   1   |	1    | e1000_fc_tx_pause
	 *
	 */
	else if (!(mii_nway_adv_reg & NWAY_AR_PAUSE) &&
		 (mii_nway_adv_reg & NWAY_AR_ASM_DIR) &&
		 (mii_nway_lp_ability_reg & NWAY_LPAR_PAUSE) &&
		 (mii_nway_lp_ability_reg & NWAY_LPAR_ASM_DIR))
	{
		hw->fc = e1000_fc_tx_pause;
		dev_dbg(hw->dev, "Flow Control = TX PAUSE frames only.\r\n");
	}
	/* For transmitting PAUSE frames ONLY.
	 *
	 *   LOCAL DEVICE  |   LINK PARTNER
	 * PAUSE | ASM_DIR | PAUSE | ASM_DIR | Result
	 *-------|---------|-------|---------|--------------------
	 *   1	 |    1    |   0   |	1    | e1000_fc_rx_pause
	 *
	 */
	else if ((mii_nway_adv_reg & NWAY_AR_PAUSE) &&
		 (mii_nway_adv_reg & NWAY_AR_ASM_DIR) &&
		 !(mii_nway_lp_ability_reg & NWAY_LPAR_PAUSE) &&
		 (mii_nway_lp_ability_reg & NWAY_LPAR_ASM_DIR))
	{
		hw->fc = e1000_fc_rx_pause;
		dev_dbg(hw->dev, "Flow Control = RX PAUSE frames only.\r\n");
	}
	/* Per the IEEE spec, at this point flow control should be
	 * disabled.  However, we want to consider that we could
	 * be connected to a legacy switch that doesn't advertise
	 * desired flow control, but can be forced on the link
	 * partner.  So if we advertised no flow control, that is
	 * what we will resolve to.  If we advertised some kind of
	 * receive capability (Rx Pause Only or Full Flow Control)
	 * and the link partner advertised none, we will configure
	 * ourselves to enable Rx Flow Control only.  We can do
	 * this safely for two reasons:  If the link partner really
	 * didn't want flow control enabled, and we enable Rx, no
	 * harm done since we won't be receiving any PAUSE frames
	 * anyway.  If the intent on the link partner was to have
	 * flow control enabled, then by us enabling RX only, we
	 * can at least receive pause frames and process them.
	 * This is a good idea because in most cases, since we are
	 * predominantly a server NIC, more times than not we will
	 * be asked to delay transmission of packets than asking
	 * our link partner to pause transmission of frames.
	 */
	else if (hw->original_fc == e1000_fc_none ||
		 hw->original_fc == e1000_fc_tx_pause) {
		hw->fc = e1000_fc_none;
		dev_dbg(hw->dev, "Flow Control = NONE.\r\n");
	} else {
		hw->fc = e1000_fc_rx_pause;
		dev_dbg(hw->dev, "Flow Control = RX PAUSE frames only.\r\n");
	}
	/* Now we need to do one last check...	If we auto-
	 * negotiated to HALF DUPLEX, flow control should not be
	 * enabled per IEEE 802.3 spec.
	 */
	e1000_get_speed_and_duplex(hw, &speed, &duplex);
	if (duplex == HALF_DUPLEX)
		hw->fc = e1000_fc_none;
	/* Now we call a subroutine to actually force the MAC
	 * controller to use the correct flow control settings.
	 */
	ret_val = e1000_force_mac_fc(hw);
	if (ret_val < 0) {
		dev_err(hw->dev, "Error forcing flow control settings\n");
		return ret_val;
	}

	return E1000_SUCCESS;
}

/******************************************************************************
* Configure the MAC-to-PHY interface for 10/100Mbps
*
* hw - Struct containing variables accessed by shared code
******************************************************************************/
static int32_t e1000_configure_kmrn_for_10_100(struct e1000_hw *hw, uint16_t duplex)
{
	int32_t ret_val = E1000_SUCCESS;
	uint32_t tipg;
	uint16_t reg_data;

	DEBUGFUNC();

	reg_data = E1000_KUMCTRLSTA_HD_CTRL_10_100_DEFAULT;
	ret_val = e1000_write_kmrn_reg(hw,
			E1000_KUMCTRLSTA_OFFSET_HD_CTRL, reg_data);
	if (ret_val)
		return ret_val;

	/* Configure Transmit Inter-Packet Gap */
	tipg = e1000_read_reg(hw, E1000_TIPG);
	tipg &= ~E1000_TIPG_IPGT_MASK;
	tipg |= DEFAULT_80003ES2LAN_TIPG_IPGT_10_100;
	e1000_write_reg(hw, E1000_TIPG, tipg);

	ret_val = e1000_read_phy_reg(hw, GG82563_PHY_KMRN_MODE_CTRL, &reg_data);

	if (ret_val)
		return ret_val;

	if (duplex == HALF_DUPLEX)
		reg_data |= GG82563_KMCR_PASS_FALSE_CARRIER;
	else
		reg_data &= ~GG82563_KMCR_PASS_FALSE_CARRIER;

	ret_val = e1000_write_phy_reg(hw, GG82563_PHY_KMRN_MODE_CTRL, reg_data);

	return ret_val;
}

static int32_t e1000_configure_kmrn_for_1000(struct e1000_hw *hw)
{
	int32_t ret_val = E1000_SUCCESS;
	uint16_t reg_data;
	uint32_t tipg;

	DEBUGFUNC();

	reg_data = E1000_KUMCTRLSTA_HD_CTRL_1000_DEFAULT;
	ret_val = e1000_write_kmrn_reg(hw,
			E1000_KUMCTRLSTA_OFFSET_HD_CTRL, reg_data);
	if (ret_val)
		return ret_val;

	/* Configure Transmit Inter-Packet Gap */
	tipg = e1000_read_reg(hw, E1000_TIPG);
	tipg &= ~E1000_TIPG_IPGT_MASK;
	tipg |= DEFAULT_80003ES2LAN_TIPG_IPGT_1000;
	e1000_write_reg(hw, E1000_TIPG, tipg);

	ret_val = e1000_read_phy_reg(hw, GG82563_PHY_KMRN_MODE_CTRL, &reg_data);

	if (ret_val)
		return ret_val;

	reg_data &= ~GG82563_KMCR_PASS_FALSE_CARRIER;
	ret_val = e1000_write_phy_reg(hw, GG82563_PHY_KMRN_MODE_CTRL, reg_data);

	return ret_val;
}

/******************************************************************************
 * Detects the current speed and duplex settings of the hardware.
 *
 * hw - Struct containing variables accessed by shared code
 * speed - Speed of the connection
 * duplex - Duplex setting of the connection
 *****************************************************************************/
static int e1000_get_speed_and_duplex(struct e1000_hw *hw, uint16_t *speed,
		uint16_t *duplex)
{
	uint32_t status;
	int32_t ret_val;

	DEBUGFUNC();

	if (hw->mac_type >= e1000_82543) {
		status = e1000_read_reg(hw, E1000_STATUS);
		if (status & E1000_STATUS_SPEED_1000) {
			*speed = SPEED_1000;
			dev_dbg(hw->dev, "1000 Mbs, ");
		} else if (status & E1000_STATUS_SPEED_100) {
			*speed = SPEED_100;
			dev_dbg(hw->dev, "100 Mbs, ");
		} else {
			*speed = SPEED_10;
			dev_dbg(hw->dev, "10 Mbs, ");
		}

		if (status & E1000_STATUS_FD) {
			*duplex = FULL_DUPLEX;
			dev_dbg(hw->dev, "Full Duplex\r\n");
		} else {
			*duplex = HALF_DUPLEX;
			dev_dbg(hw->dev, " Half Duplex\r\n");
		}
	} else {
		dev_dbg(hw->dev, "1000 Mbs, Full Duplex\r\n");
		*speed = SPEED_1000;
		*duplex = FULL_DUPLEX;
	}

	if ((hw->mac_type == e1000_80003es2lan) && e1000_media_copper(hw)) {
		if (*speed == SPEED_1000)
			ret_val = e1000_configure_kmrn_for_1000(hw);
		else
			ret_val = e1000_configure_kmrn_for_10_100(hw, *duplex);
		if (ret_val)
			return ret_val;
	}
	return E1000_SUCCESS;
}

/******************************************************************************
* Blocks until autoneg completes or times out (~4.5 seconds)
*
* hw - Struct containing variables accessed by shared code
******************************************************************************/
static int e1000_wait_autoneg(struct e1000_hw *hw)
{
	uint16_t i;
	uint16_t phy_data;

	DEBUGFUNC();
	dev_dbg(hw->dev, "Waiting for Auto-Neg to complete.\n");

	/* We will wait for autoneg to complete or 4.5 seconds to expire. */
	for (i = PHY_AUTO_NEG_TIME; i > 0; i--) {
		/* Read the MII Status Register and wait for Auto-Neg
		 * Complete bit to be set.
		 */
		if (e1000_read_phy_reg(hw, PHY_STATUS, &phy_data) < 0) {
			dev_err(hw->dev, "PHY Read Error\n");
			return -E1000_ERR_PHY;
		}
		if (e1000_read_phy_reg(hw, PHY_STATUS, &phy_data) < 0) {
			dev_err(hw->dev, "PHY Read Error\n");
			return -E1000_ERR_PHY;
		}
		if (phy_data & MII_SR_AUTONEG_COMPLETE) {
			dev_dbg(hw->dev, "Auto-Neg complete.\n");
			return 0;
		}
		mdelay(100);
	}
	dev_err(hw->dev, "Auto-Neg timedout.\n");
	return -E1000_ERR_TIMEOUT;
}

/******************************************************************************
* Raises the Management Data Clock
*
* hw - Struct containing variables accessed by shared code
* ctrl - Device control register's current value
******************************************************************************/
static void e1000_raise_mdi_clk(struct e1000_hw *hw, uint32_t * ctrl)
{
	/* Raise the clock input to the Management Data Clock (by setting the MDC
	 * bit), and then delay 2 microseconds.
	 */
	e1000_write_reg(hw, E1000_CTRL, (*ctrl | E1000_CTRL_MDC));
	e1000_write_flush(hw);
	udelay(2);
}

/******************************************************************************
* Lowers the Management Data Clock
*
* hw - Struct containing variables accessed by shared code
* ctrl - Device control register's current value
******************************************************************************/
static void e1000_lower_mdi_clk(struct e1000_hw *hw, uint32_t * ctrl)
{
	/* Lower the clock input to the Management Data Clock (by clearing the MDC
	 * bit), and then delay 2 microseconds.
	 */
	e1000_write_reg(hw, E1000_CTRL, (*ctrl & ~E1000_CTRL_MDC));
	e1000_write_flush(hw);
	udelay(2);
}

/******************************************************************************
* Shifts data bits out to the PHY
*
* hw - Struct containing variables accessed by shared code
* data - Data to send out to the PHY
* count - Number of bits to shift out
*
* Bits are shifted out in MSB to LSB order.
******************************************************************************/
static void e1000_shift_out_mdi_bits(struct e1000_hw *hw, uint32_t data,
		uint16_t count)
{
	uint32_t ctrl;
	uint32_t mask;

	/* We need to shift "count" number of bits out to the PHY. So, the value
	 * in the "data" parameter will be shifted out to the PHY one bit at a
	 * time. In order to do this, "data" must be broken down into bits.
	 */
	mask = 0x01;
	mask <<= (count - 1);

	ctrl = e1000_read_reg(hw, E1000_CTRL);

	/* Set MDIO_DIR and MDC_DIR direction bits to be used as output pins. */
	ctrl |= (E1000_CTRL_MDIO_DIR | E1000_CTRL_MDC_DIR);

	while (mask) {
		/* A "1" is shifted out to the PHY by setting the MDIO bit to "1" and
		 * then raising and lowering the Management Data Clock. A "0" is
		 * shifted out to the PHY by setting the MDIO bit to "0" and then
		 * raising and lowering the clock.
		 */
		if (data & mask)
			ctrl |= E1000_CTRL_MDIO;
		else
			ctrl &= ~E1000_CTRL_MDIO;

		e1000_write_reg(hw, E1000_CTRL, ctrl);
		e1000_write_flush(hw);

		udelay(2);

		e1000_raise_mdi_clk(hw, &ctrl);
		e1000_lower_mdi_clk(hw, &ctrl);

		mask = mask >> 1;
	}
}

/******************************************************************************
* Shifts data bits in from the PHY
*
* hw - Struct containing variables accessed by shared code
*
* Bits are shifted in in MSB to LSB order.
******************************************************************************/
static uint16_t e1000_shift_in_mdi_bits(struct e1000_hw *hw)
{
	uint32_t ctrl;
	uint16_t data = 0;
	uint8_t i;

	/* In order to read a register from the PHY, we need to shift in a total
	 * of 18 bits from the PHY. The first two bit (turnaround) times are used
	 * to avoid contention on the MDIO pin when a read operation is performed.
	 * These two bits are ignored by us and thrown away. Bits are "shifted in"
	 * by raising the input to the Management Data Clock (setting the MDC bit),
	 * and then reading the value of the MDIO bit.
	 */
	ctrl = e1000_read_reg(hw, E1000_CTRL);

	/* Clear MDIO_DIR (SWDPIO1) to indicate this bit is to be used as input. */
	ctrl &= ~E1000_CTRL_MDIO_DIR;
	ctrl &= ~E1000_CTRL_MDIO;

	e1000_write_reg(hw, E1000_CTRL, ctrl);
	e1000_write_flush(hw);

	/* Raise and Lower the clock before reading in the data. This accounts for
	 * the turnaround bits. The first clock occurred when we clocked out the
	 * last bit of the Register Address.
	 */
	e1000_raise_mdi_clk(hw, &ctrl);
	e1000_lower_mdi_clk(hw, &ctrl);

	for (data = 0, i = 0; i < 16; i++) {
		data = data << 1;
		e1000_raise_mdi_clk(hw, &ctrl);
		ctrl = e1000_read_reg(hw, E1000_CTRL);
		/* Check to see if we shifted in a "1". */
		if (ctrl & E1000_CTRL_MDIO)
			data |= 1;
		e1000_lower_mdi_clk(hw, &ctrl);
	}

	e1000_raise_mdi_clk(hw, &ctrl);
	e1000_lower_mdi_clk(hw, &ctrl);

	return data;
}

static int e1000_phy_read(struct mii_bus *bus, int phy_addr, int reg_addr)
{
	struct e1000_hw *hw = bus->priv;
	uint32_t i;
	uint32_t mdic = 0;

	if (phy_addr != 1)
		return -EIO;

	if (hw->mac_type > e1000_82543) {
		/* Set up Op-code, Phy Address, and register address in the MDI
		 * Control register.  The MAC will take care of interfacing with the
		 * PHY to retrieve the desired data.
		 */
		mdic = ((reg_addr << E1000_MDIC_REG_SHIFT) |
			(phy_addr << E1000_MDIC_PHY_SHIFT) |
			(E1000_MDIC_OP_READ));

		e1000_write_reg(hw, E1000_MDIC, mdic);

		/* Poll the ready bit to see if the MDI read completed */
		for (i = 0; i < 64; i++) {
			udelay(10);
			mdic = e1000_read_reg(hw, E1000_MDIC);
			if (mdic & E1000_MDIC_READY)
				break;
		}
		if (!(mdic & E1000_MDIC_READY)) {
			dev_err(hw->dev, "MDI Read did not complete\n");
			return -E1000_ERR_PHY;
		}
		if (mdic & E1000_MDIC_ERROR) {
			dev_err(hw->dev, "MDI Error\n");
			return -E1000_ERR_PHY;
		}
		return mdic;
	} else {
		/* We must first send a preamble through the MDIO pin to signal the
		 * beginning of an MII instruction.  This is done by sending 32
		 * consecutive "1" bits.
		 */
		e1000_shift_out_mdi_bits(hw, PHY_PREAMBLE, PHY_PREAMBLE_SIZE);

		/* Now combine the next few fields that are required for a read
		 * operation.  We use this method instead of calling the
		 * e1000_shift_out_mdi_bits routine five different times. The format of
		 * a MII read instruction consists of a shift out of 14 bits and is
		 * defined as follows:
		 *    <Preamble><SOF><Op Code><Phy Addr><Reg Addr>
		 * followed by a shift in of 18 bits.  This first two bits shifted in
		 * are TurnAround bits used to avoid contention on the MDIO pin when a
		 * READ operation is performed.  These two bits are thrown away
		 * followed by a shift in of 16 bits which contains the desired data.
		 */
		mdic = ((reg_addr) | (phy_addr << 5) |
			(PHY_OP_READ << 10) | (PHY_SOF << 12));

		e1000_shift_out_mdi_bits(hw, mdic, 14);

		/* Now that we've shifted out the read command to the MII, we need to
		 * "shift in" the 16-bit value (18 total bits) of the requested PHY
		 * register address.
		 */
		return e1000_shift_in_mdi_bits(hw);
	}
}

/*****************************************************************************
* Reads the value from a PHY register
*
* hw - Struct containing variables accessed by shared code
* reg_addr - address of the PHY register to read
******************************************************************************/
static int e1000_read_phy_reg(struct e1000_hw *hw, uint32_t reg_addr,
		uint16_t *phy_data)
{
	int ret;

	ret = e1000_phy_read(&hw->miibus, 1, reg_addr);
	if (ret < 0)
		return ret;

	*phy_data = ret;

	return 0;
}

static int e1000_phy_write(struct mii_bus *bus, int phy_addr,
	int reg_addr, u16 phy_data)
{
	struct e1000_hw *hw = bus->priv;
	uint32_t i;
	uint32_t mdic = 0;

	if (phy_addr != 1)
		return -EIO;

	if (hw->mac_type > e1000_82543) {
		/* Set up Op-code, Phy Address, register address, and data intended
		 * for the PHY register in the MDI Control register.  The MAC will take
		 * care of interfacing with the PHY to send the desired data.
		 */
		mdic = (((uint32_t) phy_data) |
			(reg_addr << E1000_MDIC_REG_SHIFT) |
			(phy_addr << E1000_MDIC_PHY_SHIFT) |
			(E1000_MDIC_OP_WRITE));

		e1000_write_reg(hw, E1000_MDIC, mdic);

		/* Poll the ready bit to see if the MDI read completed */
		for (i = 0; i < 64; i++) {
			udelay(10);
			mdic = e1000_read_reg(hw, E1000_MDIC);
			if (mdic & E1000_MDIC_READY)
				break;
		}
		if (!(mdic & E1000_MDIC_READY)) {
			dev_err(hw->dev, "MDI Write did not complete\n");
			return -E1000_ERR_PHY;
		}
	} else {
		/* We'll need to use the SW defined pins to shift the write command
		 * out to the PHY. We first send a preamble to the PHY to signal the
		 * beginning of the MII instruction.  This is done by sending 32
		 * consecutive "1" bits.
		 */
		e1000_shift_out_mdi_bits(hw, PHY_PREAMBLE, PHY_PREAMBLE_SIZE);

		/* Now combine the remaining required fields that will indicate a
		 * write operation. We use this method instead of calling the
		 * e1000_shift_out_mdi_bits routine for each field in the command. The
		 * format of a MII write instruction is as follows:
		 * <Preamble><SOF><Op Code><Phy Addr><Reg Addr><Turnaround><Data>.
		 */
		mdic = ((PHY_TURNAROUND) | (reg_addr << 2) | (phy_addr << 7) |
			(PHY_OP_WRITE << 12) | (PHY_SOF << 14));
		mdic <<= 16;
		mdic |= (uint32_t) phy_data;

		e1000_shift_out_mdi_bits(hw, mdic, 32);
	}
	return 0;
}

/******************************************************************************
 * Writes a value to a PHY register
 *
 * hw - Struct containing variables accessed by shared code
 * reg_addr - address of the PHY register to write
 * data - data to write to the PHY
 ******************************************************************************/
static int e1000_write_phy_reg(struct e1000_hw *hw, uint32_t reg_addr, uint16_t phy_data)
{
	return e1000_phy_write(&hw->miibus, 1, reg_addr, phy_data);
}

/******************************************************************************
 * Checks if PHY reset is blocked due to SOL/IDER session, for example.
 * Returning E1000_BLK_PHY_RESET isn't necessarily an error.  But it's up to
 * the caller to figure out how to deal with it.
 *
 * hw - Struct containing variables accessed by shared code
 *
 * returns: - E1000_BLK_PHY_RESET
 *            E1000_SUCCESS
 *
 *****************************************************************************/
static int32_t e1000_check_phy_reset_block(struct e1000_hw *hw)
{
	if (hw->mac_type == e1000_ich8lan) {
		if (e1000_read_reg(hw, E1000_FWSM) & E1000_FWSM_RSPCIPHY)
			return E1000_SUCCESS;
		else
			return E1000_BLK_PHY_RESET;
	}

	if (hw->mac_type > e1000_82547_rev_2) {
		if (e1000_read_reg(hw, E1000_MANC) & E1000_MANC_BLK_PHY_RST_ON_IDE)
			return E1000_BLK_PHY_RESET;
		else
			return E1000_SUCCESS;
	}

	return E1000_SUCCESS;
}

/***************************************************************************
 * Checks if the PHY configuration is done
 *
 * hw: Struct containing variables accessed by shared code
 *
 * returns: - E1000_ERR_RESET if fail to reset MAC
 *            E1000_SUCCESS at any other case.
 *
 ***************************************************************************/
static int32_t e1000_get_phy_cfg_done(struct e1000_hw *hw)
{
	int32_t timeout = PHY_CFG_TIMEOUT;
	uint32_t cfg_mask = E1000_EEPROM_CFG_DONE;

	DEBUGFUNC();

	switch (hw->mac_type) {
	default:
		mdelay(10);
		break;

	case e1000_80003es2lan:
		/* Separate *_CFG_DONE_* bit for each port */
		if (e1000_is_second_port(hw))
			cfg_mask = E1000_EEPROM_CFG_DONE_PORT_1;
		/* Fall Through */

	case e1000_82571:
	case e1000_82572:
	case e1000_igb:
		while (timeout) {
			if (e1000_read_reg(hw, E1000_EEMNGCTL) & cfg_mask)
				break;

			mdelay(1);
			timeout--;
		}
		if (!timeout) {
			dev_err(hw->dev, "MNG configuration cycle has not completed.\n");
			return -E1000_ERR_RESET;
		}
		break;
	}

	return E1000_SUCCESS;
}

/******************************************************************************
* Returns the PHY to the power-on reset state
*
* hw - Struct containing variables accessed by shared code
******************************************************************************/
static int32_t e1000_phy_hw_reset(struct e1000_hw *hw)
{
	uint16_t swfw = E1000_SWFW_PHY0_SM;
	uint32_t ctrl, ctrl_ext;
	uint32_t led_ctrl;
	int32_t ret_val;

	DEBUGFUNC();

	/* In the case of the phy reset being blocked, it's not an error, we
	 * simply return success without performing the reset. */
	ret_val = e1000_check_phy_reset_block(hw);
	if (ret_val)
		return E1000_SUCCESS;

	dev_dbg(hw->dev, "Resetting Phy...\n");

	if (hw->mac_type > e1000_82543) {
		if (e1000_is_second_port(hw))
			swfw = E1000_SWFW_PHY1_SM;

		if (e1000_swfw_sync_acquire(hw, swfw)) {
			dev_err(hw->dev, "Unable to acquire swfw sync\n");
			return -E1000_ERR_SWFW_SYNC;
		}

		/* Read the device control register and assert the E1000_CTRL_PHY_RST
		 * bit. Then, take it out of reset.
		 */
		ctrl = e1000_read_reg(hw, E1000_CTRL);
		e1000_write_reg(hw, E1000_CTRL, ctrl | E1000_CTRL_PHY_RST);
		e1000_write_flush(hw);

		udelay(100);

		e1000_write_reg(hw, E1000_CTRL, ctrl);
		e1000_write_flush(hw);

		if (hw->mac_type >= e1000_82571)
			mdelay(10);

		if (e1000_swfw_sync_release(hw, swfw) < 0)
			dev_warn(hw->dev,
				 "Timeout while releasing SWFW_SYNC bits (0x%08x)\n",
				 swfw);
	} else {
		/* Read the Extended Device Control Register, assert the PHY_RESET_DIR
		 * bit to put the PHY into reset. Then, take it out of reset.
		 */
		ctrl_ext = e1000_read_reg(hw, E1000_CTRL_EXT);
		ctrl_ext |= E1000_CTRL_EXT_SDP4_DIR;
		ctrl_ext &= ~E1000_CTRL_EXT_SDP4_DATA;
		e1000_write_reg(hw, E1000_CTRL_EXT, ctrl_ext);
		e1000_write_flush(hw);
		mdelay(10);
		ctrl_ext |= E1000_CTRL_EXT_SDP4_DATA;
		e1000_write_reg(hw, E1000_CTRL_EXT, ctrl_ext);
		e1000_write_flush(hw);
	}
	udelay(150);

	if ((hw->mac_type == e1000_82541) || (hw->mac_type == e1000_82547)) {
		/* Configure activity LED after PHY reset */
		led_ctrl = e1000_read_reg(hw, E1000_LEDCTL);
		led_ctrl &= IGP_ACTIVITY_LED_MASK;
		led_ctrl |= (IGP_ACTIVITY_LED_ENABLE | IGP_LED3_MODE);
		e1000_write_reg(hw, E1000_LEDCTL, led_ctrl);
	}

	/* Wait for FW to finish PHY configuration. */
	return e1000_get_phy_cfg_done(hw);
}

/******************************************************************************
 * IGP phy init script - initializes the GbE PHY
 *
 * hw - Struct containing variables accessed by shared code
 *****************************************************************************/
static void e1000_phy_init_script(struct e1000_hw *hw)
{
	uint32_t ret_val;
	uint16_t phy_saved_data;

	DEBUGFUNC();

	switch (hw->mac_type) {
	case e1000_82541:
	case e1000_82547:
	case e1000_82541_rev_2:
	case e1000_82547_rev_2:
		break;
	default:
		return;
	}

	mdelay(20);

	/* Save off the current value of register 0x2F5B to be
	 * restored at the end of this routine. */
	ret_val = e1000_read_phy_reg(hw, 0x2F5B, &phy_saved_data);

	/* Disabled the PHY transmitter */
	e1000_write_phy_reg(hw, 0x2F5B, 0x0003);

	mdelay(20);

	e1000_write_phy_reg(hw, 0x0000, 0x0140);

	mdelay(5);

	switch (hw->mac_type) {
	case e1000_82541:
	case e1000_82547:
		e1000_write_phy_reg(hw, 0x1F95, 0x0001);

		e1000_write_phy_reg(hw, 0x1F71, 0xBD21);

		e1000_write_phy_reg(hw, 0x1F79, 0x0018);

		e1000_write_phy_reg(hw, 0x1F30, 0x1600);

		e1000_write_phy_reg(hw, 0x1F31, 0x0014);

		e1000_write_phy_reg(hw, 0x1F32, 0x161C);

		e1000_write_phy_reg(hw, 0x1F94, 0x0003);

		e1000_write_phy_reg(hw, 0x1F96, 0x003F);

		e1000_write_phy_reg(hw, 0x2010, 0x0008);
		break;

	case e1000_82541_rev_2:
	case e1000_82547_rev_2:
		e1000_write_phy_reg(hw, 0x1F73, 0x0099);
		break;
	default:
		break;
	}

	e1000_write_phy_reg(hw, 0x0000, 0x3300);

	mdelay(20);

	/* Now enable the transmitter */
	if (!ret_val)
		e1000_write_phy_reg(hw, 0x2F5B, phy_saved_data);

	if (hw->mac_type == e1000_82547) {
		uint16_t fused, fine, coarse;

		/* Move to analog registers page */
		e1000_read_phy_reg(hw, IGP01E1000_ANALOG_SPARE_FUSE_STATUS, &fused);

		if (!(fused & IGP01E1000_ANALOG_SPARE_FUSE_ENABLED)) {
			e1000_read_phy_reg(hw, IGP01E1000_ANALOG_FUSE_STATUS, &fused);

			fine = fused & IGP01E1000_ANALOG_FUSE_FINE_MASK;
			coarse = fused & IGP01E1000_ANALOG_FUSE_COARSE_MASK;

			if (coarse >
				IGP01E1000_ANALOG_FUSE_COARSE_THRESH) {
				coarse -=
				IGP01E1000_ANALOG_FUSE_COARSE_10;
				fine -= IGP01E1000_ANALOG_FUSE_FINE_1;
			} else if (coarse
				== IGP01E1000_ANALOG_FUSE_COARSE_THRESH)
				fine -= IGP01E1000_ANALOG_FUSE_FINE_10;

			fused = (fused & IGP01E1000_ANALOG_FUSE_POLY_MASK) |
				(fine & IGP01E1000_ANALOG_FUSE_FINE_MASK) |
				(coarse & IGP01E1000_ANALOG_FUSE_COARSE_MASK);

			e1000_write_phy_reg(hw, IGP01E1000_ANALOG_FUSE_CONTROL, fused);
			e1000_write_phy_reg(hw, IGP01E1000_ANALOG_FUSE_BYPASS,
					IGP01E1000_ANALOG_FUSE_ENABLE_SW_CONTROL);
		}
	}
}

/******************************************************************************
* Resets the PHY
*
* hw - Struct containing variables accessed by shared code
*
* Sets bit 15 of the MII Control register
******************************************************************************/
static int32_t e1000_phy_reset(struct e1000_hw *hw)
{
	uint16_t phy_data;
	int ret;

	DEBUGFUNC();

	/*
	 * In the case of the phy reset being blocked, it's not an error, we
	 * simply return success without performing the reset.
	 */
	if (e1000_check_phy_reset_block(hw))
		return E1000_SUCCESS;

	switch (hw->phy_type) {
	case e1000_phy_igp:
	case e1000_phy_igp_2:
	case e1000_phy_igp_3:
	case e1000_phy_ife:
	case e1000_phy_igb:
		ret = e1000_phy_hw_reset(hw);
		if (ret)
			return ret;
		break;
	default:
		ret = e1000_read_phy_reg(hw, PHY_CTRL, &phy_data);
		if (ret)
			return ret;

		phy_data |= MII_CR_RESET;
		ret = e1000_write_phy_reg(hw, PHY_CTRL, phy_data);
		if (ret)
			return ret;

		udelay(1);
		break;
	}

	if (hw->phy_type == e1000_phy_igp || hw->phy_type == e1000_phy_igp_2)
		e1000_phy_init_script(hw);

	return E1000_SUCCESS;
}

/******************************************************************************
* Probes the expected PHY address for known PHY IDs
*
* hw - Struct containing variables accessed by shared code
******************************************************************************/
static int32_t e1000_detect_gig_phy(struct e1000_hw *hw)
{
	int32_t ret_val;
	uint16_t phy_id_high, phy_id_low;
	e1000_phy_type phy_type = e1000_phy_undefined;

	DEBUGFUNC();

	/* The 82571 firmware may still be configuring the PHY.  In this
	 * case, we cannot access the PHY until the configuration is done.  So
	 * we explicitly set the PHY values. */
	if (hw->mac_type == e1000_82571 || hw->mac_type == e1000_82572) {
		hw->phy_id = IGP01E1000_I_PHY_ID;
		hw->phy_type = e1000_phy_igp_2;
		return E1000_SUCCESS;
	}

	/* Read the PHY ID Registers to identify which PHY is onboard. */
	ret_val = e1000_read_phy_reg(hw, PHY_ID1, &phy_id_high);
	if (ret_val)
		return ret_val;

	hw->phy_id = (uint32_t) (phy_id_high << 16);
	udelay(20);
	ret_val = e1000_read_phy_reg(hw, PHY_ID2, &phy_id_low);
	if (ret_val)
		return ret_val;

	hw->phy_id |= (uint32_t) (phy_id_low & PHY_REVISION_MASK);
	hw->phy_revision = (uint32_t) phy_id_low & ~PHY_REVISION_MASK;

	switch (hw->mac_type) {
	case e1000_82543:
		if (hw->phy_id == M88E1000_E_PHY_ID)
			phy_type = e1000_phy_m88;
		break;
	case e1000_82544:
		if (hw->phy_id == M88E1000_I_PHY_ID)
			phy_type = e1000_phy_m88;
		break;
	case e1000_82540:
	case e1000_82545:
	case e1000_82545_rev_3:
	case e1000_82546:
	case e1000_82546_rev_3:
		if (hw->phy_id == M88E1011_I_PHY_ID)
			phy_type = e1000_phy_m88;
		break;
	case e1000_82541:
	case e1000_82541_rev_2:
	case e1000_82547:
	case e1000_82547_rev_2:
		if (hw->phy_id == IGP01E1000_I_PHY_ID)
			phy_type = e1000_phy_igp;

		break;
	case e1000_82573:
		if (hw->phy_id == M88E1111_I_PHY_ID)
			phy_type = e1000_phy_m88;
		break;
	case e1000_82574:
		if (hw->phy_id == BME1000_E_PHY_ID)
			phy_type = e1000_phy_bm;
		break;
	case e1000_80003es2lan:
		if (hw->phy_id == GG82563_E_PHY_ID)
			phy_type = e1000_phy_gg82563;
		break;
	case e1000_ich8lan:
		if (hw->phy_id == IGP03E1000_E_PHY_ID)
			phy_type = e1000_phy_igp_3;
		if (hw->phy_id == IFE_E_PHY_ID)
			phy_type = e1000_phy_ife;
		if (hw->phy_id == IFE_PLUS_E_PHY_ID)
			phy_type = e1000_phy_ife;
		if (hw->phy_id == IFE_C_E_PHY_ID)
			phy_type = e1000_phy_ife;
		break;
	case e1000_igb:
		if (hw->phy_id == I210_I_PHY_ID)
			phy_type = e1000_phy_igb;
		if (hw->phy_id == I350_I_PHY_ID)
			phy_type = e1000_phy_igb;
		break;
	default:
		dev_err(hw->dev, "Invalid MAC type %d\n", hw->mac_type);
		return -E1000_ERR_CONFIG;
	}

	if (phy_type == e1000_phy_undefined) {
		dev_err(hw->dev, "Invalid PHY ID 0x%X\n", hw->phy_id);
		return -EINVAL;
	}

	hw->phy_type = phy_type;

	return 0;
}

/*****************************************************************************
 * Set media type and TBI compatibility.
 *
 * hw - Struct containing variables accessed by shared code
 * **************************************************************************/
static void e1000_set_media_type(struct e1000_hw *hw)
{
	DEBUGFUNC();

	switch (hw->device_id) {
	case E1000_DEV_ID_82545GM_SERDES:
	case E1000_DEV_ID_82546GB_SERDES:
	case E1000_DEV_ID_82571EB_SERDES:
	case E1000_DEV_ID_82571EB_SERDES_DUAL:
	case E1000_DEV_ID_82571EB_SERDES_QUAD:
	case E1000_DEV_ID_82572EI_SERDES:
	case E1000_DEV_ID_80003ES2LAN_SERDES_DPT:
		hw->media_type = e1000_media_type_internal_serdes;
		return;
	default:
		break;
	}

	switch (hw->mac_type) {
	case e1000_82542_rev2_0:
	case e1000_82542_rev2_1:
		hw->media_type = e1000_media_type_fiber;
		return;
	case e1000_ich8lan:
	case e1000_82573:
	case e1000_82574:
	case e1000_igb:
		/* The STATUS_TBIMODE bit is reserved or reused
		 * for the this device.
		 */
		hw->media_type = e1000_media_type_copper;
		return;
	default:
		break;
	}

	if (e1000_read_reg(hw, E1000_STATUS) & E1000_STATUS_TBIMODE)
		hw->media_type = e1000_media_type_fiber;
	else
		hw->media_type = e1000_media_type_copper;
}

/**
 * e1000_sw_init - Initialize general software structures (struct e1000_adapter)
 *
 * e1000_sw_init initializes the Adapter private data structure.
 * Fields are initialized based on PCI device information and
 * OS network device settings (MTU size).
 **/

static int e1000_sw_init(struct eth_device *edev)
{
	struct e1000_hw *hw = edev->priv;
	int result;

	/* PCI config space info */
	pci_read_config_word(hw->pdev, PCI_VENDOR_ID, &hw->vendor_id);
	pci_read_config_word(hw->pdev, PCI_DEVICE_ID, &hw->device_id);
	pci_read_config_byte(hw->pdev, PCI_REVISION_ID, &hw->revision_id);
	pci_read_config_word(hw->pdev, PCI_COMMAND, &hw->pci_cmd_word);

	/* identify the MAC */
	result = e1000_set_mac_type(hw);
	if (result) {
		dev_err(&hw->edev.dev, "Unknown MAC Type\n");
		return result;
	}

	return E1000_SUCCESS;
}

static void fill_rx(struct e1000_hw *hw)
{
	volatile struct e1000_rx_desc *rd;
	volatile u32 *bla;
	int i;

	hw->rx_last = hw->rx_tail;
	rd = hw->rx_base + hw->rx_tail;
	hw->rx_tail = (hw->rx_tail + 1) % 8;

	bla = (void *)rd;
	for (i = 0; i < 4; i++)
		*bla++ = 0;

	rd->buffer_addr = cpu_to_le64((unsigned long)hw->packet);

	e1000_write_reg(hw, E1000_RDT, hw->rx_tail);
}

/**
 * e1000_configure_tx - Configure 8254x Transmit Unit after Reset
 * @adapter: board private structure
 *
 * Configure the Tx unit of the MAC after a reset.
 **/

static void e1000_configure_tx(struct e1000_hw *hw)
{
	unsigned long tctl;
	unsigned long tipg, tarc;
	uint32_t ipgr1, ipgr2;

	e1000_write_reg(hw, E1000_TDBAL, (unsigned long)hw->tx_base);
	e1000_write_reg(hw, E1000_TDBAH, 0);

	e1000_write_reg(hw, E1000_TDLEN, 128);

	/* Setup the HW Tx Head and Tail descriptor pointers */
	e1000_write_reg(hw, E1000_TDH, 0);
	e1000_write_reg(hw, E1000_TDT, 0);
	hw->tx_tail = 0;

	/* Set the default values for the Tx Inter Packet Gap timer */
	if (hw->mac_type <= e1000_82547_rev_2 &&
	    (hw->media_type == e1000_media_type_fiber ||
	     hw->media_type == e1000_media_type_internal_serdes))
		tipg = DEFAULT_82543_TIPG_IPGT_FIBER;
	else
		tipg = DEFAULT_82543_TIPG_IPGT_COPPER;

	/* Set the default values for the Tx Inter Packet Gap timer */
	switch (hw->mac_type) {
	case e1000_82542_rev2_0:
	case e1000_82542_rev2_1:
		tipg = DEFAULT_82542_TIPG_IPGT;
		ipgr1 = DEFAULT_82542_TIPG_IPGR1;
		ipgr2 = DEFAULT_82542_TIPG_IPGR2;
		break;
	case e1000_80003es2lan:
		ipgr1 = DEFAULT_82543_TIPG_IPGR1;
		ipgr2 = DEFAULT_80003ES2LAN_TIPG_IPGR2;
		break;
	default:
		ipgr1 = DEFAULT_82543_TIPG_IPGR1;
		ipgr2 = DEFAULT_82543_TIPG_IPGR2;
		break;
	}
	tipg |= ipgr1 << E1000_TIPG_IPGR1_SHIFT;
	tipg |= ipgr2 << E1000_TIPG_IPGR2_SHIFT;
	e1000_write_reg(hw, E1000_TIPG, tipg);
	/* Program the Transmit Control Register */
	tctl = e1000_read_reg(hw, E1000_TCTL);
	tctl &= ~E1000_TCTL_CT;
	tctl |= E1000_TCTL_EN | E1000_TCTL_PSP |
	    (E1000_COLLISION_THRESHOLD << E1000_CT_SHIFT);

	if (hw->mac_type == e1000_82571 || hw->mac_type == e1000_82572) {
		tarc = e1000_read_reg(hw, E1000_TARC0);
		/* set the speed mode bit, we'll clear it if we're not at
		 * gigabit link later */
		/* git bit can be set to 1*/
	} else if (hw->mac_type == e1000_80003es2lan) {
		tarc = e1000_read_reg(hw, E1000_TARC0);
		tarc |= 1;
		e1000_write_reg(hw, E1000_TARC0, tarc);
		tarc = e1000_read_reg(hw, E1000_TARC1);
		tarc |= 1;
		e1000_write_reg(hw, E1000_TARC1, tarc);
	}


	e1000_config_collision_dist(hw);
	/* Setup Transmit Descriptor Settings for eop descriptor */
	hw->txd_cmd = E1000_TXD_CMD_EOP | E1000_TXD_CMD_IFCS;

	/* Need to set up RS bit */
	if (hw->mac_type < e1000_82543)
		hw->txd_cmd |= E1000_TXD_CMD_RPS;
	else
		hw->txd_cmd |= E1000_TXD_CMD_RS;


	if (hw->mac_type == e1000_igb) {
		uint32_t reg_txdctl;

		e1000_write_reg(hw, E1000_TCTL_EXT, 0x42 << 10);

		reg_txdctl = e1000_read_reg(hw, E1000_TXDCTL);
		reg_txdctl |= 1 << 25;
		e1000_write_reg(hw, E1000_TXDCTL, reg_txdctl);
		mdelay(20);
	}

	e1000_write_reg(hw, E1000_TCTL, tctl);
}

/**
 * e1000_setup_rctl - configure the receive control register
 * @adapter: Board private structure
 **/
static void e1000_setup_rctl(struct e1000_hw *hw)
{
	uint32_t rctl;

	rctl = e1000_read_reg(hw, E1000_RCTL);

	rctl &= ~(3 << E1000_RCTL_MO_SHIFT);

	rctl |= E1000_RCTL_EN | E1000_RCTL_BAM | E1000_RCTL_LBM_NO
		| E1000_RCTL_RDMTS_HALF;	/* |
			(hw.mc_filter_type << E1000_RCTL_MO_SHIFT); */

	rctl &= ~E1000_RCTL_SBP;

	rctl &= ~(E1000_RCTL_SZ_4096);
		rctl |= E1000_RCTL_SZ_2048;
		rctl &= ~(E1000_RCTL_BSEX | E1000_RCTL_LPE);
	e1000_write_reg(hw, E1000_RCTL, rctl);
}

/**
 * e1000_configure_rx - Configure 8254x Receive Unit after Reset
 * @adapter: board private structure
 *
 * Configure the Rx unit of the MAC after a reset.
 **/
static void e1000_configure_rx(struct e1000_hw *hw)
{
	unsigned long rctl, ctrl_ext;

	hw->rx_tail = 0;
	/* make sure receives are disabled while setting up the descriptors */
	rctl = e1000_read_reg(hw, E1000_RCTL);
	e1000_write_reg(hw, E1000_RCTL, rctl & ~E1000_RCTL_EN);
	if (hw->mac_type >= e1000_82540) {
		/* Set the interrupt throttling rate.  Value is calculated
		 * as DEFAULT_ITR = 1/(MAX_INTS_PER_SEC * 256ns) */
#define MAX_INTS_PER_SEC	8000
#define DEFAULT_ITR		1000000000/(MAX_INTS_PER_SEC * 256)
		e1000_write_reg(hw, E1000_ITR, DEFAULT_ITR);
	}

	if (hw->mac_type >= e1000_82571) {
		ctrl_ext = e1000_read_reg(hw, E1000_CTRL_EXT);
		/* Reset delay timers after every interrupt */
		ctrl_ext |= E1000_CTRL_EXT_INT_TIMER_CLR;
		e1000_write_reg(hw, E1000_CTRL_EXT, ctrl_ext);
		e1000_write_flush(hw);
	}
	/* Setup the Base and Length of the Rx Descriptor Ring */
	e1000_write_reg(hw, E1000_RDBAL, (unsigned long)hw->rx_base);
	e1000_write_reg(hw, E1000_RDBAH, 0);

	e1000_write_reg(hw, E1000_RDLEN, 128);

	/* Setup the HW Rx Head and Tail Descriptor Pointers */
	e1000_write_reg(hw, E1000_RDH, 0);
	e1000_write_reg(hw, E1000_RDT, 0);
	/* Enable Receives */

	if (hw->mac_type == e1000_igb) {
		uint32_t reg_rxdctl = e1000_read_reg(hw, E1000_RXDCTL);
		reg_rxdctl |= 1 << 25;
		e1000_write_reg(hw, E1000_RXDCTL, reg_rxdctl);
		mdelay(20);
	}

	e1000_write_reg(hw, E1000_RCTL, rctl);

	fill_rx(hw);
}

static int e1000_poll(struct eth_device *edev)
{
	struct e1000_hw *hw = edev->priv;
	volatile struct e1000_rx_desc *rd;
	uint32_t len;

	rd = hw->rx_base + hw->rx_last;

	if (!(le32_to_cpu(rd->status)) & E1000_RXD_STAT_DD)
		return 0;

	len = le32_to_cpu(rd->length);

	dma_sync_single_for_cpu((unsigned long)hw->packet, len, DMA_FROM_DEVICE);

	net_receive(edev, (uchar *)hw->packet, len);
	fill_rx(hw);
	return 1;
}

static int e1000_transmit(struct eth_device *edev, void *txpacket, int length)
{
	struct e1000_hw *hw = edev->priv;
	volatile struct e1000_tx_desc *txp;
	uint64_t to;

	txp = hw->tx_base + hw->tx_tail;
	hw->tx_tail = (hw->tx_tail + 1) % 8;

	txp->buffer_addr = cpu_to_le64(virt_to_bus(hw->pdev, txpacket));
	txp->lower.data = cpu_to_le32(hw->txd_cmd | length);
	txp->upper.data = 0;

	dma_sync_single_for_device((unsigned long)txpacket, length, DMA_TO_DEVICE);

	e1000_write_reg(hw, E1000_TDT, hw->tx_tail);

	e1000_write_flush(hw);

	to = get_time_ns();
	while (1) {
		if (le32_to_cpu(txp->upper.data) & E1000_TXD_STAT_DD)
			break;
		if (is_timeout(to, MSECOND)) {
			dev_dbg(hw->dev, "e1000: tx timeout\n");
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static void e1000_disable(struct eth_device *edev)
{
	struct e1000_hw *hw = edev->priv;

	/* Turn off the ethernet interface */
	e1000_write_reg(hw, E1000_RCTL, 0);
	e1000_write_reg(hw, E1000_TCTL, 0);

	/* Clear the transmit ring */
	e1000_write_reg(hw, E1000_TDH, 0);
	e1000_write_reg(hw, E1000_TDT, 0);

	/* Clear the receive ring */
	e1000_write_reg(hw, E1000_RDH, 0);
	e1000_write_reg(hw, E1000_RDT, 0);

	mdelay(10);
}

static int e1000_init(struct eth_device *edev)
{
	struct e1000_hw *hw = edev->priv;
	uint32_t i;
	uint32_t mta_size;
	uint32_t reg_data;

	DEBUGFUNC();

	if (hw->mac_type >= e1000_82544)
		e1000_write_reg(hw, E1000_WUC, 0);

	/* force full DMA clock frequency for 10/100 on ICH8 A0-B0 */
	if ((hw->mac_type == e1000_ich8lan) && ((hw->revision_id < 3) ||
	    ((hw->device_id != E1000_DEV_ID_ICH8_IGP_M_AMT) &&
	     (hw->device_id != E1000_DEV_ID_ICH8_IGP_M)))) {
		reg_data = e1000_read_reg(hw, E1000_STATUS);
		reg_data &= ~0x80000000;
		e1000_write_reg(hw, E1000_STATUS, reg_data);
	}

	/* Set the media type and TBI compatibility */
	e1000_set_media_type(hw);

	/* Must be called after e1000_set_media_type
	 * because media_type is used */
	e1000_initialize_hardware_bits(hw);

	/* Disabling VLAN filtering. */
	/* VET hardcoded to standard value and VFTA removed in ICH8 LAN */
	if (hw->mac_type != e1000_ich8lan) {
		if (hw->mac_type < e1000_82545_rev_3)
			e1000_write_reg(hw, E1000_VET, 0);
		e1000_clear_vfta(hw);
	}

	/* For 82542 (rev 2.0), disable MWI and put the receiver into reset */
	if (hw->mac_type == e1000_82542_rev2_0) {
		dev_dbg(hw->dev, "Disabling MWI on 82542 rev 2.0\n");
		pci_write_config_word(hw->pdev, PCI_COMMAND,
				      hw->pci_cmd_word & ~PCI_COMMAND_INVALIDATE);
		e1000_write_reg(hw, E1000_RCTL, E1000_RCTL_RST);
		e1000_write_flush(hw);
		mdelay(5);
	}

	for (i = 1; i < E1000_RAR_ENTRIES; i++) {
		e1000_write_reg_array(hw, E1000_RA, (i << 1), 0);
		e1000_write_reg_array(hw, E1000_RA, (i << 1) + 1, 0);
	}

	/* For 82542 (rev 2.0), take the receiver out of reset and enable MWI */
	if (hw->mac_type == e1000_82542_rev2_0) {
		e1000_write_reg(hw, E1000_RCTL, 0);
		e1000_write_flush(hw);
		mdelay(1);
		pci_write_config_word(hw->pdev, PCI_COMMAND, hw->pci_cmd_word);
	}

	/* Zero out the Multicast HASH table */
	mta_size = E1000_MC_TBL_SIZE;
	if (hw->mac_type == e1000_ich8lan)
		mta_size = E1000_MC_TBL_SIZE_ICH8LAN;

	for (i = 0; i < mta_size; i++) {
		e1000_write_reg_array(hw, E1000_MTA, i, 0);
		/* use write flush to prevent Memory Write Block (MWB) from
		 * occuring when accessing our register space */
		e1000_write_flush(hw);
	}

	/* More time needed for PHY to initialize */
	if (hw->mac_type == e1000_ich8lan)
		mdelay(15);
	if (hw->mac_type == e1000_igb)
		mdelay(15);

	e1000_configure_tx(hw);
	e1000_configure_rx(hw);
	e1000_setup_rctl(hw);

	return 0;
}

static int e1000_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct e1000_hw *hw;
	struct eth_device *edev;
	int ret;

	pci_enable_device(pdev);
	pci_set_master(pdev);

	hw = xzalloc(sizeof(*hw));

	hw->tx_base = dma_alloc_coherent(16 * sizeof(*hw->tx_base), DMA_ADDRESS_BROKEN);
	hw->rx_base = dma_alloc_coherent(16 * sizeof(*hw->rx_base), DMA_ADDRESS_BROKEN);
	hw->packet = dma_alloc_coherent(4096, DMA_ADDRESS_BROKEN);

	edev = &hw->edev;

	hw->pdev = pdev;
	hw->dev = &pdev->dev;
	pdev->dev.priv = hw;
	edev->priv = hw;

	hw->hw_addr = pci_iomap(pdev, 0);

	/* MAC and Phy settings */
	if (e1000_sw_init(edev) < 0) {
		dev_err(&pdev->dev, "Software init failed\n");
		return -EINVAL;
	}

	if (e1000_check_phy_reset_block(hw))
		dev_err(&pdev->dev, "PHY Reset is blocked!\n");

	/* Basic init was OK, reset the hardware and allow SPI access */
	e1000_reset_hw(hw);

	/* Validate the EEPROM and get chipset information */
	if (e1000_init_eeprom_params(hw)) {
		dev_err(&pdev->dev, "EEPROM is invalid!\n");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_MTD)) {
		ret = e1000_register_eeprom(hw);
		if (ret < 0) {
			dev_err(&pdev->dev,
			        "failed to register EEPROM devices!\n");
			return ret;
		}
	}

	if (e1000_validate_eeprom_checksum(hw))
		return 0;

	e1000_get_ethaddr(edev, edev->ethaddr);

	/* Set up the function pointers and register the device */
	edev->parent = &pdev->dev;
	edev->init = e1000_init;
	edev->recv = e1000_poll;
	edev->send = e1000_transmit;
	edev->halt = e1000_disable;
	edev->open = e1000_open;
	edev->get_ethaddr = e1000_get_ethaddr;
	edev->set_ethaddr = e1000_set_ethaddr;

	hw->miibus.read = e1000_phy_read;
	hw->miibus.write = e1000_phy_write;
	hw->miibus.priv = hw;
	hw->miibus.parent = &edev->dev;

	ret = eth_register(edev);
	if (ret)
		return ret;

	/*
	 * The e1000 driver does its own phy handling, but registering
	 * the phy allows to show the phy registers for debugging purposes.
	 */
	ret = mdiobus_register(&hw->miibus);
	if (ret)
		return ret;

	return 0;
}

static void e1000_remove(struct pci_dev *pdev)
{
	struct e1000_hw *hw = pdev->dev.priv;

	e1000_disable(&hw->edev);
}

static DEFINE_PCI_DEVICE_TABLE(e1000_pci_tbl) = {
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82542), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82543GC_FIBER), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82543GC_COPPER), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82544EI_COPPER), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82544EI_FIBER), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82544GC_COPPER), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82544GC_LOM), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82540EM), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82545EM_COPPER), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82545GM_COPPER), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82546EB_COPPER), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82545EM_FIBER), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82546EB_FIBER), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82546GB_COPPER), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82540EM_LOM), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82541ER), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82541GI_LF), },
	/* E1000 PCIe card */
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82571EB_COPPER), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82571EB_FIBER), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82571EB_SERDES), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82571EB_QUAD_COPPER), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82571PT_QUAD_COPPER), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82571EB_QUAD_FIBER), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82571EB_QUAD_COPPER_LOWPROFILE), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82571EB_SERDES_DUAL), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82571EB_SERDES_QUAD), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82572EI_COPPER), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82572EI_FIBER), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82572EI_SERDES), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82572EI), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82573E), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82573E_IAMT), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82573L), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82574L), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_82546GB_QUAD_COPPER_KSP3), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_80003ES2LAN_COPPER_DPT), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_80003ES2LAN_SERDES_DPT), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_80003ES2LAN_COPPER_SPT), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_80003ES2LAN_SERDES_SPT), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_I210_UNPROGRAMMED), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_I211_UNPROGRAMMED), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_I210_COPPER), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_I211_COPPER), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_I210_COPPER_FLASHLESS), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_I210_SERDES), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_I210_SERDES_FLASHLESS), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_I210_1000BASEKX), },
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, E1000_DEV_ID_I350_COPPER), },
	{ /* sentinel */ }
};

static struct pci_driver e1000_eth_driver = {
	.name = "e1000",
	.id_table = e1000_pci_tbl,
	.probe = e1000_probe,
	.remove = e1000_remove,
};

static int e1000_driver_init(void)
{
	return pci_register_driver(&e1000_eth_driver);
}
device_initcall(e1000_driver_init);
