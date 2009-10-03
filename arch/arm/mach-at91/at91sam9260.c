#include <common.h>
#include <gpio.h>
#include <init.h>
#include <asm/hardware.h>

static struct at91_gpio_bank at91sam9260_gpio[] = {
	{
		.id		= AT91SAM9260_ID_PIOA,
		.offset		= AT91_PIOA,
	}, {
		.id		= AT91SAM9260_ID_PIOB,
		.offset		= AT91_PIOB,
	}, {
		.id		= AT91SAM9260_ID_PIOC,
		.offset		= AT91_PIOC,
	}
};

static int at91sam9260_initialize(void)
{
	/* Register GPIO subsystem */
	at91_gpio_init(at91sam9260_gpio, 3);
	return 0;
}

core_initcall(at91sam9260_initialize);
