#include <common.h>
#include <gpio.h>
#include <init.h>
#include <restart.h>
#include <mach/hardware.h>
#include <mach/at91_pmc.h>
#include <mach/cpu.h>
#include <mach/board.h>
#include <mach/at91_rstc.h>

#include "generic.h"
#include "clock.h"

/* --------------------------------------------------------------------
 *  Clocks
 * -------------------------------------------------------------------- */

/*
 * The peripheral clocks.
 */
static struct clk pioAB_clk = {
	.name		= "pioAB_clk",
	.pmc_mask	= 1 << AT91SAM9N12_ID_PIOAB,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk pioCD_clk = {
	.name		= "pioCD_clk",
	.pmc_mask	= 1 << AT91SAM9N12_ID_PIOCD,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk usart0_clk = {
	.name		= "usart0_clk",
	.pmc_mask	= 1 << AT91SAM9N12_ID_USART0,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk usart1_clk = {
	.name		= "usart1_clk",
	.pmc_mask	= 1 << AT91SAM9N12_ID_USART1,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk usart2_clk = {
	.name		= "usart2_clk",
	.pmc_mask	= 1 << AT91SAM9N12_ID_USART2,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk usart3_clk = {
	.name		= "usart3_clk",
	.pmc_mask	= 1 << AT91SAM9N12_ID_USART3,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk twi0_clk = {
	.name		= "twi0_clk",
	.pmc_mask	= 1 << AT91SAM9N12_ID_TWI0,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk twi1_clk = {
	.name		= "twi1_clk",
	.pmc_mask	= 1 << AT91SAM9N12_ID_TWI1,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk mmc_clk = {
	.name		= "mci_clk",
	.pmc_mask	= 1 << AT91SAM9N12_ID_MCI,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk spi0_clk = {
	.name		= "spi0_clk",
	.pmc_mask	= 1 << AT91SAM9N12_ID_SPI0,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk spi1_clk = {
	.name		= "spi1_clk",
	.pmc_mask	= 1 << AT91SAM9N12_ID_SPI1,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk uart0_clk = {
	.name		= "uart0_clk",
	.pmc_mask	= 1 << AT91SAM9N12_ID_UART0,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk uart1_clk = {
	.name		= "uart1_clk",
	.pmc_mask	= 1 << AT91SAM9N12_ID_UART1,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk tcb0_clk = {
	.name		= "tcb0_clk",
	.pmc_mask	= 1 << AT91SAM9N12_ID_TCB,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk pwm_clk = {
	.name		= "pwm_clk",
	.pmc_mask	= 1 << AT91SAM9N12_ID_PWM,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk adc_clk = {
	.name		= "adc_clk",
	.pmc_mask	= 1 << AT91SAM9N12_ID_ADC,
	.type	= CLK_TYPE_PERIPHERAL,
};
static struct clk dma_clk = {
	.name		= "dma_clk",
	.pmc_mask	= 1 << AT91SAM9N12_ID_DMA,
	.type	= CLK_TYPE_PERIPHERAL,
};
static struct clk uhpfs_clk = {
	.name		= "uhpfs_clk",
	.pmc_mask	= 1 << AT91SAM9N12_ID_UHPFS,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk udc_clk = {
	.name		= "udc_clk",
	.pmc_mask	= 1 << AT91SAM9N12_ID_UDPFS,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk lcdc_clk = {
	.name		= "lcdc_clk",
	.pmc_mask	= 1 << AT91SAM9N12_ID_LCDC,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk ssc_clk = {
	.name		= "ssc_clk",
	.pmc_mask	= 1 << AT91SAM9N12_ID_SSC,
	.type		= CLK_TYPE_PERIPHERAL,
};

static struct clk *periph_clocks[] __initdata = {
	&pioAB_clk,
	&pioCD_clk,
	&usart0_clk,
	&usart1_clk,
	&usart2_clk,
	&usart3_clk,
	&twi0_clk,
	&twi1_clk,
	&mmc_clk,
	&spi0_clk,
	&spi1_clk,
	&uart0_clk,
	&uart1_clk,
	&tcb0_clk,
	&pwm_clk,
	&adc_clk,
	&dma_clk,
	&lcdc_clk,
	&uhpfs_clk,
	&udc_clk,
	&ssc_clk,
};

static struct clk_lookup periph_clocks_lookups[] = {
	CLKDEV_CON_ID("ohci_clk", &uhpfs_clk),
	CLKDEV_CON_DEV_ID("spi_clk", "atmel_spi0", &spi0_clk),
	CLKDEV_CON_DEV_ID("spi_clk", "atmel_spi1", &spi1_clk),
	CLKDEV_DEV_ID("at91sam9x5-gpio0", &pioAB_clk),
	CLKDEV_DEV_ID("at91sam9x5-gpio1", &pioAB_clk),
	CLKDEV_DEV_ID("at91sam9x5-gpio2", &pioCD_clk),
	CLKDEV_DEV_ID("at91sam9x5-gpio3", &pioCD_clk),
	CLKDEV_DEV_ID("at91-pit", &mck),
	CLKDEV_CON_DEV_ID("hck1", "atmel_hlcdfb", &lcdc_clk),
};

static struct clk_lookup usart_clocks_lookups[] = {
	CLKDEV_CON_DEV_ID("usart", "atmel_usart0", &mck),
	CLKDEV_CON_DEV_ID("usart", "atmel_usart1", &usart0_clk),
	CLKDEV_CON_DEV_ID("usart", "atmel_usart2", &usart1_clk),
	CLKDEV_CON_DEV_ID("usart", "atmel_usart3", &usart2_clk),
	CLKDEV_CON_DEV_ID("usart", "atmel_usart4", &usart3_clk),
};

/*
 * The two programmable clocks.
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

static void __init at91sam9n12_register_clocks(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(periph_clocks); i++)
		clk_register(periph_clocks[i]);

	clk_register(&pck0);
	clk_register(&pck1);

	clkdev_add_table(periph_clocks_lookups,
			 ARRAY_SIZE(periph_clocks_lookups));
	clkdev_add_table(usart_clocks_lookups,
			 ARRAY_SIZE(usart_clocks_lookups));

}

static void at91sam9n12_restart(struct restart_handler *rst)
{
	at91sam9g45_reset(IOMEM(AT91SAM9N12_BASE_DDRSDRC0),
			  IOMEM(AT91SAM9N12_BASE_RSTC + AT91_RSTC_CR));
}

/* --------------------------------------------------------------------
 *  AT91SAM9N12 processor initialization
 * -------------------------------------------------------------------- */

static void at91sam9n12_initialize(void)
{
	/* Register the processor-specific clocks */
	at91sam9n12_register_clocks();

	/* Register GPIO subsystem */
	at91_add_sam9x5_gpio(0, AT91SAM9N12_BASE_PIOA);
	at91_add_sam9x5_gpio(1, AT91SAM9N12_BASE_PIOB);
	at91_add_sam9x5_gpio(2, AT91SAM9N12_BASE_PIOC);
	at91_add_sam9x5_gpio(3, AT91SAM9N12_BASE_PIOD);

	at91_add_pit(AT91SAM9N12_BASE_PIT);
	at91_add_sam9_smc(DEVICE_ID_SINGLE, AT91SAM9N12_BASE_SMC, 0x200);

	restart_handler_register_fn(at91sam9n12_restart);
}

static int at91sam9n12_setup(void)
{
	at91_boot_soc = at91sam9n12_initialize;
	return 0;
}
pure_initcall(at91sam9n12_setup);
