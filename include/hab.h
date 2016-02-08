/*
 * Copyright (C) 2014, 2015 Marc Kleine-Budde <mkl@pengutronix.de>
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
 */

#ifndef __HABV4_H
#define __HABV4_H

#ifdef CONFIG_HABV4
int imx28_hab_get_status(void);
int imx6_hab_get_status(void);
#else
static inline int imx28_hab_get_status(void)
{
	return -EPERM;
}
static inline int imx6_hab_get_status(void)
{
	return -EPERM;
}
#endif

#ifdef CONFIG_HABV3
int imx25_hab_get_status(void);
#else
static inline int imx25_hab_get_status(void)
{
	return -EPERM;
}
#endif

#endif /* __HABV4_H */
