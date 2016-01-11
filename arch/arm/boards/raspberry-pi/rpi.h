#ifndef __ARCH_ARM_BOARDS_RPI_H__
#define __ARCH_ARM_BOARDS_RPI_H__

#include <types.h>
#include <led.h>

#include <mach/mbox.h>

#define RPI_MODEL(_id, _name, _init)	\
	[_id] = {				\
		.name			= _name,\
		.init			= _init,\
	}

struct rpi_model {
	const char *name;
	void (*init)(void);
};

extern const struct rpi_model rpi_models[];
extern const size_t rpi_models_size;
extern const char *rpi_model_string;
extern struct gpio_led rpi_leds[];

void rpi_b_plus_init(void);
void rpi_set_usbethaddr(void);

#endif /* __ARCH_ARM_BOARDS_RPI_H__ */
