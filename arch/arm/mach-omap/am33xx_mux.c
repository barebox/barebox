/*
 * mux.c
 *
 * Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <common.h>
#include <config.h>
#include <asm/io.h>
#include <mach/am33xx-mux.h>
#include <mach/am33xx-silicon.h>

#define MUX_CFG(value, offset)	\
	__raw_writel(value, (AM33XX_CTRL_BASE + offset));

static const __maybe_unused struct module_pin_mux uart0_pin_mux[] = {
	{OFFSET(uart0_rxd), (MODE(0) | PULLUP_EN | RXACTIVE)},	/* UART0_RXD */
	{OFFSET(uart0_txd), (MODE(0) | PULLUDEN)},		/* UART0_TXD */
	{-1},
};

static const __maybe_unused struct module_pin_mux uart2_pin_mux[] = {
	{OFFSET(mii1_txclk), (MODE(1) | PULLUDEN | RXACTIVE)},	/* UART2_RXD */
	{OFFSET(mii1_rxclk), (MODE(1) | PULLUDEN)},		/* UART2_TXD */
	{-1},
};

static const __maybe_unused struct module_pin_mux uart3_pin_mux[] = {
	{OFFSET(spi0_cs1), (MODE(1) | PULLUDEN | RXACTIVE)},	/* UART3_RXD */
	{OFFSET(ecap0_in_pwm0_out), (MODE(1) | PULLUDEN)},	/* UART3_TXD */
	{-1},
};

static const __maybe_unused struct module_pin_mux nand_pin_mux[] = {
	{OFFSET(gpmc_ad0), (MODE(0) | PULLUP_EN | RXACTIVE)},	/* NAND AD0 */
	{OFFSET(gpmc_ad1), (MODE(0) | PULLUP_EN | RXACTIVE)},	/* NAND AD1 */
	{OFFSET(gpmc_ad2), (MODE(0) | PULLUP_EN | RXACTIVE)},	/* NAND AD2 */
	{OFFSET(gpmc_ad3), (MODE(0) | PULLUP_EN | RXACTIVE)},	/* NAND AD3 */
	{OFFSET(gpmc_ad4), (MODE(0) | PULLUP_EN | RXACTIVE)},	/* NAND AD4 */
	{OFFSET(gpmc_ad5), (MODE(0) | PULLUP_EN | RXACTIVE)},	/* NAND AD5 */
	{OFFSET(gpmc_ad6), (MODE(0) | PULLUP_EN | RXACTIVE)},	/* NAND AD6 */
	{OFFSET(gpmc_ad7), (MODE(0) | PULLUP_EN | RXACTIVE)},	/* NAND AD7 */
	{OFFSET(gpmc_wait0), (MODE(0) | RXACTIVE | PULLUP_EN)}, /* NAND WAIT */
	{OFFSET(gpmc_wpn), (MODE(7) | PULLUP_EN | RXACTIVE)},	/* NAND_WPN */
	{OFFSET(gpmc_csn0), (MODE(0) | PULLUDEN)},	/* NAND_CS0 */
	{OFFSET(gpmc_advn_ale), (MODE(0) | PULLUDEN)}, /* NAND_ADV_ALE */
	{OFFSET(gpmc_oen_ren), (MODE(0) | PULLUDEN)},	/* NAND_OE */
	{OFFSET(gpmc_wen), (MODE(0) | PULLUDEN)},	/* NAND_WEN */
	{OFFSET(gpmc_be0n_cle), (MODE(0) | PULLUDEN)},	/* NAND_BE_CLE */
	{-1},
};

static const __maybe_unused struct module_pin_mux i2c0_pin_mux[] = {
	{OFFSET(i2c0_sda), (MODE(0) | RXACTIVE | PULLUDEN | SLEWCTRL)},	/* I2C_DATA */
	{OFFSET(i2c0_scl), (MODE(0) | RXACTIVE | PULLUDEN | SLEWCTRL)},	/* I2C_SCLK */
	{-1},
};

static const __maybe_unused struct module_pin_mux i2c1_pin_mux[] = {
	{OFFSET(spi0_d1), (MODE(2) | RXACTIVE | PULLUDEN | SLEWCTRL)},	/* I2C_DATA */
	{OFFSET(spi0_cs0), (MODE(2) | RXACTIVE | PULLUDEN | SLEWCTRL)},	/* I2C_SCLK */
	{-1},
};

static const __maybe_unused struct module_pin_mux i2c2_pin_mux[] = {
	{OFFSET(uart1_ctsn), (MODE(3) | RXACTIVE | PULLUDEN | SLEWCTRL)},  /* I2C_DATA */
	{OFFSET(uart1_rtsn), (MODE(3) | RXACTIVE | PULLUDEN | SLEWCTRL)}, /* I2C_SCLK */
	{-1},
};

static const __maybe_unused struct module_pin_mux rgmii1_pin_mux[] = {
	{OFFSET(mii1_txen), MODE(2)},			/* RGMII1_TCTL */
	{OFFSET(mii1_rxdv), MODE(2) | RXACTIVE},	/* RGMII1_RCTL */
	{OFFSET(mii1_txd3), MODE(2)},			/* RGMII1_TD3 */
	{OFFSET(mii1_txd2), MODE(2)},			/* RGMII1_TD2 */
	{OFFSET(mii1_txd1), MODE(2)},			/* RGMII1_TD1 */
	{OFFSET(mii1_txd0), MODE(2)},			/* RGMII1_TD0 */
	{OFFSET(mii1_txclk), MODE(2)},			/* RGMII1_TCLK */
	{OFFSET(mii1_rxclk), MODE(2) | RXACTIVE},	/* RGMII1_RCLK */
	{OFFSET(mii1_rxd3), MODE(2) | RXACTIVE},	/* RGMII1_RD3 */
	{OFFSET(mii1_rxd2), MODE(2) | RXACTIVE},	/* RGMII1_RD2 */
	{OFFSET(mii1_rxd1), MODE(2) | RXACTIVE},	/* RGMII1_RD1 */
	{OFFSET(mii1_rxd0), MODE(2) | RXACTIVE},	/* RGMII1_RD0 */
	{OFFSET(mdio_data), MODE(0) | RXACTIVE | PULLUP_EN}, /* MDIO_DATA */
	{OFFSET(mdio_clk), MODE(0) | PULLUP_EN},	/* MDIO_CLK */
	{-1},
};

static const __maybe_unused struct module_pin_mux rgmii2_pin_mux[] = {
	{OFFSET(gpmc_a0), MODE(2)},			/* RGMII2_TCTL */
	{OFFSET(gpmc_a1), MODE(2) | RXACTIVE},		/* RGMII2_RCTL */
	{OFFSET(gpmc_a2), MODE(2)},			/* RGMII2_TD3 */
	{OFFSET(gpmc_a3), MODE(2)},			/* RGMII2_TD2 */
	{OFFSET(gpmc_a4), MODE(2)},			/* RGMII2_TD1 */
	{OFFSET(gpmc_a5), MODE(2)},			/* RGMII2_TD0 */
	{OFFSET(gpmc_a6), MODE(2)},			/* RGMII2_TCLK */
	{OFFSET(gpmc_a7), MODE(2) | RXACTIVE},		/* RGMII2_RCLK */
	{OFFSET(gpmc_a8), MODE(2) | RXACTIVE},		/* RGMII2_RD3 */
	{OFFSET(gpmc_a9), MODE(2) | RXACTIVE},		/* RGMII2_RD2 */
	{OFFSET(gpmc_a10), MODE(2) | RXACTIVE},		/* RGMII2_RD1 */
	{OFFSET(gpmc_a11), MODE(2) | RXACTIVE},		/* RGMII2_RD0 */
	{OFFSET(mdio_data), MODE(0) | RXACTIVE | PULLUP_EN}, /* MDIO_DATA */
	{OFFSET(mdio_clk), MODE(0) | PULLUP_EN},	/* MDIO_CLK */
	{-1},
};

static const __maybe_unused struct module_pin_mux mii1_pin_mux[] = {
	{OFFSET(mii1_rxerr), MODE(0) | RXACTIVE},	/* MII1_RXERR */
	{OFFSET(mii1_txen), MODE(0)},			/* MII1_TXEN */
	{OFFSET(mii1_rxdv), MODE(0) | RXACTIVE},	/* MII1_RXDV */
	{OFFSET(mii1_txd3), MODE(0)},			/* MII1_TXD3 */
	{OFFSET(mii1_txd2), MODE(0)},			/* MII1_TXD2 */
	{OFFSET(mii1_txd1), MODE(0)},			/* MII1_TXD1 */
	{OFFSET(mii1_txd0), MODE(0)},			/* MII1_TXD0 */
	{OFFSET(mii1_txclk), MODE(0) | RXACTIVE},	/* MII1_TXCLK */
	{OFFSET(mii1_rxclk), MODE(0) | RXACTIVE},	/* MII1_RXCLK */
	{OFFSET(mii1_rxd3), MODE(0) | RXACTIVE},	/* MII1_RXD3 */
	{OFFSET(mii1_rxd2), MODE(0) | RXACTIVE},	/* MII1_RXD2 */
	{OFFSET(mii1_rxd1), MODE(0) | RXACTIVE},	/* MII1_RXD1 */
	{OFFSET(mii1_rxd0), MODE(0) | RXACTIVE},	/* MII1_RXD0 */
	{OFFSET(mdio_data), MODE(0) | RXACTIVE | PULLUP_EN}, /* MDIO_DATA */
	{OFFSET(mdio_clk), MODE(0) | PULLUP_EN},	/* MDIO_CLK */
	{-1},
};

static const __maybe_unused struct module_pin_mux rmii1_pin_mux[] = {
	{OFFSET(mii1_crs), MODE(1) | RXACTIVE},     /* RMII1_CRS */
	{OFFSET(mii1_rxerr), MODE(1) | RXACTIVE},   /* RMII1_RXERR */
	{OFFSET(mii1_txen), MODE(1)},           /* RMII1_TXEN */
	{OFFSET(mii1_txd1), MODE(1)},           /* RMII1_TXD1 */
	{OFFSET(mii1_txd0), MODE(1)},           /* RMII1_TXD0 */
	{OFFSET(mii1_rxd1), MODE(1) | RXACTIVE},    /* RMII1_RXD1 */
	{OFFSET(mii1_rxd0), MODE(1) | RXACTIVE},    /* RMII1_RXD0 */
	{OFFSET(mdio_data), MODE(0) | RXACTIVE | PULLUP_EN}, /* MDIO_DATA */
	{OFFSET(mdio_clk), MODE(0) | PULLUP_EN},    /* MDIO_CLK */
	{OFFSET(rmii1_refclk), MODE(0) | RXACTIVE}, /* RMII1_REFCLK */
	{-1},
};

static const __maybe_unused struct module_pin_mux rmii2_pin_mux[] = {
	{OFFSET(gpmc_a0), MODE(3)},           /* RMII2_TXEN */
	{OFFSET(gpmc_a4), MODE(3)},           /* RMII2_TXD1 */
	{OFFSET(gpmc_a5), MODE(3)},           /* RMII2_TXD0 */
	{OFFSET(gpmc_a10), MODE(3) | RXACTIVE},    /* RMII2_RXD1 */
	{OFFSET(gpmc_a11), MODE(3) | RXACTIVE},    /* RMII2_RXD0 */
	{OFFSET(gpmc_wait0), MODE(3) | RXACTIVE},     /* RMII2_CRS */
	{OFFSET(gpmc_wpn), MODE(3) | RXACTIVE},   /* RMII2_RXERR */
	{OFFSET(mdio_data), MODE(0) | RXACTIVE | PULLUP_EN}, /* MDIO_DATA */
	{OFFSET(mdio_clk), MODE(0) | PULLUP_EN},    /* MDIO_CLK */
	{OFFSET(mii1_col), MODE(1) | RXACTIVE}, /* RMII2_REFCLK */
	{-1},
};

#ifdef CONFIG_NOR
static const __maybe_unused struct module_pin_mux nor_pin_mux[] = {
	{OFFSET(lcd_data0), MODE(1) | PULLUDEN},	/* NOR_A0 */
	{OFFSET(lcd_data1), MODE(1) | PULLUDEN},	/* NOR_A1 */
	{OFFSET(lcd_data2), MODE(1) | PULLUDEN},	/* NOR_A2 */
	{OFFSET(lcd_data3), MODE(1) | PULLUDEN},	/* NOR_A3 */
	{OFFSET(lcd_data4), MODE(1) | PULLUDEN},	/* NOR_A4 */
	{OFFSET(lcd_data5), MODE(1) | PULLUDEN},	/* NOR_A5 */
	{OFFSET(lcd_data6), MODE(1) | PULLUDEN},	/* NOR_A6 */
	{OFFSET(lcd_data7), MODE(1) | PULLUDEN},	/* NOR_A7 */
	{OFFSET(gpmc_a8), MODE(0)},			/* NOR_A8 */
	{OFFSET(gpmc_a9), MODE(0)},			/* NOR_A9 */
	{OFFSET(gpmc_a10), MODE(0)},			/* NOR_A10 */
	{OFFSET(gpmc_a11), MODE(0)},			/* NOR_A11 */
	{OFFSET(lcd_data8), MODE(1) | PULLUDEN},	/* NOR_A12 */
	{OFFSET(lcd_data9), MODE(1) | PULLUDEN},	/* NOR_A13 */
	{OFFSET(lcd_data10), MODE(1) | PULLUDEN},	/* NOR_A14 */
	{OFFSET(lcd_data11), MODE(1) | PULLUDEN},	/* NOR_A15 */
	{OFFSET(lcd_data12), MODE(1) | PULLUDEN},	/* NOR_A16 */
	{OFFSET(lcd_data13), MODE(1) | PULLUDEN},	/* NOR_A17 */
	{OFFSET(lcd_data14), MODE(1) | PULLUDEN},	/* NOR_A18 */
	{OFFSET(lcd_data15), MODE(1) | PULLUDEN},	/* NOR_A19 */
	{OFFSET(gpmc_a4), MODE(4)},			/* NOR_A20 */
	{OFFSET(gpmc_a5), MODE(4)},			/* NOR_A21 */
	{OFFSET(gpmc_a6), MODE(4)},			/* NOR_A22 */
	{OFFSET(gpmc_ad0), MODE(0) | RXACTIVE},		/* NOR_AD0 */
	{OFFSET(gpmc_ad1), MODE(0) | RXACTIVE},		/* NOR_AD1 */
	{OFFSET(gpmc_ad2), MODE(0) | RXACTIVE},		/* NOR_AD2 */
	{OFFSET(gpmc_ad3), MODE(0) | RXACTIVE},		/* NOR_AD3 */
	{OFFSET(gpmc_ad4), MODE(0) | RXACTIVE},		/* NOR_AD4 */
	{OFFSET(gpmc_ad5), MODE(0) | RXACTIVE},		/* NOR_AD5 */
	{OFFSET(gpmc_ad6), MODE(0) | RXACTIVE},		/* NOR_AD6 */
	{OFFSET(gpmc_ad7), MODE(0) | RXACTIVE},		/* NOR_AD7 */
	{OFFSET(gpmc_ad8), MODE(0) | RXACTIVE},		/* NOR_AD8 */
	{OFFSET(gpmc_ad9), MODE(0) | RXACTIVE},		/* NOR_AD9 */
	{OFFSET(gpmc_ad10), MODE(0) | RXACTIVE},	/* NOR_AD10 */
	{OFFSET(gpmc_ad11), MODE(0) | RXACTIVE},	/* NOR_AD11 */
	{OFFSET(gpmc_ad12), MODE(0) | RXACTIVE},	/* NOR_AD12 */
	{OFFSET(gpmc_ad13), MODE(0) | RXACTIVE},	/* NOR_AD13 */
	{OFFSET(gpmc_ad14), MODE(0) | RXACTIVE},	/* NOR_AD14 */
	{OFFSET(gpmc_ad15), MODE(0) | RXACTIVE},	/* NOR_AD15 */
	{OFFSET(gpmc_csn0), (MODE(0) | PULLUP_EN)},	/* NOR_CE */
	{OFFSET(gpmc_oen_ren), (MODE(0) | PULLUP_EN)},	/* NOR_OE */
	{OFFSET(gpmc_wen), (MODE(0) | PULLUP_EN)},	/* NOR_WEN */
	{OFFSET(gpmc_wait0), (MODE(0) | RXACTIVE | PULLUP_EN)}, /* NOR WAIT */
	{OFFSET(lcd_ac_bias_en), MODE(7) | RXACTIVE | PULLUDEN}, /* NOR RESET */
	{-1},
};
#endif

static const __maybe_unused struct module_pin_mux mmc0_pin_mux[] = {
	{OFFSET(mmc0_dat3), (MODE(0) | RXACTIVE | PULLUP_EN)},	/* MMC0_DAT3 */
	{OFFSET(mmc0_dat2), (MODE(0) | RXACTIVE | PULLUP_EN)},	/* MMC0_DAT2 */
	{OFFSET(mmc0_dat1), (MODE(0) | RXACTIVE | PULLUP_EN)},	/* MMC0_DAT1 */
	{OFFSET(mmc0_dat0), (MODE(0) | RXACTIVE | PULLUP_EN)},	/* MMC0_DAT0 */
	{OFFSET(mmc0_clk), (MODE(0) | RXACTIVE | PULLUP_EN)},	/* MMC0_CLK */
	{OFFSET(mmc0_cmd), (MODE(0) | RXACTIVE | PULLUP_EN)},	/* MMC0_CMD */
	{OFFSET(mcasp0_aclkr), (MODE(4) | RXACTIVE)},		/* MMC0_WP */
	{OFFSET(spi0_cs1), (MODE(5) | RXACTIVE | PULLUP_EN)},	/* MMC0_CD */
	{-1},
};

static const __maybe_unused struct module_pin_mux mmc1_pin_mux[] = {
	{OFFSET(gpmc_ad3), (MODE(1) | RXACTIVE)},	/* MMC1_DAT3 */
	{OFFSET(gpmc_ad2), (MODE(1) | RXACTIVE)},	/* MMC1_DAT2 */
	{OFFSET(gpmc_ad1), (MODE(1) | RXACTIVE)},	/* MMC1_DAT1 */
	{OFFSET(gpmc_ad0), (MODE(1) | RXACTIVE)},	/* MMC1_DAT0 */
	{OFFSET(gpmc_csn1), (MODE(2) | RXACTIVE | PULLUP_EN)},	/* MMC1_CLK */
	{OFFSET(gpmc_csn2), (MODE(2) | RXACTIVE | PULLUP_EN)},	/* MMC1_CMD */
	{OFFSET(uart1_rxd), (MODE(1) | RXACTIVE | PULLUP_EN)},	/* MMC1_WP */
	{OFFSET(mcasp0_fsx), (MODE(4) | RXACTIVE)},	/* MMC1_CD */
	{-1},
};

static const __maybe_unused struct module_pin_mux spi0_pin_mux[] = {
	{OFFSET(spi0_sclk), MODE(0) | PULLUDEN | RXACTIVE},	/*SPI0_SCLK */
	{OFFSET(spi0_d0), MODE(0) | PULLUDEN | PULLUP_EN |
							RXACTIVE}, /*SPI0_D0 */
	{OFFSET(spi0_d1), MODE(0) | PULLUDEN |
							RXACTIVE}, /*SPI0_D1 */
	{OFFSET(spi0_cs0), MODE(0) | PULLUDEN | PULLUP_EN | RXACTIVE},	/*SPI0_CS0 */
	{-1},
};

static const __maybe_unused struct module_pin_mux spi1_pin_mux[] = {
	{OFFSET(mcasp0_aclkx), MODE(3) | PULLUDEN | RXACTIVE},	/*SPI0_SCLK */
	{OFFSET(mcasp0_fsx), MODE(3) | PULLUDEN | PULLUP_EN |
							RXACTIVE}, /*SPI0_D0 */
	{OFFSET(mcasp0_axr0), MODE(3) | PULLUDEN | RXACTIVE}, /*SPI0_D1 */
	{OFFSET(mcasp0_ahclkr), MODE(3) | PULLUDEN | PULLUP_EN |
							RXACTIVE}, /*SPI0_CS0 */
	{-1},
};

/*
 * Configure the pin mux for the module
 */
void configure_module_pin_mux(const struct module_pin_mux *mod_pin_mux)
{
	int i;

	if (!mod_pin_mux)
		return;

	for (i = 0; mod_pin_mux[i].reg_offset != -1; i++)
		MUX_CFG(mod_pin_mux[i].val, mod_pin_mux[i].reg_offset);
}

void am33xx_enable_mii1_pin_mux(void)
{
	configure_module_pin_mux(mii1_pin_mux);
}

void am33xx_enable_rmii1_pin_mux(void)
{
	configure_module_pin_mux(rmii1_pin_mux);
}

void am33xx_enable_rmii2_pin_mux(void)
{
	configure_module_pin_mux(rmii2_pin_mux);
}

void am33xx_enable_i2c0_pin_mux(void)
{
	configure_module_pin_mux(i2c0_pin_mux);
}

void am33xx_enable_i2c1_pin_mux(void)
{
	configure_module_pin_mux(i2c1_pin_mux);
}

void am33xx_enable_i2c2_pin_mux(void)
{
	configure_module_pin_mux(i2c2_pin_mux);
}

void am33xx_enable_uart0_pin_mux(void)
{
	configure_module_pin_mux(uart0_pin_mux);
}

void am33xx_enable_uart2_pin_mux(void)
{
	configure_module_pin_mux(uart2_pin_mux);
}

void am33xx_enable_mmc0_pin_mux(void)
{
	configure_module_pin_mux(mmc0_pin_mux);
}

void am33xx_enable_spi0_pin_mux(void)
{
	configure_module_pin_mux(spi0_pin_mux);
}

void am33xx_enable_nand_pin_mux(void)
{
	configure_module_pin_mux(nand_pin_mux);
}
