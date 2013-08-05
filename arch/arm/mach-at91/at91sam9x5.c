#include <common.h>
#include <gpio.h>
#include <init.h>
#include <asm/hardware.h>
#include <mach/at91_pmc.h>
#include <mach/io.h>
#include <mach/cpu.h>

#include "soc.h"
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
	.pmc_mask	= 1 << AT91SAM9X5_ID_PIOAB,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk pioCD_clk = {
	.name		= "pioCD_clk",
	.pmc_mask	= 1 << AT91SAM9X5_ID_PIOCD,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk smd_clk = {
	.name		= "smd_clk",
	.pmc_mask	= 1 << AT91SAM9X5_ID_SMD,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk usart0_clk = {
	.name		= "usart0_clk",
	.pmc_mask	= 1 << AT91SAM9X5_ID_USART0,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk usart1_clk = {
	.name		= "usart1_clk",
	.pmc_mask	= 1 << AT91SAM9X5_ID_USART1,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk usart2_clk = {
	.name		= "usart2_clk",
	.pmc_mask	= 1 << AT91SAM9X5_ID_USART2,
	.type		= CLK_TYPE_PERIPHERAL,
};
/* USART3 clock - Only for sam9g25/sam9x25 */
static struct clk usart3_clk = {
	.name		= "usart3_clk",
	.pmc_mask	= 1 << AT91SAM9X5_ID_USART3,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk twi0_clk = {
	.name		= "twi0_clk",
	.pmc_mask	= 1 << AT91SAM9X5_ID_TWI0,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk twi1_clk = {
	.name		= "twi1_clk",
	.pmc_mask	= 1 << AT91SAM9X5_ID_TWI1,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk twi2_clk = {
	.name		= "twi2_clk",
	.pmc_mask	= 1 << AT91SAM9X5_ID_TWI2,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk mmc0_clk = {
	.name		= "mci0_clk",
	.pmc_mask	= 1 << AT91SAM9X5_ID_MCI0,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk spi0_clk = {
	.name		= "spi0_clk",
	.pmc_mask	= 1 << AT91SAM9X5_ID_SPI0,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk spi1_clk = {
	.name		= "spi1_clk",
	.pmc_mask	= 1 << AT91SAM9X5_ID_SPI1,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk uart0_clk = {
	.name		= "uart0_clk",
	.pmc_mask	= 1 << AT91SAM9X5_ID_UART0,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk uart1_clk = {
	.name		= "uart1_clk",
	.pmc_mask	= 1 << AT91SAM9X5_ID_UART1,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk tcb0_clk = {
	.name		= "tcb0_clk",
	.pmc_mask	= 1 << AT91SAM9X5_ID_TCB,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk pwm_clk = {
	.name		= "pwm_clk",
	.pmc_mask	= 1 << AT91SAM9X5_ID_PWM,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk adc_clk = {
	.name		= "adc_clk",
	.pmc_mask	= 1 << AT91SAM9X5_ID_ADC,
	.type	= CLK_TYPE_PERIPHERAL,
};
static struct clk dma0_clk = {
	.name		= "dma0_clk",
	.pmc_mask	= 1 << AT91SAM9X5_ID_DMA0,
	.type	= CLK_TYPE_PERIPHERAL,
};
static struct clk dma1_clk = {
	.name		= "dma1_clk",
	.pmc_mask	= 1 << AT91SAM9X5_ID_DMA1,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk uhphs_clk = {
	.name		= "uhphs",
	.pmc_mask	= 1 << AT91SAM9X5_ID_UHPHS,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk udphs_clk = {
	.name		= "udphs_clk",
	.pmc_mask	= 1 << AT91SAM9X5_ID_UDPHS,
	.type		= CLK_TYPE_PERIPHERAL,
};
/* emac0 clock - Only for sam9g25/sam9x25/sam9g35/sam9x35 */
static struct clk macb0_clk = {
	.name		= "macb0_clk",
	.pmc_mask	= 1 << AT91SAM9X5_ID_EMAC0,
	.type		= CLK_TYPE_PERIPHERAL,
};
/* lcd clock - Only for sam9g15/sam9g35/sam9x35 */
static struct clk lcdc_clk = {
	.name		= "lcdc_clk",
	.pmc_mask	= 1 << AT91SAM9X5_ID_LCDC,
	.type		= CLK_TYPE_PERIPHERAL,
};
/* isi clock - Only for sam9g25 */
static struct clk isi_clk = {
	.name		= "isi_clk",
	.pmc_mask	= 1 << AT91SAM9X5_ID_ISI,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk mmc1_clk = {
	.name		= "mci1_clk",
	.pmc_mask	= 1 << AT91SAM9X5_ID_MCI1,
	.type		= CLK_TYPE_PERIPHERAL,
};
/* emac1 clock - Only for sam9x25 */
static struct clk macb1_clk = {
	.name		= "macb1_clk",
	.pmc_mask	= 1 << AT91SAM9X5_ID_EMAC1,
	.type		= CLK_TYPE_PERIPHERAL,
};
static struct clk ssc_clk = {
	.name		= "ssc_clk",
	.pmc_mask	= 1 << AT91SAM9X5_ID_SSC,
	.type		= CLK_TYPE_PERIPHERAL,
};
/* can0 clock - Only for sam9x35 */
static struct clk can0_clk = {
	.name		= "can0_clk",
	.pmc_mask	= 1 << AT91SAM9X5_ID_CAN0,
	.type		= CLK_TYPE_PERIPHERAL,
};
/* can1 clock - Only for sam9x35 */
static struct clk can1_clk = {
	.name		= "can1_clk",
	.pmc_mask	= 1 << AT91SAM9X5_ID_CAN1,
	.type		= CLK_TYPE_PERIPHERAL,
};

static struct clk *periph_clocks[] __initdata = {
	&pioAB_clk,
	&pioCD_clk,
	&smd_clk,
	&usart0_clk,
	&usart1_clk,
	&usart2_clk,
	&twi0_clk,
	&twi1_clk,
	&twi2_clk,
	&mmc0_clk,
	&spi0_clk,
	&spi1_clk,
	&uart0_clk,
	&uart1_clk,
	&tcb0_clk,
	&pwm_clk,
	&adc_clk,
	&dma0_clk,
	&dma1_clk,
	&uhphs_clk,
	&udphs_clk,
	&mmc1_clk,
	&ssc_clk,
	// irq0
};

static struct clk_lookup periph_clocks_lookups[] = {
	CLKDEV_CON_DEV_ID("macb_clk", "macb0", &macb0_clk),
	CLKDEV_CON_DEV_ID("macb_clk", "macb1", &macb1_clk),
	CLKDEV_CON_ID("ohci_clk", &uhphs_clk),
	CLKDEV_CON_DEV_ID("ehci_clk", "atmel-ehci", &uhphs_clk),
	CLKDEV_CON_DEV_ID("spi_clk", "atmel_spi0", &spi0_clk),
	CLKDEV_CON_DEV_ID("spi_clk", "atmel_spi1", &spi1_clk),
	CLKDEV_CON_DEV_ID("mci_clk", "atmel_mci0", &mmc0_clk),
	CLKDEV_CON_DEV_ID("mci_clk", "atmel_mci1", &mmc1_clk),
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

static void __init at91sam9x5_register_clocks(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(periph_clocks); i++)
		clk_register(periph_clocks[i]);

	clkdev_add_table(periph_clocks_lookups,
			 ARRAY_SIZE(periph_clocks_lookups));
	clkdev_add_table(usart_clocks_lookups,
			 ARRAY_SIZE(usart_clocks_lookups));

	if (cpu_is_at91sam9g25()
	|| cpu_is_at91sam9x25())
		clk_register(&usart3_clk);

	if (cpu_is_at91sam9g25()
	|| cpu_is_at91sam9x25()
	|| cpu_is_at91sam9g35()
	|| cpu_is_at91sam9x35())
		clk_register(&macb0_clk);

	if (cpu_is_at91sam9g15()
	|| cpu_is_at91sam9g35()
	|| cpu_is_at91sam9x35())
		clk_register(&lcdc_clk);

	if (cpu_is_at91sam9g25())
		clk_register(&isi_clk);

	if (cpu_is_at91sam9x25())
		clk_register(&macb1_clk);

	if (cpu_is_at91sam9x25()
	|| cpu_is_at91sam9x35()) {
		clk_register(&can0_clk);
		clk_register(&can1_clk);
	}

	clk_register(&pck0);
	clk_register(&pck1);
}

/* --------------------------------------------------------------------
 *  AT91SAM9x5 processor initialization
 * -------------------------------------------------------------------- */

static void at91sam9x5_initialize(void)
{
	/* Register the processor-specific clocks */
	at91sam9x5_register_clocks();

	/* Register GPIO subsystem */
	at91_add_sam9x5_gpio(0, AT91SAM9X5_BASE_PIOA);
	at91_add_sam9x5_gpio(1, AT91SAM9X5_BASE_PIOB);
	at91_add_sam9x5_gpio(2, AT91SAM9X5_BASE_PIOC);
	at91_add_sam9x5_gpio(3, AT91SAM9X5_BASE_PIOD);

	at91_add_pit(AT91SAM9X5_BASE_PIT);
	at91_add_sam9_smc(DEVICE_ID_SINGLE, AT91SAM9X5_BASE_SMC, 0x200);
}

AT91_SOC_START(sam9x5)
	.init = at91sam9x5_initialize,
AT91_SOC_END
