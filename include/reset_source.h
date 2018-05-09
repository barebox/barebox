/*
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
};

#ifdef CONFIG_RESET_SOURCE
void reset_source_set_priority(enum reset_src_type, unsigned int priority);
enum reset_src_type reset_source_get(void);
void reset_source_set_instance(enum reset_src_type type, int instance);
int reset_source_get_instance(void);
unsigned int of_get_reset_source_priority(struct device_node *node);
const char *reset_source_name(void);
#else
static inline void reset_source_set_priority(enum reset_src_type type,
		unsigned int priority)
{
}

static inline void reset_source_set_instance(enum reset_src_type type, int instance)
{
}

static inline enum reset_src_type reset_source_get(void)
{
	return RESET_UKWN;
}

static inline int reset_source_get_instance(void)
{
	return 0;
}

static inline unsigned int of_get_reset_source_priority(struct device_node *node)
{
	return 0;
}

static inline const char *reset_source_name(void)
{
	return "unknown";
}
#endif

#define RESET_SOURCE_DEFAULT_PRIORITY 100

static inline void reset_source_set(enum reset_src_type type)
{
	reset_source_set_priority(type, RESET_SOURCE_DEFAULT_PRIORITY);
}

#endif /* __INCLUDE_RESET_SOURCE_H */
