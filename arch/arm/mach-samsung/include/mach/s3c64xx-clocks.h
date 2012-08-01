/*
 * Copyright (C) 2012 Juergen Beisert
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

#define S3C_EPLL_LOCK  (S3C_CLOCK_POWER_BASE + 0x08)
# define S3C_EPLL_LOCK_PLL_LOCKTIME(x) ((x) & 0xffff)
#define S3C_APLLCON (S3C_CLOCK_POWER_BASE + 0x0c)
# define S3C_APLLCON_ENABLE (1 << 31)
# define S3C_APLLCON_GET_MDIV(x) (((x) >> 16) & 0x3ff)
# define S3C_APLLCON_GET_PDIV(x) (((x) >> 8) & 0x3f)
# define S3C_APLLCON_GET_SDIV(x) ((x) & 0x7)
#define S3C_MPLLCON (S3C_CLOCK_POWER_BASE + 0x10)
# define S3C_MPLLCON_ENABLE (1 << 31)
# define S3C_MPLLCON_GET_MDIV(x) (((x) >> 16) & 0x3ff)
# define S3C_MPLLCON_GET_PDIV(x) (((x) >> 8) & 0x3f)
# define S3C_MPLLCON_GET_SDIV(x) ((x) & 0x7)
#define S3C_EPLLCON0 (S3C_CLOCK_POWER_BASE + 0x14)
# define S3C_EPLLCON0_ENABLE (1 << 31)
# define S3C_EPLLCON0_GET_MDIV(x) (((x) >> 16) & 0xff)
# define S3C_EPLLCON0_SET_MDIV(x) (((x) & 0xff) << 16)
# define S3C_EPLLCON0_GET_PDIV(x) (((x) >> 8) & 0x3f)
# define S3C_EPLLCON0_SET_PDIV(x) (((x) & 0x3f) << 8)
# define S3C_EPLLCON0_GET_SDIV(x) ((x) & 0x7)
# define S3C_EPLLCON0_SET_SDIV(x) ((x) & 0x7)
#define S3C_EPLLCON1 (S3C_CLOCK_POWER_BASE + 0x18)
# define S3C_EPLLCON1_GET_KDIV(x) ((x) & 0xffff)
# define S3C_EPLLCON1_SET_KDIV(x) ((x) & 0xffff)
#define S3C_CLKCON (S3C_CLOCK_POWER_BASE + 0xc)
#define S3C_CLKSLOW (S3C_CLOCK_POWER_BASE + 0x10)
#define S3C_CLKDIVN (S3C_CLOCK_POWER_BASE + 0x14)
#define S3C_CLK_SRC (S3C_CLOCK_POWER_BASE + 0x01c)
# define S3C_CLK_SRC_GET_MMC_SEL(x, v) (((v) >> (18 + (x * 2))) & 0x3)
# define S3C_CLK_SRC_SET_MMC_SEL(x, v) (((v) & 0x3) << (18 + (x * 2)))
# define S3C_CLK_SRC_UARTMPLL (1 << 13)
# define S3C_CLK_SRC_FOUTEPLL (1 << 2)
# define S3C_CLK_SRC_FOUTMPLL (1 << 1)
# define S3C_CLK_SRC_FOUTAPLL (1 << 0)
#define S3C_CLK_DIV0 (S3C_CLOCK_POWER_BASE + 0x020)
# define S3C_CLK_DIV0_GET_ADIV(x) ((x) & 0xf)
# define S3C_CLK_DIV0_GET_HCLK2(x) (((x) >> 9) & 0x7)
# define S3C_CLK_DIV0_GET_HCLK(x) (((x) >> 8) & 0x1)
# define S3C_CLK_DIV0_GET_PCLK(x) (((x) >> 12) & 0xf)
# define S3C_CLK_DIV0_SET_MPLL_DIV(x) (((x) & 0x1) << 4)
# define S3C_CLK_DIV0_GET_MPLL_DIV(x) (((x) >> 4) & 0x1)
# define S3C_CLK_DIV0_GET_MMC(x, v) (((v) >> (4 * x)) & 0xf)
# define S3C_CLK_DIV0_SET_MMC(x, v) (((v) & 0xf) << (4 * x))
#define S3C_CLK_DIV2 (S3C_CLOCK_POWER_BASE + 0x028)
# define S3C_CLK_DIV2_UART_MASK (0xf << 16)
# define S3C_CLK_DIV2_SET_UART(x) ((x) << 16)
# define S3C_CLK_DIV2_GET_UART(x) (((x) >> 16) & 0xf)
#define S3C_SCLK_GATE (S3C_CLOCK_POWER_BASE + 0x038)
# define S3C_SCLK_GATE_UART (1 << 5)
# define S3C_SCLK_GATE_MMC(x) (1 << (24 + x))
#define S3C_MISC_CON (S3C_CLOCK_POWER_BASE + 0x838)
# define S3C_MISC_CON_SYN667 (1 << 19)
#define S3C_OTHERS (S3C_CLOCK_POWER_BASE + 0x900)
# define S3C_OTHERS_CLK_SELECT (1 << 6)
