/*
 * Copyright (c) 2013 Juergen Beisert <kernel@pengutronix.de>, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef FIRMWARE_H
#define FIRMWARE_H

#include <types.h>
#include <driver.h>

struct firmware_handler {
	char *id; /* unique identifier for this firmware device */
	char *model; /* description for this device */
	struct device_d *dev;
	/* called once to prepare the firmware's programming cycle */
	int (*open)(struct firmware_handler*);
	/* called multiple times to program the firmware with the given data */
	int (*write)(struct firmware_handler*, const void*, size_t);
	/* called once to finish programming cycle */
	int (*close)(struct firmware_handler*);
};

struct firmware_mgr;

int firmwaremgr_register(struct firmware_handler *);

struct firmware_mgr *firmwaremgr_find(const char *);
#ifdef CONFIG_FIRMWARE
struct firmware_mgr *firmwaremgr_find_by_node(const struct device_node *np);
#else
static inline struct firmware_mgr *firmwaremgr_find_by_node(const struct device_node *np)
{
	return NULL;
}
#endif

void firmwaremgr_list_handlers(void);

#ifdef CONFIG_FIRMWARE
int firmwaremgr_load_file(struct firmware_mgr *, const char *path);
#else
static inline int firmwaremgr_load_file(struct firmware_mgr *mgr, const char *path)
{
	return -ENOSYS;
}
#endif

#define get_builtin_firmware(name, start, size) \
	{							\
		extern char _fw_##name##_start[];		\
		extern char _fw_##name##_end[];			\
		*start = (typeof(*start)) _fw_##name##_start;	\
		*size = _fw_##name##_end - _fw_##name##_start;	\
	}

#endif /* FIRMWARE_H */
