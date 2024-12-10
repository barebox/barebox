/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef INCLUDE_WATCHDOG_H
# define INCLUDE_WATCHDOG_H

#include <poller.h>
#include <driver.h>
#include <param.h>

enum wdog_hw_runnning {
	 WDOG_HW_RUNNING_UNSUPPORTED = PARAM_TRISTATE_UNKNOWN,
	 WDOG_HW_RUNNING = PARAM_TRISTATE_TRUE,
	 WDOG_HW_NOT_RUNNING = PARAM_TRISTATE_FALSE
};

struct device_node;

struct watchdog {
	int (*set_timeout)(struct watchdog *, unsigned);
	int (*ping)(struct watchdog *);
	const char *name;
	struct device *hwdev;
	struct device dev;
	unsigned int priority;
	unsigned int timeout_max;
	unsigned int timeout_cur;
	unsigned int poller_enable;
	uint64_t last_ping;
	int seconds_to_expire;
	struct poller_async poller;
	int running; /* enum wdog_hw_running */
};

/*
 * Use the following function to check whether or not the hardware watchdog
 * is running
 */
static inline int watchdog_hw_running(struct watchdog *w)
{
	if (w->running == WDOG_HW_RUNNING_UNSUPPORTED)
		return -ENOSYS;

	return w->running == WDOG_HW_RUNNING;
}

#ifdef CONFIG_WATCHDOG
int watchdog_register(struct watchdog *);
int watchdog_deregister(struct watchdog *);
struct watchdog *watchdog_get_default(void);
struct watchdog *watchdog_get_by_name(const char *name);
int watchdog_get_alias_id_from(struct watchdog *, struct device_node *);
int watchdog_set_timeout(struct watchdog*, unsigned);
int watchdog_ping(struct watchdog *);
int watchdog_inhibit_all(void);
#else
static inline int watchdog_register(struct watchdog *w)
{
	return 0;
}

static inline int watchdog_deregister(struct watchdog *w)
{
	return 0;
}

static inline struct watchdog *watchdog_get_default(void)
{
	return NULL;
}

static inline struct watchdog *watchdog_get_by_name(const char *name)
{
	return NULL;
}

static inline int watchdog_set_timeout(struct watchdog*w, unsigned t)
{
	return 0;
}

static inline int watchdog_ping(struct watchdog *w)
{
	return 0;
}

static inline int watchdog_inhibit_all(void)
{
	return -ENOSYS;
}

static inline int watchdog_get_alias_id_from(struct watchdog *wd, struct device_node *np)
{
	return -ENOSYS;
}
#endif

#define WATCHDOG_DEFAULT_PRIORITY 100

#endif /* INCLUDE_WATCHDOG_H */
