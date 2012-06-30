#ifndef __GPIO_H
#define __GPIO_H

#include <asm/gpio.h>

static inline int gpio_request(unsigned gpio, const char *label)
{
	return 0;
}

static inline void gpio_free(unsigned gpio)
{
}
#endif /* __GPIO_H */
