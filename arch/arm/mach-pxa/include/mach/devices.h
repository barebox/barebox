/*
 * (C) 2011 Robert Jarzmik <robert.jarzmik@free.fr>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <i2c/i2c.h>
#include <mach/pxafb.h>

struct device_d *pxa_add_i2c(void *base, int id,
			     struct i2c_platform_data *pdata);
struct device_d *pxa_add_uart(void *base, int id);
struct device_d *pxa_add_fb(void *base, struct pxafb_platform_data *pdata);
struct device_d *pxa_add_mmc(void *base, int id, void *pdata);
struct device_d *pxa_add_pwm(void *base, int id);
