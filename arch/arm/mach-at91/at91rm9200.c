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
static struct clk udc_clk = {
	.name		= "udc_clk",
	.pmc_mask	= 1 << AT91RM9200_ID_UDP,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk ohci_clk = {
	.name		= "ohci_clk",
	.pmc_mask	= 1 << AT91RM9200_ID_UHP,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk ether_clk = {
	.name		= "ether_clk",
	.pmc_mask	= 1 << AT91RM9200_ID_EMAC,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk mmc_clk = {
	.name		= "mci_clk",
	.pmc_mask	= 1 << AT91RM9200_ID_MCI,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk twi_clk = {
	.name		= "twi_clk",
	.pmc_mask	= 1 << AT91RM9200_ID_TWI,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk usart0_clk = {
	.name		= "usart0_clk",
	.pmc_mask	= 1 << AT91RM9200_ID_US0,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk usart1_clk = {
	.name		= "usart1_clk",
	.pmc_mask	= 1 << AT91RM9200_ID_US1,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk usart2_clk = {
	.name		= "usart2_clk",
	.pmc_mask	= 1 << AT91RM9200_ID_US2,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk usart3_clk = {
	.name		= "usart3_clk",
	.pmc_mask	= 1 << AT91RM9200_ID_US3,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk spi_clk = {
	.name		= "spi_clk",
	.pmc_mask	= 1 << AT91RM9200_ID_SPI,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk pioA_clk = {
	.name		= "pioA_clk",
	.pmc_mask	= 1 << AT91RM9200_ID_PIOA,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk pioB_clk = {
	.name		= "pioB_clk",
	.pmc_mask	= 1 << AT91RM9200_ID_PIOB,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk pioC_clk = {
	.name		= "pioC_clk",
	.pmc_mask	= 1 << AT91RM9200_ID_PIOC,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk pioD_clk = {
	.name		= "pioD_clk",
	.pmc_mask	= 1 << AT91RM9200_ID_PIOD,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk ssc0_clk = {
	.name		= "ssc0_clk",
	.pmc_mask	= 1 << AT91RM9200_ID_SSC0,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk ssc1_clk = {
	.name		= "ssc1_clk",
	.pmc_mask	= 1 << AT91RM9200_ID_SSC1,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk ssc2_clk = {
	.name		= "ssc2_clk",
	.pmc_mask	= 1 << AT91RM9200_ID_SSC2,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk tc0_clk = {
	.name		= "tc0_clk",
	.pmc_mask	= 1 << AT91RM9200_ID_TC0,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk tc1_clk = {
	.name		= "tc1_clk",
	.pmc_mask	= 1 << AT91RM9200_ID_TC1,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk tc2_clk = {
	.name		= "tc2_clk",
	.pmc_mask	= 1 << AT91RM9200_ID_TC2,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk tc3_clk = {
	.name		= "tc3_clk",
	.pmc_mask	= 1 << AT91RM9200_ID_TC3,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk tc4_clk = {
	.name		= "tc4_clk",
	.pmc_mask	= 1 << AT91RM9200_ID_TC4,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk tc5_clk = {
	.name		= "tc5_clk",
	.pmc_mask	= 1 << AT91RM9200_ID_TC5,
	.type		= CLK_TYPE_PERIPHERAL,
};

static struct clk *periph_clocks[] __initdata = {
	&pioA_clk,
	&pioB_clk,
	&pioC_clk,
	&pioD_clk,
	&usart0_clk,
	&usart1_clk,
	&usart2_clk,
	&usart3_clk,
	&mmc_clk,
	&udc_clk,
	&twi_clk,
	&spi_clk,
	&ssc0_clk,
	&ssc1_clk,
	&ssc2_clk,
	&tc0_clk,
	&tc1_clk,
	&tc2_clk,
	&tc3_clk,
	&tc4_clk,
	&tc5_clk,
	&ohci_clk,
	&ether_clk,
	// irq0 .. irq6
};

static struct clk_lookup periph_clocks_lookups[] = {
	CLKDEV_DEV_ID("at91rm9200-gpio0", &pioA_clk),
	CLKDEV_DEV_ID("at91rm9200-gpio1", &pioB_clk),
	CLKDEV_DEV_ID("at91rm9200-gpio2", &pioC_clk),
	CLKDEV_DEV_ID("at91rm9200-gpio3", &pioD_clk),
};

static struct clk_lookup usart_clocks_lookups[] = {
	CLKDEV_CON_DEV_ID("usart", "atmel_usart0", &mck),
	CLKDEV_CON_DEV_ID("usart", "atmel_usart1", &usart0_clk),
	CLKDEV_CON_DEV_ID("usart", "atmel_usart2", &usart1_clk),
	CLKDEV_CON_DEV_ID("usart", "atmel_usart3", &usart2_clk),
	CLKDEV_CON_DEV_ID("usart", "atmel_usart4", &usart3_clk),
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

static void __init at91rm9200_register_clocks(void)
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

/* --------------------------------------------------------------------
 *  AT91RM9200 processor initialization
 * -------------------------------------------------------------------- */
static void __init at91rm9200_initialize(void)
{
	/* Register the processor-specific clocks */
	at91rm9200_register_clocks();

	/* Register GPIO subsystem */
	at91_add_rm9200_gpio(0, AT91RM9200_BASE_PIOA);
	at91_add_rm9200_gpio(1, AT91RM9200_BASE_PIOB);
	at91_add_rm9200_gpio(2, AT91RM9200_BASE_PIOC);
	at91_add_rm9200_gpio(3, AT91RM9200_BASE_PIOD);
}

static int at91rm9200_setup(void)
{
	at91_boot_soc = at91rm9200_initialize;
	return 0;
}
pure_initcall(at91rm9200_setup);
