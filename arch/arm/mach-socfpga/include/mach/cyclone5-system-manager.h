/*
 *  Copyright (C) 2012 Altera Corporation <www.altera.com>
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef	_SYSTEM_MANAGER_H_
#define	_SYSTEM_MANAGER_H_

void socfpga_sysmgr_pinmux_init(unsigned long *sys_mgr_init_table, int num);

/* address */
#define CONFIG_SYSMGR_ROMCODEGRP_CTRL	(CYCLONE5_SYSMGR_ADDRESS + 0xc0)

/* FPGA interface group */
#define SYSMGR_FPGAINTF_MODULE		(CYCLONE5_SYSMGR_ADDRESS + 0x28)
/* EMAC interface selection */
#define CONFIG_SYSMGR_EMAC_CTRL		(CYCLONE5_SYSMGR_ADDRESS + 0x60)

#define ISWGRP_HANDOFF_AXIBRIDGE	SYSMGR_ISWGRP_HANDOFF0
#define ISWGRP_HANDOFF_L3REMAP		SYSMGR_ISWGRP_HANDOFF1
#define ISWGRP_HANDOFF_FPGAINTF		SYSMGR_ISWGRP_HANDOFF2
#define ISWGRP_HANDOFF_FPGA2SDR		SYSMGR_ISWGRP_HANDOFF3

/* pin mux */
#define SYSMGR_PINMUXGRP		(CYCLONE5_SYSMGR_ADDRESS + 0x400)
#define SYSMGR_PINMUXGRP_NANDUSEFPGA	(SYSMGR_PINMUXGRP + 0x2F0)
#define SYSMGR_PINMUXGRP_EMAC1USEFPGA	(SYSMGR_PINMUXGRP + 0x2F8)
#define SYSMGR_PINMUXGRP_SDMMCUSEFPGA	(SYSMGR_PINMUXGRP + 0x308)
#define SYSMGR_PINMUXGRP_EMAC0USEFPGA	(SYSMGR_PINMUXGRP + 0x314)
#define SYSMGR_PINMUXGRP_SPIM1USEFPGA	(SYSMGR_PINMUXGRP + 0x330)
#define SYSMGR_PINMUXGRP_SPIM0USEFPGA	(SYSMGR_PINMUXGRP + 0x338)

/* bit fields */
#define CONFIG_SYSMGR_PINMUXGRP_OFFSET	(0x400)
#define SYSMGR_ROMCODEGRP_CTRL_WARMRSTCFGPINMUX		(1<<0)
#define SYSMGR_ROMCODEGRP_CTRL_WARMRSTCFGIO		(1<<1)
#define SYSMGR_ECC_OCRAM_EN		(1<<0)
#define SYSMGR_ECC_OCRAM_SERR		(1<<3)
#define SYSMGR_ECC_OCRAM_DERR		(1<<4)
#define SYSMGR_FPGAINTF_USEFPGA		0x1
#define SYSMGR_FPGAINTF_SPIM0		(1<<0)
#define SYSMGR_FPGAINTF_SPIM1		(1<<1)
#define SYSMGR_FPGAINTF_EMAC0		(1<<2)
#define SYSMGR_FPGAINTF_EMAC1		(1<<3)
#define SYSMGR_FPGAINTF_NAND		(1<<4)
#define SYSMGR_FPGAINTF_SDMMC		(1<<5)

/* Enumeration: sysmgr::emacgrp::ctrl::physel::enum                        */
#define SYSMGR_EMACGRP_CTRL_PHYSEL_ENUM_GMII_MII 0x0
#define SYSMGR_EMACGRP_CTRL_PHYSEL_ENUM_RGMII 0x1
#define SYSMGR_EMACGRP_CTRL_PHYSEL_ENUM_RMII 0x2
#define SYSMGR_EMACGRP_CTRL_PHYSEL0_LSB 0
#define SYSMGR_EMACGRP_CTRL_PHYSEL1_LSB 2
#define SYSMGR_EMACGRP_CTRL_PHYSEL_MASK 0x00000003

#define SYSMGR_FPGAGRP_MODULE  0x00000028
#define SYSMGR_FPGAGRP_MODULE_EMAC 0x00000004

#endif /* _SYSTEM_MANAGER_H_ */
