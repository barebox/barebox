/*
 * Auther:
 *       Vaibhav Hiremath <hvaibhav@ti.com>
 *
 * Copyright (C) 2010
 * Texas Instruments Incorporated - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef _EMIF_H_
#define _EMIF_H_

#define EMIF4_MOD_ID_REV					0x0
#define EMIF4_STATUS						0x04
#define EMIF4_SDRAM_CONFIG					0x08
#define EMIF4_SDRAM_CONFIG2					0x0c
#define EMIF4_SDRAM_REF_CTRL					0x10
#define EMIF4_SDRAM_REF_CTRL_SHADOW				0x14
#define EMIF4_SDRAM_TIM_1					0x18
#define EMIF4_SDRAM_TIM_1_SHADOW				0x1c
#define EMIF4_SDRAM_TIM_2					0x20
#define EMIF4_SDRAM_TIM_2_SHADOW				0x24
#define EMIF4_SDRAM_TIM_3					0x28
#define EMIF4_SDRAM_TIM_3_SHADOW				0x2c
#define EMIF4_POWER_MANAGEMENT_CTRL				0x38
#define EMIF4_POWER_MANAGEMENT_CTRL_SHADOW			0x3c
#define EMIF4_OCP_CONFIG					0x54
#define EMIF4_ZQ_CONFIG						0xc8
#define EMIF4_DDR_PHY_CTRL_1					0xe4
#define EMIF4_DDR_PHY_CTRL_1_SHADOW				0xe8
#define EMIF4_DDR_PHY_CTRL_2					0xec
#define EMIF4_IODFT_TLGC					0x60

void am35xx_emif4_init(void);

#endif /* endif _EMIF_H_ */
