#include <common.h>
#include <gpio.h>
#include <init.h>
#include <asm/hardware.h>

static struct at91_gpio_bank at91sam9263_gpio[] = {
	{
		.id		= AT91C_ID_PIOA,
		.regbase	= (void __iomem *)AT91C_BASE_PIOA,
	}, {
		.id		= AT91C_ID_PIOB,
		.regbase	= (void __iomem *)AT91C_BASE_PIOB,
	}, {
		.id		= AT91C_ID_PIOCDE,
		.regbase	= (void __iomem *)AT91C_BASE_PIOC,
	}
};

static int at91sam9263_initialize(void)
{
	/* Register GPIO subsystem */
	at91_gpio_init(at91sam9263_gpio, 3);
	return 0;
}

core_initcall(at91sam9263_initialize);
