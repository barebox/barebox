#include <common.h>
#include <gpio.h>
#include <init.h>
#include <asm/hardware.h>
#include <mach/at91_pmc.h>

#include "clock.h"
#include "generic.h"

/* --------------------------------------------------------------------
 *  Clocks
 * -------------------------------------------------------------------- */

/*
 * The peripheral clocks.
 */
static struct clk pioA_clk = {
	.name		= "pioA_clk",
	.pmc_mask	= 1 << AT91SAM9263_ID_PIOA,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk pioB_clk = {
	.name		= "pioB_clk",
	.pmc_mask	= 1 << AT91SAM9263_ID_PIOB,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk pioCDE_clk = {
	.name		= "pioCDE_clk",
	.pmc_mask	= 1 << AT91SAM9263_ID_PIOCDE,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk usart0_clk = {
	.name		= "usart0_clk",
	.pmc_mask	= 1 << AT91SAM9263_ID_US0,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk usart1_clk = {
	.name		= "usart1_clk",
	.pmc_mask	= 1 << AT91SAM9263_ID_US1,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk usart2_clk = {
	.name		= "usart2_clk",
	.pmc_mask	= 1 << AT91SAM9263_ID_US2,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk mmc0_clk = {
	.name		= "mci0_clk",
	.pmc_mask	= 1 << AT91SAM9263_ID_MCI0,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk mmc1_clk = {
	.name		= "mci1_clk",
	.pmc_mask	= 1 << AT91SAM9263_ID_MCI1,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk can_clk = {
	.name		= "can_clk",
	.pmc_mask	= 1 << AT91SAM9263_ID_CAN,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk twi_clk = {
	.name		= "twi_clk",
	.pmc_mask	= 1 << AT91SAM9263_ID_TWI,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk spi0_clk = {
	.name		= "spi0_clk",
	.pmc_mask	= 1 << AT91SAM9263_ID_SPI0,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk spi1_clk = {
	.name		= "spi1_clk",
	.pmc_mask	= 1 << AT91SAM9263_ID_SPI1,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk ssc0_clk = {
	.name		= "ssc0_clk",
	.pmc_mask	= 1 << AT91SAM9263_ID_SSC0,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk ssc1_clk = {
	.name		= "ssc1_clk",
	.pmc_mask	= 1 << AT91SAM9263_ID_SSC1,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk ac97_clk = {
	.name		= "ac97_clk",
	.pmc_mask	= 1 << AT91SAM9263_ID_AC97C,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk tcb_clk = {
	.name		= "tcb_clk",
	.pmc_mask	= 1 << AT91SAM9263_ID_TCB,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk pwm_clk = {
	.name		= "pwm_clk",
	.pmc_mask	= 1 << AT91SAM9263_ID_PWMC,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk macb_clk = {
	.name		= "macb_clk",
	.pmc_mask	= 1 << AT91SAM9263_ID_EMAC,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk dma_clk = {
	.name		= "dma_clk",
	.pmc_mask	= 1 << AT91SAM9263_ID_DMA,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk twodge_clk = {
	.name		= "2dge_clk",
	.pmc_mask	= 1 << AT91SAM9263_ID_2DGE,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk udc_clk = {
	.name		= "udc_clk",
	.pmc_mask	= 1 << AT91SAM9263_ID_UDP,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk isi_clk = {
	.name		= "isi_clk",
	.pmc_mask	= 1 << AT91SAM9263_ID_ISI,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk lcdc_clk = {
	.name		= "lcdc_clk",
	.pmc_mask	= 1 << AT91SAM9263_ID_LCDC,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk ohci_clk = {
	.name		= "ohci_clk",
	.pmc_mask	= 1 << AT91SAM9263_ID_UHP,
	.type		= CLK_TYPE_PERIPHERAL,
};

static struct clk *periph_clocks[] = {
	&pioA_clk,
	&pioB_clk,
	&pioCDE_clk,
	&usart0_clk,
	&usart1_clk,
	&usart2_clk,
	&mmc0_clk,
	&mmc1_clk,
	&can_clk,
	&twi_clk,
	&spi0_clk,
	&spi1_clk,
	&ssc0_clk,
	&ssc1_clk,
	&ac97_clk,
	&tcb_clk,
	&pwm_clk,
	&macb_clk,
	&twodge_clk,
	&udc_clk,
	&isi_clk,
	&lcdc_clk,
	&dma_clk,
	&ohci_clk,
	// irq0 .. irq1
};

/*
 * The four programmable clocks.
 * You must configure pin multiplexing to bring these signals out.
 */
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
static struct clk pck3 = {
	.name		= "pck3",
	.pmc_mask	= AT91_PMC_PCK3,
	.type		= CLK_TYPE_PROGRAMMABLE,
	.id		= 3,
};

static void __init at91sam9263_register_clocks(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(periph_clocks); i++)
		clk_register(periph_clocks[i]);

	clk_register(&pck0);
	clk_register(&pck1);
	clk_register(&pck2);
	clk_register(&pck3);
}

/* --------------------------------------------------------------------
 *  GPIO
 * -------------------------------------------------------------------- */

static struct at91_gpio_bank at91sam9263_gpio[] = {
	{
		.id		= AT91SAM9263_ID_PIOA,
		.offset		= AT91_PIOA,
		.clock		= &pioA_clk,
	}, {
		.id		= AT91SAM9263_ID_PIOB,
		.offset		= AT91_PIOB,
		.clock		= &pioB_clk,
	}, {
		.id		= AT91SAM9263_ID_PIOCDE,
		.offset		= AT91_PIOC,
		.clock		= &pioCDE_clk,
	}, {
		.id		= AT91SAM9263_ID_PIOCDE,
		.offset		= AT91_PIOD,
		.clock		= &pioCDE_clk,
	}, {
		.id		= AT91SAM9263_ID_PIOCDE,
		.offset		= AT91_PIOE,
		.clock		= &pioCDE_clk,
	}
};

static int at91sam9263_initialize(void)
{
	/* Init clock subsystem */
	at91_clock_init(AT91_MAIN_CLOCK);

	/* Register the processor-specific clocks */
	at91sam9263_register_clocks();

	/* Register GPIO subsystem */
	at91_gpio_init(at91sam9263_gpio, 5);
	return 0;
}

core_initcall(at91sam9263_initialize);
