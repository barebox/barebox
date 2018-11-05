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
static struct clk pioA_clk = {
	.name		= "pioA_clk",
	.pmc_mask	= 1 << AT91SAM9G45_ID_PIOA,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk pioB_clk = {
	.name		= "pioB_clk",
	.pmc_mask	= 1 << AT91SAM9G45_ID_PIOB,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk pioC_clk = {
	.name		= "pioC_clk",
	.pmc_mask	= 1 << AT91SAM9G45_ID_PIOC,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk pioDE_clk = {
	.name		= "pioDE_clk",
	.pmc_mask	= 1 << AT91SAM9G45_ID_PIODE,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk usart0_clk = {
	.name		= "usart0_clk",
	.pmc_mask	= 1 << AT91SAM9G45_ID_US0,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk usart1_clk = {
	.name		= "usart1_clk",
	.pmc_mask	= 1 << AT91SAM9G45_ID_US1,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk usart2_clk = {
	.name		= "usart2_clk",
	.pmc_mask	= 1 << AT91SAM9G45_ID_US2,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk usart3_clk = {
	.name		= "usart3_clk",
	.pmc_mask	= 1 << AT91SAM9G45_ID_US3,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk mmc0_clk = {
	.name		= "mci0_clk",
	.pmc_mask	= 1 << AT91SAM9G45_ID_MCI0,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk twi0_clk = {
	.name		= "twi0_clk",
	.pmc_mask	= 1 << AT91SAM9G45_ID_TWI0,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk twi1_clk = {
	.name		= "twi1_clk",
	.pmc_mask	= 1 << AT91SAM9G45_ID_TWI1,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk spi0_clk = {
	.name		= "spi0_clk",
	.pmc_mask	= 1 << AT91SAM9G45_ID_SPI0,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk spi1_clk = {
	.name		= "spi1_clk",
	.pmc_mask	= 1 << AT91SAM9G45_ID_SPI1,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk ssc0_clk = {
	.name		= "ssc0_clk",
	.pmc_mask	= 1 << AT91SAM9G45_ID_SSC0,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk ssc1_clk = {
	.name		= "ssc1_clk",
	.pmc_mask	= 1 << AT91SAM9G45_ID_SSC1,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk tcb0_clk = {
	.name		= "tcb0_clk",
	.pmc_mask	= 1 << AT91SAM9G45_ID_TCB,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk pwm_clk = {
	.name		= "pwm_clk",
	.pmc_mask	= 1 << AT91SAM9G45_ID_PWMC,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk tsc_clk = {
	.name		= "tsc_clk",
	.pmc_mask	= 1 << AT91SAM9G45_ID_TSC,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk dma_clk = {
	.name		= "dma_clk",
	.pmc_mask	= 1 << AT91SAM9G45_ID_DMA,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk uhphs_clk = {
	.name		= "uhphs_clk",
	.pmc_mask	= 1 << AT91SAM9G45_ID_UHPHS,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk lcdc_clk = {
	.name		= "lcdc_clk",
	.pmc_mask	= 1 << AT91SAM9G45_ID_LCDC,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk ac97_clk = {
	.name		= "ac97_clk",
	.pmc_mask	= 1 << AT91SAM9G45_ID_AC97C,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk macb_clk = {
	.name		= "macb_clk",
	.pmc_mask	= 1 << AT91SAM9G45_ID_EMAC,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk isi_clk = {
	.name		= "isi_clk",
	.pmc_mask	= 1 << AT91SAM9G45_ID_ISI,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk udphs_clk = {
	.name		= "udphs_clk",
	.pmc_mask	= 1 << AT91SAM9G45_ID_UDPHS,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk mmc1_clk = {
	.name		= "mci1_clk",
	.pmc_mask	= 1 << AT91SAM9G45_ID_MCI1,
	.type		= CLK_TYPE_PERIPHERAL,
};

/* Video decoder clock - Only for sam9m10/sam9m11 */
static struct clk vdec_clk = {
	.name		= "vdec_clk",
	.pmc_mask	= 1 << AT91SAM9G45_ID_VDEC,
	.type		= CLK_TYPE_PERIPHERAL,
};

static struct clk *periph_clocks[] __initdata = {
	&pioA_clk,
	&pioB_clk,
	&pioC_clk,
	&pioDE_clk,
	&usart0_clk,
	&usart1_clk,
	&usart2_clk,
	&usart3_clk,
	&mmc0_clk,
	&twi0_clk,
	&twi1_clk,
	&spi0_clk,
	&spi1_clk,
	&ssc0_clk,
	&ssc1_clk,
	&tcb0_clk,
	&pwm_clk,
	&tsc_clk,
	&dma_clk,
	&uhphs_clk,
	&lcdc_clk,
	&ac97_clk,
	&macb_clk,
	&isi_clk,
	&udphs_clk,
	&mmc1_clk,
};

static struct clk_lookup periph_clocks_lookups[] = {
	/* One additional fake clock for ohci */
	CLKDEV_CON_ID("ohci_clk", &uhphs_clk),
	CLKDEV_CON_DEV_ID("ehci_clk", "atmel-ehci", &uhphs_clk),
	CLKDEV_CON_DEV_ID("mci_clk", "atmel_mci0", &mmc0_clk),
	CLKDEV_CON_DEV_ID("mci_clk", "atmel_mci1", &mmc1_clk),
	CLKDEV_CON_DEV_ID("spi_clk", "atmel_spi0", &spi0_clk),
	CLKDEV_CON_DEV_ID("spi_clk", "atmel_spi1", &spi1_clk),
	CLKDEV_DEV_ID("at91-pit", &mck),
	CLKDEV_CON_DEV_ID("hck1", "atmel_lcdfb", &lcdc_clk),
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

static void __init at91sam9g45_register_clocks(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(periph_clocks); i++)
		clk_register(periph_clocks[i]);

	clkdev_add_table(periph_clocks_lookups,
			 ARRAY_SIZE(periph_clocks_lookups));
	clkdev_add_table(usart_clocks_lookups,
			 ARRAY_SIZE(usart_clocks_lookups));

	clkdev_add_physbase(&twi0_clk, AT91SAM9G45_BASE_TWI0, NULL);
	clkdev_add_physbase(&twi1_clk, AT91SAM9G45_BASE_TWI1, NULL);
	clkdev_add_physbase(&pioA_clk, AT91SAM9G45_BASE_PIOA, NULL);
	clkdev_add_physbase(&pioB_clk, AT91SAM9G45_BASE_PIOB, NULL);
	clkdev_add_physbase(&pioC_clk, AT91SAM9G45_BASE_PIOC, NULL);
	clkdev_add_physbase(&pioDE_clk, AT91SAM9G45_BASE_PIOD, NULL);
	clkdev_add_physbase(&pioDE_clk, AT91SAM9G45_BASE_PIOE, NULL);

	if (cpu_is_at91sam9m10() || cpu_is_at91sam9m11())
		clk_register(&vdec_clk);

	clk_register(&pck0);
	clk_register(&pck1);
}

static void at91sam9g45_restart(struct restart_handler *rst)
{
	at91sam9g45_reset(IOMEM(AT91SAM9G45_BASE_DDRSDRC0),
			  IOMEM(AT91SAM9G45_BASE_RSTC + AT91_RSTC_CR));
}

static void at91sam9g45_initialize(void)
{
	/* Register the processor-specific clocks */
	at91sam9g45_register_clocks();

	/* Register GPIO subsystem */
	at91_add_rm9200_gpio(0, AT91SAM9G45_BASE_PIOA);
	at91_add_rm9200_gpio(1, AT91SAM9G45_BASE_PIOB);
	at91_add_rm9200_gpio(2, AT91SAM9G45_BASE_PIOC);
	at91_add_rm9200_gpio(3, AT91SAM9G45_BASE_PIOD);
	at91_add_rm9200_gpio(4, AT91SAM9G45_BASE_PIOE);

	at91_add_pit(AT91SAM9G45_BASE_PIT);
	at91_add_sam9_smc(DEVICE_ID_SINGLE, AT91SAM9G45_BASE_SMC, 0x200);

	restart_handler_register_fn(at91sam9g45_restart);
}

static int at91sam9g45_setup(void)
{
	at91_boot_soc = at91sam9g45_initialize;
	return 0;
}
pure_initcall(at91sam9g45_setup);
