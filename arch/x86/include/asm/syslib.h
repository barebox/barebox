/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: 2009 Juergen Beisert, Pengutronix */

#ifdef CONFIG_X86_BIOS_BRINGUP

extern int bios_disk_rw_int13_extensions(int, int, void*) __attribute__((regparm(3)));
extern uint16_t bios_get_memsize(void);

#endif

#ifdef CONFIG_CMD_LINUX16
extern void bios_start_linux(unsigned) __attribute__((regparm(1)));
#endif
