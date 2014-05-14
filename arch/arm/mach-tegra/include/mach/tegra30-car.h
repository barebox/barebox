/*
 * Copyright (C) 2014 Lucas Stach <l.stach@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Register definitions */
#define CRC_CLK_OUT_ENB_V		0x360
#define CRC_CLK_OUT_ENB_V_MSELECT	(1 << 3)

#define CRC_CLK_SOURCE_MSEL		0x3b4
#define CRC_CLK_SOURCE_MSEL_SRC_SHIFT	30
#define CRC_CLK_SOURCE_MSEL_SRC_PLLP	0
#define CRC_CLK_SOURCE_MSEL_SRC_PLLC	1
#define CRC_CLK_SOURCE_MSEL_SRC_PLLM	2
#define CRC_CLK_SOURCE_MSEL_SRC_CLKM	3

#define CRC_CLK_SOURCE_I2C4		0x3c4

#define CRC_RST_DEV_V_SET		0x430
#define CRC_RST_DEV_V_MSELECT		(1 << 3)

#define CRC_RST_DEV_V_CLR		0x434
