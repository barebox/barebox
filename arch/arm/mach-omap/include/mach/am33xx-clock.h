/*
 * (C) Copyright 2012 Teresa Gámez, Phytec Messtechnik GmbH
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
#ifndef _AM33XX_CLOCKS_H_
#define _AM33XX_CLOCKS_H_

#include "am33xx-silicon.h"

/* Put the pll config values over here */

/* MAIN PLL Fdll = 1 GHZ, */
#define MPUPLL_M_500	500	/* 125 * n */
#define MPUPLL_M_550	550	/* 125 * n */
#define MPUPLL_M_600	600	/* 125 * n */
#define MPUPLL_M_720	720	/* 125 * n */

#define MPUPLL_M2	1

/* Core PLL Fdll = 1 GHZ, */
#define COREPLL_M	1000	/* 125 * n */

#define COREPLL_M4	10	/* CORE_CLKOUTM4 = 200 MHZ */
#define COREPLL_M5	8	/* CORE_CLKOUTM5 = 250 MHZ */
#define COREPLL_M6	4	/* CORE_CLKOUTM6 = 500 MHZ */

/*
 * USB PHY clock is 960 MHZ. Since, this comes directly from Fdll, Fdll
 * frequency needs to be set to 960 MHZ. Hence,
 * For clkout = 192 MHZ, Fdll = 960 MHZ, divider values are given below
 */
#define PERPLL_M	960
#define PERPLL_M2	5

/* DDR Freq is 266 MHZ for now*/
/* Set Fdll = 400 MHZ , Fdll = M * 2 * CLKINP/ N + 1; clkout = Fdll /(2 * M2) */
#define DDRPLL_M_266	266
#define DDRPLL_M_303	303
#define DDRPLL_M_400	400
#define DDRPLL_N	(OSC - 1)
#define DDRPLL_M2	1

/* PRCM */
/* Module Offsets */
#define CM_PER                          (AM33XX_PRM_BASE + 0x0)
#define CM_WKUP                         (AM33XX_PRM_BASE + 0x400)
#define CM_DPLL                         (AM33XX_PRM_BASE + 0x500)
#define CM_DEVICE                       (AM33XX_PRM_BASE + 0x0700)
#define CM_CEFUSE                       (AM33XX_PRM_BASE + 0x0A00)
#define PRM_DEVICE                      (AM33XX_PRM_BASE + 0x0F00)
/* Register Offsets */
/* Core PLL ADPLLS */
#define CM_CLKSEL_DPLL_CORE             (CM_WKUP + 0x68)
#define CM_CLKMODE_DPLL_CORE            (CM_WKUP + 0x90)

/* Core HSDIV */
#define CM_DIV_M4_DPLL_CORE             (CM_WKUP + 0x80)
#define CM_DIV_M5_DPLL_CORE             (CM_WKUP + 0x84)
#define CM_DIV_M6_DPLL_CORE             (CM_WKUP + 0xD8)
#define CM_IDLEST_DPLL_CORE             (CM_WKUP + 0x5c)

/* Peripheral PLL */
#define CM_CLKSEL_DPLL_PER              (CM_WKUP + 0x9c)
#define CM_CLKMODE_DPLL_PER             (CM_WKUP + 0x8c)
#define CM_DIV_M2_DPLL_PER              (CM_WKUP + 0xAC)
#define CM_IDLEST_DPLL_PER              (CM_WKUP + 0x70)

/* Display PLL */
#define CM_CLKSEL_DPLL_DISP             (CM_WKUP + 0x54)
#define CM_CLKMODE_DPLL_DISP            (CM_WKUP + 0x98)
#define CM_DIV_M2_DPLL_DISP             (CM_WKUP + 0xA4)

/* DDR PLL */
#define CM_CLKSEL_DPLL_DDR              (CM_WKUP + 0x40)
#define CM_CLKMODE_DPLL_DDR             (CM_WKUP + 0x94)
#define CM_DIV_M2_DPLL_DDR              (CM_WKUP + 0xA0)
#define CM_IDLEST_DPLL_DDR              (CM_WKUP + 0x34)

/* MPU PLL */
#define CM_CLKSEL_DPLL_MPU              (CM_WKUP + 0x2c)
#define CM_CLKMODE_DPLL_MPU             (CM_WKUP + 0x88)
#define CM_DIV_M2_DPLL_MPU              (CM_WKUP + 0xA8)
#define CM_IDLEST_DPLL_MPU              (CM_WKUP + 0x20)

/* TIMER Clock Source Select */
#define CLKSEL_TIMER2_CLK               (CM_DPLL + 0x8)

/* Interconnect clocks */
#define CM_PER_L4LS_CLKCTRL             (CM_PER + 0x60) /* EMIF */
#define CM_PER_L4FW_CLKCTRL             (CM_PER + 0x64) /* EMIF FW */
#define CM_PER_L3_CLKCTRL               (CM_PER + 0xE0) /* OCMC RAM */
#define CM_PER_L3_INSTR_CLKCTRL         (CM_PER + 0xDC)
#define CM_PER_L4HS_CLKCTRL             (CM_PER + 0x120)
#define CM_WKUP_L4WKUP_CLKCTRL          (CM_WKUP + 0x0c)/* UART0 */

/* Domain Wake UP */
#define CM_WKUP_CLKSTCTRL               (CM_WKUP + 0)   /* UART0 */
#define CM_PER_L4LS_CLKSTCTRL           (CM_PER + 0x0)  /* TIMER2 */
#define CM_PER_L3_CLKSTCTRL             (CM_PER + 0x0c) /* EMIF */
#define CM_PER_L4FW_CLKSTCTRL           (CM_PER + 0x08) /* EMIF FW */
#define CM_PER_L3S_CLKSTCTRL            (CM_PER + 0x4)
#define CM_PER_L4HS_CLKSTCTRL           (CM_PER + 0x011c)
#define CM_CEFUSE_CLKSTCTRL             (CM_CEFUSE + 0x0)

/* Module Enable Registers */
#define CM_PER_TIMER2_CLKCTRL           (CM_PER + 0x80) /* Timer2 */
#define CM_WKUP_UART0_CLKCTRL           (CM_WKUP + 0xB4)/* UART0 */
#define CM_WKUP_CONTROL_CLKCTRL         (CM_WKUP + 0x4) /* Control Module */
#define CM_PER_EMIF_CLKCTRL             (CM_PER + 0x28) /* EMIF */
#define CM_PER_EMIF_FW_CLKCTRL          (CM_PER + 0xD0) /* EMIF FW */
#define CM_PER_GPMC_CLKCTRL             (CM_PER + 0x30) /* GPMC */
#define CM_PER_ELM_CLKCTRL              (CM_PER + 0x40) /* ELM */
#define CM_PER_SPI0_CLKCTRL             (CM_PER + 0x4c) /* SPI0 */
#define CM_PER_SPI1_CLKCTRL             (CM_PER + 0x50) /* SPI1 */
#define CM_WKUP_I2C0_CLKCTRL            (CM_WKUP + 0xB8) /* I2C0 */
#define CM_PER_CPGMAC0_CLKCTRL          (CM_PER + 0x14) /* Ethernet */
#define CM_PER_CPSW_CLKSTCTRL           (CM_PER + 0x144)/* Ethernet */
#define CM_PER_OCMCRAM_CLKCTRL          (CM_PER + 0x2C) /* OCMC RAM */
#define CM_PER_GPIO1_CLKCTRL            (CM_PER + 0xAC) /* GPIO1 */
#define CM_PER_GPIO2_CLKCTRL            (CM_PER + 0xB0) /* GPIO2 */
#define CM_PER_GPIO3_CLKCTRL            (CM_PER + 0xB4) /* GPIO3 */
#define CM_PER_UART1_CLKCTRL            (CM_PER + 0x6C) /* UART1 */
#define CM_PER_UART2_CLKCTRL            (CM_PER + 0x70) /* UART2 */
#define CM_PER_UART3_CLKCTRL            (CM_PER + 0x74) /* UART3 */
#define CM_PER_UART4_CLKCTRL            (CM_PER + 0x78) /* UART4 */
#define CM_PER_I2C1_CLKCTRL             (CM_PER + 0x48) /* I2C1 */
#define CM_PER_I2C2_CLKCTRL             (CM_PER + 0x44) /* I2C2 */
#define CM_WKUP_GPIO0_CLKCTRL           (CM_WKUP + 0x8) /* GPIO0 */

#define CM_PER_MMC0_CLKCTRL             (CM_PER + 0x3C)
#define CM_PER_MMC1_CLKCTRL             (CM_PER + 0xF4)
#define CM_PER_MMC2_CLKCTRL             (CM_PER + 0xF8)

/* PRCM */
#define CM_DPLL_OFFSET                  (AM33XX_PRM_BASE + 0x0300)

#define CM_ALWON_WDTIMER_CLKCTRL        (AM33XX_PRM_BASE + 0x158C)
#define CM_ALWON_SPI_CLKCTRL            (AM33XX_PRM_BASE + 0x1590)
#define CM_ALWON_CONTROL_CLKCTRL        (AM33XX_PRM_BASE + 0x15C4)

#define CM_ALWON_L3_SLOW_CLKSTCTRL      (AM33XX_PRM_BASE + 0x1400)

#define CM_ALWON_GPIO_0_CLKCTRL         (AM33XX_PRM_BASE + 0x155c)
#define CM_ALWON_GPIO_0_OPTFCLKEN_DBCLK (AM33XX_PRM_BASE + 0x155c)

/* Ethernet */
#define CM_ETHERNET_CLKSTCTRL           (AM33XX_PRM_BASE + 0x1404)
#define CM_ALWON_ETHERNET_0_CLKCTRL     (AM33XX_PRM_BASE + 0x15D4)
#define CM_ALWON_ETHERNET_1_CLKCTRL     (AM33XX_PRM_BASE + 0x15D8)

/* UARTs */
#define CM_ALWON_UART_0_CLKCTRL         (AM33XX_PRM_BASE + 0x1550)
#define CM_ALWON_UART_1_CLKCTRL         (AM33XX_PRM_BASE + 0x1554)
#define CM_ALWON_UART_2_CLKCTRL         (AM33XX_PRM_BASE + 0x1558)

/* I2C */
/* Note: In ti814x I2C0 and I2C2 have common clk control */
#define CM_ALWON_I2C_0_CLKCTRL          (AM33XX_PRM_BASE + 0x1564)

/* EMIF4 PRCM Defintion */
#define CM_DEFAULT_L3_FAST_CLKSTCTRL    (AM33XX_PRM_BASE + 0x0508)
#define CM_DEFAULT_EMIF_0_CLKCTRL       (AM33XX_PRM_BASE + 0x0520)
#define CM_DEFAULT_EMIF_1_CLKCTRL       (AM33XX_PRM_BASE + 0x0524)
#define CM_DEFAULT_DMM_CLKCTRL          (AM33XX_PRM_BASE + 0x0528)
#define CM_DEFAULT_FW_CLKCTRL           (AM33XX_PRM_BASE + 0x052C)

/* ALWON PRCM */
#define CM_ALWON_OCMC_0_CLKSTCTRL       CM_PER_L3_CLKSTCTRL
#define CM_ALWON_OCMC_0_CLKCTRL         CM_PER_OCMCRAM_CLKCTRL

#define CM_ALWON_GPMC_CLKCTRL           CM_PER_GPMC_CLKCTRL

extern void pll_init(int mpupll_M, int osc, int ddrpll_M);
extern void enable_ddr_clocks(void);

#endif  /* endif _AM33XX_CLOCKS_H_ */
