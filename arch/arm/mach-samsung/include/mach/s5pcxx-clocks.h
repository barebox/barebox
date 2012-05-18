/*
 * Copyright (C) 2012 Alexey Galakhov
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
 */

# define S5P_APLL 0x00
# define S5P_MPLL 0x08
# define S5P_EPLL 0x10
# define S5P_VPLL 0x20
# define S5P_xPLL_LOCK (S5P_CLOCK_POWER_BASE)
# define S5P_xPLL_CON  (S5P_CLOCK_POWER_BASE + 0x100)
# define S5P_xPLL_CON0 (S5P_xPLL_CON)
# define S5P_xPLL_CON1 (S5P_xPLL_CON + 0x4)

# define S5P_xPLLCON_GET_MDIV(x)  (((x) >> 16) & 0x3ff)
# define S5P_xPLLCON_GET_PDIV(x)  (((x) >> 8) & 0x3f)
# define S5P_xPLLCON_GET_SDIV(x)  ((x) & 0x3)

# define S5P_CLK_SRC0 (S5P_CLOCK_POWER_BASE + 0x200)
# define S5P_CLK_SRC1 (S5P_CLOCK_POWER_BASE + 0x204)
# define S5P_CLK_SRC2 (S5P_CLOCK_POWER_BASE + 0x208)
# define S5P_CLK_SRC3 (S5P_CLOCK_POWER_BASE + 0x20C)
# define S5P_CLK_SRC4 (S5P_CLOCK_POWER_BASE + 0x210)
# define S5P_CLK_SRC5 (S5P_CLOCK_POWER_BASE + 0x214)
# define S5P_CLK_SRC6 (S5P_CLOCK_BASE + 0x218)

# define S5P_CLK_DIV0 (S5P_CLOCK_POWER_BASE + 0x300)
# define S5P_CLK_DIV1 (S5P_CLOCK_POWER_BASE + 0x304)
# define S5P_CLK_DIV2 (S5P_CLOCK_POWER_BASE + 0x308)
# define S5P_CLK_DIV3 (S5P_CLOCK_POWER_BASE + 0x30C)
# define S5P_CLK_DIV4 (S5P_CLOCK_POWER_BASE + 0x310)
# define S5P_CLK_DIV5 (S5P_CLOCK_POWER_BASE + 0x314)
# define S5P_CLK_DIV6 (S5P_CLOCK_POWER_BASE + 0x318)
# define S5P_CLK_DIV7 (S5P_CLOCK_POWER_BASE + 0x31C)

# define S5P_CLK_GATE_SCLK  (S5P_CLOCK_POWER_BASE + 0x444)
# define S5P_CLK_GATE_IP0   (S5P_CLOCK_POWER_BASE + 0x460)
# define S5P_CLK_GATE_IP1   (S5P_CLOCK_POWER_BASE + 0x464)
# define S5P_CLK_GATE_IP2   (S5P_CLOCK_POWER_BASE + 0x468)
# define S5P_CLK_GATE_IP3   (S5P_CLOCK_POWER_BASE + 0x46C)
# define S5P_CLK_GATE_IP4   (S5P_CLOCK_POWER_BASE + 0x470)
# define S5P_CLK_GATE_BLOCK (S5P_CLOCK_POWER_BASE + 0x480)
# define S5P_CLK_GATE_IP5   (S5P_CLOCK_POWER_BASE + 0x484)

# define S5P_OTHERS (S5P_CLOCK_POWER_BASE + 0xE000)
# define S5P_USB_PHY_CONTROL (S5P_CLOCK_POWER_BASE + 0xE80C)
