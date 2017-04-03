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

#ifndef _CYCLONE5_FREEZE_CONTROLLER_H_
#define _CYCLONE5_FREEZE_CONTROLLER_H_

#include <mach/cyclone5-regs.h>

#define SYSMGR_FRZCTRL_ADDRESS		0x40
#define SYSMGR_FRZCTRL_VIOCTRL_ADDRESS	0x40
#define SYSMGR_FRZCTRL_HIOCTRL_ADDRESS	0x50
#define SYSMGR_FRZCTRL_SRC_ADDRESS	0x54
#define SYSMGR_FRZCTRL_HWCTRL_ADDRESS	0x58

#define SYSMGR_FRZCTRL_SRC_VIO1_ENUM_SW 0x0
#define SYSMGR_FRZCTRL_SRC_VIO1_ENUM_HW 0x1
#define SYSMGR_FRZCTRL_VIOCTRL_SLEW_MASK 0x00000010
#define SYSMGR_FRZCTRL_VIOCTRL_WKPULLUP_MASK 0x00000008
#define SYSMGR_FRZCTRL_VIOCTRL_TRISTATE_MASK 0x00000004
#define SYSMGR_FRZCTRL_VIOCTRL_BUSHOLD_MASK 0x00000002
#define SYSMGR_FRZCTRL_VIOCTRL_CFG_MASK 0x00000001
#define SYSMGR_FRZCTRL_HIOCTRL_SLEW_MASK 0x00000010
#define SYSMGR_FRZCTRL_HIOCTRL_WKPULLUP_MASK 0x00000008
#define SYSMGR_FRZCTRL_HIOCTRL_TRISTATE_MASK 0x00000004
#define SYSMGR_FRZCTRL_HIOCTRL_BUSHOLD_MASK 0x00000002
#define SYSMGR_FRZCTRL_HIOCTRL_CFG_MASK 0x00000001
#define SYSMGR_FRZCTRL_HIOCTRL_REGRST_MASK 0x00000080
#define SYSMGR_FRZCTRL_HIOCTRL_OCTRST_MASK 0x00000040
#define SYSMGR_FRZCTRL_HIOCTRL_OCT_CFGEN_CALSTART_MASK 0x00000100
#define SYSMGR_FRZCTRL_HIOCTRL_DLLRST_MASK 0x00000020
#define SYSMGR_FRZCTRL_HWCTRL_VIO1REQ_MASK 0x00000001
#define SYSMGR_FRZCTRL_HWCTRL_VIO1STATE_ENUM_FROZEN 0x2
#define SYSMGR_FRZCTRL_HWCTRL_VIO1STATE_ENUM_THAWED 0x1

#define SYSMGR_FRZCTRL_HWCTRL_VIO1STATE_GET(x) (((x) & 0x00000006) >> 1)

/*
 * FreezeChannelSelect
 * Definition of enum for freeze channel
 */
enum frz_channel_id {
	FREEZE_CHANNEL_0 = 0,   /* EMAC_IO & MIXED2_IO */
	FREEZE_CHANNEL_1,   /* MIXED1_IO and FLASH_IO */
	FREEZE_CHANNEL_2,   /* General IO */
	FREEZE_CHANNEL_3,   /* DDR IO */
};

/* Shift count needed to calculte for FRZCTRL VIO control register offset */
#define SYSMGR_FRZCTRL_VIOCTRL_SHIFT    (2)

/*
 * Freeze HPS IOs
 *
 * FreezeChannelSelect [in] - Freeze channel ID
 * FreezeControllerFSMSelect [in] - To use hardware or software state machine
 * If FREEZE_CONTROLLER_FSM_HW is selected for FSM select then the
 *       the freeze channel id is input is ignored. It is default to channel 1
 */
int sys_mgr_frzctrl_freeze_req(enum frz_channel_id channel_id);

/*
 * Unfreeze/Thaw HPS IOs
 *
 * FreezeChannelSelect [in] - Freeze channel ID
 * FreezeControllerFSMSelect [in] - To use hardware or software state machine
 * If FREEZE_CONTROLLER_FSM_HW is selected for FSM select then the
 *       the freeze channel id is input is ignored. It is default to channel 1
 */
int sys_mgr_frzctrl_thaw_req(enum frz_channel_id channel_id);

#endif	/* _FREEZE_CONTROLLER_H_ */
