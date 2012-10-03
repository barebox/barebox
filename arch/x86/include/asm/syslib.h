/*
 * Copyright (C) 2009 Juergen Beisert, Pengutronix
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
 *
 */

extern unsigned int x86_uart_read(unsigned long, unsigned char);
extern void x86_uart_write(unsigned int, unsigned long, unsigned char);

#ifdef CONFIG_X86_BIOS_BRINGUP

extern int bios_disk_rw_int13_extensions(int, int, void*) __attribute__((regparm(3)));
extern uint16_t bios_get_memsize(void);

#endif

#ifdef CONFIG_CMD_LINUX16
extern void bios_start_linux(unsigned) __attribute__((regparm(1)));
#endif
