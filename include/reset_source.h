/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __INCLUDE_RESET_SOURCE_H
# define __INCLUDE_RESET_SOURCE_H

enum reset_src_type {
	RESET_UKWN,	/* maybe the SoC cannot detect the reset source */
	RESET_POR,	/* Power On Reset (cold start) */
	RESET_RST,	/* generic ReSeT (warm start) */
	RESET_WDG,	/* watchdog */
	RESET_WKE,	/* wake-up (some SoCs can handle this) */
	RESET_JTAG,	/* JTAG reset */
	RESET_THERM,	/* SoC shut down because of overtemperature */
	RESET_EXT,	/* External reset through device pin */
	RESET_BROWNOUT,	/* Brownout Reset */
};

#ifdef CONFIG_RESET_SOURCE

enum reset_src_type reset_source_get(void);
const char *reset_source_to_string(enum reset_src_type st);
int reset_source_get_instance(void);
struct device *reset_source_get_device(void);

void reset_source_set_device(struct device *dev, enum reset_src_type st,
			     unsigned int priority);
void reset_source_set_prinst(enum reset_src_type,
			     unsigned int priority, int instance);

#else

static inline enum reset_src_type reset_source_get(void)
{
	return RESET_UKWN;
}

static inline const char *reset_source_to_string(enum reset_src_type st)
{
	return "unknown";
}

static inline int reset_source_get_instance(void)
{
	return -1;
}

static inline struct device *reset_source_get_device(void)
{
	return NULL;
}

static inline void reset_source_set_device(struct device *dev,
					   enum reset_src_type st,
					   unsigned int priority)
{
}

static inline void reset_source_set_prinst(enum reset_src_type type,
					   unsigned int priority, int instance)
{
}

static inline void reset_source_set_instance(enum reset_src_type type, int instance)
{
}
#endif

#define RESET_SOURCE_DEFAULT_PRIORITY 100

static inline void reset_source_set_priority(enum reset_src_type type,
					     unsigned int priority)
{
	reset_source_set_prinst(type, priority, -1);
}

static inline void reset_source_set(enum reset_src_type type)
{
	reset_source_set_priority(type, RESET_SOURCE_DEFAULT_PRIORITY);
}

static inline const char *reset_source_name(void)
{
	return reset_source_to_string(reset_source_get());
}

#endif /* __INCLUDE_RESET_SOURCE_H */
