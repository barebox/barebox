#include <common.h>
#include <gpio.h>
#include <init.h>
#include <asm/hardware.h>

static struct at91_gpio_bank at91sam9263_gpio[] = {
	{
		.id		= AT91SAM9263_ID_PIOA,
		.offset		= AT91_PIOA,
	}, {
		.id		= AT91SAM9263_ID_PIOB,
		.offset		= AT91_PIOB,
	}, {
		.id		= AT91SAM9263_ID_PIOCDE,
		.offset		= AT91_PIOC,
	}, {
		.id		= AT91SAM9263_ID_PIOCDE,
		.offset		= AT91_PIOD,
	}, {
		.id		= AT91SAM9263_ID_PIOCDE,
		.offset		= AT91_PIOE,
	}
};

static int at91sam9263_initialize(void)
{
	/* Register GPIO subsystem */
	at91_gpio_init(at91sam9263_gpio, 5);
	return 0;
}

core_initcall(at91sam9263_initialize);
