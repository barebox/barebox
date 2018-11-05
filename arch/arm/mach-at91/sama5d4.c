/*
 * Chip-specific setup code for the SAMA5D4 family
 *
 * Copyright (C) 2014 Atmel Corporation,
 *		      Bo Shen <voice.shen@atmel.com>
 *
 * Licensed under GPLv2 or later.
 */

#include <common.h>
#include <gpio.h>
#include <init.h>
#include <restart.h>
#include <mach/hardware.h>
#include <mach/at91_pmc.h>
#include <mach/cpu.h>
#include <mach/board.h>
#include <mach/at91_rstc.h>
#include <linux/clk.h>

#include "generic.h"
#include "clock.h"

/*
 * The peripheral clocks.
 */
static struct clk pit_clk = {
	.name		= "pit_clk",
	.pid		= SAMA5D4_ID_PIT,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk smc_clk = {
	.name		= "smc_clk",
	.pid		= SAMA5D4_ID_HSMC,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk pioA_clk = {
	.name		= "pioA_clk",
	.pid		= SAMA5D4_ID_PIOA,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk pioB_clk = {
	.name		= "pioB_clk",
	.pid		= SAMA5D4_ID_PIOB,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk pioC_clk = {
	.name		= "pioC_clk",
	.pid		= SAMA5D4_ID_PIOC,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk pioD_clk = {
	.name		= "pioD_clk",
	.pid		= SAMA5D4_ID_PIOD,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk pioE_clk = {
	.name		= "pioE_clk",
	.pid		= SAMA5D4_ID_PIOE,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk usart0_clk = {
	.name		= "usart0_clk",
	.pid		= SAMA5D4_ID_USART0,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk usart2_clk = {
	.name		= "usart2_clk",
	.pid		= SAMA5D4_ID_USART2,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk usart3_clk = {
	.name		= "usart3_clk",
	.pid		= SAMA5D4_ID_USART3,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk usart4_clk = {
	.name		= "usart4_clk",
	.pid		= SAMA5D4_ID_USART4,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk mmc0_clk = {
	.name		= "mci0_clk",
	.pid		= SAMA5D4_ID_HSMCI0,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk mmc1_clk = {
	.name		= "mci1_clk",
	.pid		= SAMA5D4_ID_HSMCI1,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk tcb0_clk = {
	.name		= "tcb0_clk",
	.pid		= SAMA5D4_ID_TC0,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk tcb1_clk = {
	.name		= "tcb1_clk",
	.pid		= SAMA5D4_ID_TC1,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk adc_clk = {
	.name		= "adc_clk",
	.pid		= SAMA5D4_ID_ADC,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk dma0_clk = {
	.name		= "dma0_clk",
	.pid		= SAMA5D4_ID_DMA0,
	.type		= CLK_TYPE_PERIPHERAL | CLK_TYPE_PERIPH_H64MX,
};
static struct clk dma1_clk = {
	.name		= "dma1_clk",
	.pid		= SAMA5D4_ID_DMA1,
	.type		= CLK_TYPE_PERIPHERAL | CLK_TYPE_PERIPH_H64MX,
};
static struct clk uhphs_clk = {
	.name		= "uhphs",
	.pid		= SAMA5D4_ID_UHPHS,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk udphs_clk = {
	.name		= "udphs_clk",
	.pid		= SAMA5D4_ID_UDPHS,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk lcdc_clk = {
	.name		= "lcdc_clk",
	.pid		= SAMA5D4_ID_LCDC,
	.type		= CLK_TYPE_PERIPHERAL | CLK_TYPE_PERIPH_H64MX,
};
static struct clk isi_clk = {
	.name		= "isi_clk",
	.pid		= SAMA5D4_ID_ISI,
	.type		= CLK_TYPE_PERIPHERAL | CLK_TYPE_PERIPH_H64MX,
};
static struct clk macb0_clk = {
	.name		= "macb0_clk",
	.pid		= SAMA5D4_ID_GMAC0,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk twi0_clk = {
	.name		= "twi0_clk",
	.pid		= SAMA5D4_ID_TWI0,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk twi2_clk = {
	.name		= "twi2_clk",
	.pid		= SAMA5D4_ID_TWI2,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk spi0_clk = {
	.name		= "spi0_clk",
	.pid		= SAMA5D4_ID_SPI0,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk smd_clk = {
	.name		= "smd_clk",
	.pid		= SAMA5D4_ID_SMD,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk ssc0_clk = {
	.name		= "ssc0_clk",
	.pid		= SAMA5D4_ID_SSC0,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk ssc1_clk = {
	.name		= "ssc1_clk",
	.pid		= SAMA5D4_ID_SSC1,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk sha_clk = {
	.name		= "sha_clk",
	.pid		= SAMA5D4_ID_SHA,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk aes_clk = {
	.name		= "aes_clk",
	.pid		= SAMA5D4_ID_AES,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk tdes_clk = {
	.name		= "tdes_clk",
	.pid		= SAMA5D4_ID_TDES,
	.type		= CLK_TYPE_PERIPHERAL,
};

static struct clk *periph_clocks[] __initdata = {
	&pit_clk,
	&smc_clk,
	&pioA_clk,
	&pioB_clk,
	&pioC_clk,
	&pioD_clk,
	&pioE_clk,
	&usart0_clk,
	&usart2_clk,
	&usart3_clk,
	&usart4_clk,
	&mmc0_clk,
	&mmc1_clk,
	&tcb0_clk,
	&tcb1_clk,
	&adc_clk,
	&dma0_clk,
	&dma1_clk,
	&uhphs_clk,
	&udphs_clk,
	&lcdc_clk,
	&isi_clk,
	&macb0_clk,
	&twi0_clk,
	&twi2_clk,
	&spi0_clk,
	&smd_clk,
	&ssc0_clk,
	&ssc1_clk,
	&sha_clk,
	&aes_clk,
	&tdes_clk,
};

static struct clk pck0 = {
	.name		= "pck0",
	.pmc_mask	= AT91_PMC_PCK0,
	.type		= CLK_TYPE_PROGRAMMABLE,
	.id		= 0,
};

static struct clk pck1 = {
	.name		= "pck1",
	.pmc_mask	= AT91_PMC_PCK1,
	.type		= CLK_TYPE_PROGRAMMABLE,
	.id		= 1,
};

static struct clk pck2 = {
	.name		= "pck2",
	.pmc_mask	= AT91_PMC_PCK2,
	.type		= CLK_TYPE_PROGRAMMABLE,
	.id		= 2,
};

static struct clk_lookup periph_clocks_lookups[] = {
	CLKDEV_CON_DEV_ID("macb_clk", "macb0", &macb0_clk),
	CLKDEV_CON_DEV_ID("mci_clk", "atmel_mci0", &mmc0_clk),
	CLKDEV_CON_DEV_ID("mci_clk", "atmel_mci1", &mmc1_clk),
	CLKDEV_CON_DEV_ID("spi_clk", "atmel_spi0", &spi0_clk),
	CLKDEV_DEV_ID("at91-pit", &pit_clk),
	CLKDEV_CON_DEV_ID("hck1", "atmel_hlcdfb", &lcdc_clk),
};

static struct clk_lookup usart_clocks_lookups[] = {
	CLKDEV_CON_DEV_ID("usart", "atmel_usart0", &mck),
	CLKDEV_CON_DEV_ID("usart", "atmel_usart1", &usart0_clk),
	CLKDEV_CON_DEV_ID("usart", "atmel_usart3", &usart2_clk),
	CLKDEV_CON_DEV_ID("usart", "atmel_usart4", &usart3_clk),
};

static void __init sama5d4_register_clocks(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(periph_clocks); i++)
		clk_register(periph_clocks[i]);

	clkdev_add_physbase(&pioA_clk, SAMA5D4_BASE_PIOA, 0);
	clkdev_add_physbase(&pioB_clk, SAMA5D4_BASE_PIOB, 0);
	clkdev_add_physbase(&pioC_clk, SAMA5D4_BASE_PIOC, 0);
	clkdev_add_physbase(&pioD_clk, SAMA5D4_BASE_PIOD, 0);
	clkdev_add_physbase(&pioE_clk, SAMA5D4_BASE_PIOE, 0);

	clkdev_add_table(periph_clocks_lookups,
			 ARRAY_SIZE(periph_clocks_lookups));

	clkdev_add_table(usart_clocks_lookups,
			 ARRAY_SIZE(usart_clocks_lookups));

	clk_register(&pck0);
	clk_register(&pck1);
	clk_register(&pck2);
}

static void sama5d4_restart(struct restart_handler *rst)
{
	at91sam9g45_reset(IOMEM(SAMA5D4_BASE_MPDDRC),
			  IOMEM(SAMA5D4_BASE_RSTC + AT91_RSTC_CR));
}

/* --------------------------------------------------------------------
 *  Processor initialization
 * -------------------------------------------------------------------- */
static void sama5d4_initialize(void)
{
	/* Register the processor-specific clocks */
	sama5d4_register_clocks();

	/* Register GPIO subsystem */
	at91_add_sam9x5_gpio(0, SAMA5D4_BASE_PIOA);
	at91_add_sam9x5_gpio(1, SAMA5D4_BASE_PIOB);
	at91_add_sam9x5_gpio(2, SAMA5D4_BASE_PIOC);
	at91_add_sam9x5_gpio(3, SAMA5D4_BASE_PIOD);
	at91_add_sam9x5_gpio(4, SAMA5D4_BASE_PIOE);

	at91_add_pit(SAMA5D4_BASE_PIT);
	at91_add_sam9_smc(DEVICE_ID_SINGLE, SAMA5D4_BASE_HSMC + 0x600, 0xa0);

	restart_handler_register_fn(sama5d4_restart);
}

static int sama5d4_setup(void)
{
	at91_boot_soc = sama5d4_initialize;
	return 0;
}
pure_initcall(sama5d4_setup);
