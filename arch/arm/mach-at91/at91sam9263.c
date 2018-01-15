#include <common.h>
#include <gpio.h>
#include <init.h>
#include <mach/hardware.h>
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

static struct clk_lookup periph_clocks_lookups[] = {
	CLKDEV_CON_DEV_ID("mci_clk", "atmel_mci0", &mmc0_clk),
	CLKDEV_CON_DEV_ID("mci_clk", "atmel_mci1", &mmc1_clk),
	CLKDEV_CON_DEV_ID("spi_clk", "atmel_spi0", &spi0_clk),
	CLKDEV_CON_DEV_ID("spi_clk", "atmel_spi1", &spi1_clk),
	CLKDEV_DEV_ID("at91rm9200-gpio0", &pioA_clk),
	CLKDEV_DEV_ID("at91rm9200-gpio1", &pioB_clk),
	CLKDEV_DEV_ID("at91rm9200-gpio2", &pioCDE_clk),
	CLKDEV_DEV_ID("at91rm9200-gpio3", &pioCDE_clk),
	CLKDEV_DEV_ID("at91rm9200-gpio4", &pioCDE_clk),
	CLKDEV_DEV_ID("at91-pit", &mck),
	CLKDEV_CON_DEV_ID("hck1", "atmel_lcdfb", &lcdc_clk),
};

static struct clk_lookup usart_clocks_lookups[] = {
	CLKDEV_CON_DEV_ID("usart", "atmel_usart0", &mck),
	CLKDEV_CON_DEV_ID("usart", "atmel_usart1", &usart0_clk),
	CLKDEV_CON_DEV_ID("usart", "atmel_usart2", &usart1_clk),
	CLKDEV_CON_DEV_ID("usart", "atmel_usart3", &usart2_clk),
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

	clkdev_add_table(periph_clocks_lookups,
			 ARRAY_SIZE(periph_clocks_lookups));
	clkdev_add_table(usart_clocks_lookups,
			 ARRAY_SIZE(usart_clocks_lookups));

	clk_register(&pck0);
	clk_register(&pck1);
	clk_register(&pck2);
	clk_register(&pck3);
}

static void at91sam9263_initialize(void)
{
	/* Register the processor-specific clocks */
	at91sam9263_register_clocks();

	/* Register GPIO subsystem */
	at91_add_rm9200_gpio(0, AT91SAM9263_BASE_PIOA);
	at91_add_rm9200_gpio(1, AT91SAM9263_BASE_PIOB);
	at91_add_rm9200_gpio(2, AT91SAM9263_BASE_PIOC);
	at91_add_rm9200_gpio(3, AT91SAM9263_BASE_PIOD);
	at91_add_rm9200_gpio(4, AT91SAM9263_BASE_PIOE);

	at91_add_pit(AT91SAM9263_BASE_PIT);
	at91_add_sam9_smc(0, AT91SAM9263_BASE_SMC0, 0x200);
	at91_add_sam9_smc(1, AT91SAM9263_BASE_SMC1, 0x200);
}

static int at91sam9263_setup(void)
{
	at91_boot_soc = at91sam9263_initialize;
	return 0;
}
pure_initcall(at91sam9263_setup);
