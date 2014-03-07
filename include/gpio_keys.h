#ifndef _GPIO_KEYS_H
#define _GPIO_KEYS_H

#include <poller.h>
#include <kfifo.h>

struct gpio_keys_button {
	/* Configuration parameters */
	int code;

	int gpio;
	int active_low;
};

struct gpio_keys_platform_data {
	struct gpio_keys_button *buttons;
	int nbuttons;

	/* optional */
	int fifo_size;
};

#endif
