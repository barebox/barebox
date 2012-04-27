/*
 * (c) 2012 Pengutronix, Juergen Beisert <kernel@pengutronix.de>
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

#ifndef _NETX_DEVICES_H
# define _NETX_DEVICES_H

#include <mach/netx-regs.h>

static inline struct device_d *netx_add_uart(resource_size_t base, int index)
{
	return add_generic_device("netx_serial", index, NULL,
				base, 0x40, IORESOURCE_MEM, NULL);
}

static inline struct device_d *netx_add_uart0(void)
{
	return netx_add_uart(NETX_PA_UART0, 0);
}

static inline struct device_d *netx_add_uart1(void)
{
	return netx_add_uart(NETX_PA_UART1, 1);
}

static inline struct device_d *netx_add_uart3(void)
{
	return netx_add_uart(NETX_PA_UART2, 2);
}

/* parallel flash connected to the SRAM interface */
static inline struct device_d *netx_add_pflash(resource_size_t size)
{
	return add_cfi_flash_device(0, NETX_CS0_BASE, size, 0);
}

static inline struct device_d *netx_add_eth(int index, void *pdata)
{
	return add_generic_device("netx-eth", index, NULL,
					0, 0, IORESOURCE_MEM, pdata);
}

static inline struct device_d *netx_add_eth0(void *pdata)
{
	return netx_add_eth(0, pdata);
}

static inline struct device_d *netx_add_eth1(void *pdata)
{
	return netx_add_eth(1, pdata);
}

static inline struct device_d *netx_add_eth2(void *pdata)
{
	return netx_add_eth(2, pdata);
}

static inline struct device_d *netx_add_eth3(void *pdata)
{
	return netx_add_eth(3, pdata);
}

#endif /* _NETX_DEVICES_H */
