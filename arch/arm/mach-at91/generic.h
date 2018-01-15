/*
 * linux/arch/arm/mach-at91/generic.h
 *
 *  Copyright (C) 2005 David Brownell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/* function called by setup to perform late init */
extern void (*at91_boot_soc)(void);

/* Clocks */
#ifdef CONFIG_COMMON_CLK_OF_PROVIDER
static inline int __init at91_clock_init(void) { return 0; }
#else
extern int __init at91_clock_init(void);
#endif

static inline struct device_d *at91_add_rm9200_gpio(int id, resource_size_t start)
{
	return add_generic_device("at91rm9200-gpio", id, NULL, start, 512,
				  IORESOURCE_MEM, NULL);
}

static inline struct device_d *at91_add_sam9x5_gpio(int id, resource_size_t start)
{
	return add_generic_device("at91sam9x5-gpio", id, NULL, start, 512,
				  IORESOURCE_MEM, NULL);
}

static inline struct device_d *at91_add_pit(resource_size_t start)
{
	return add_generic_device("at91-pit", DEVICE_ID_SINGLE, NULL, start, 16,
				  IORESOURCE_MEM, NULL);
}

static inline struct device_d *at91_add_sam9_smc(int id, resource_size_t start,
						 resource_size_t size)
{
	return add_generic_device("at91sam9-smc", id, NULL, start, size,
				  IORESOURCE_MEM, NULL);
}
