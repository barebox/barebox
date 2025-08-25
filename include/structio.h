/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __STRUCTIO_H_
#define __STRUCTIO_H_

#include <bobject.h>

#ifdef CONFIG_STRUCTIO
struct bobject *structio_active(void);
int structio_run_command(struct bobject **, const char *cmd);
int structio_devinfo(struct bobject **, struct device *dev);
#else
#define structio_active() NULL
static inline int structio_run_command(struct bobject **bobj, const char *cmd)
{
	return -ENOSYS;
}

static inline int structio_devinfo(struct bobject **bobj, struct device *dev)
{
	return -ENOSYS;
}
#endif

#define stprintf(name, fmt, ...) do { \
	struct bobject *__bobj = structio_active(); \
	if (__bobj) \
		bobject_add_param_fixed(__bobj, name, fmt, ##__VA_ARGS__); \
	else \
		printf(name ": " fmt "\n", ##__VA_ARGS__); \
} while (0)

#define stprintf_prefix(name, prefix, fmt, ...) do { \
	struct bobject *__bobj = structio_active(); \
	if (__bobj) \
		bobject_add_param_fixed(__bobj, name, fmt, ##__VA_ARGS__); \
	else \
		printf(prefix fmt "\n", ##__VA_ARGS__); \
} while (0)

#define stprintf_single(name, fmt...) stprintf_prefix(name, "", fmt)

#define stnoprintf(args...) ({ structio_active() ? 0 : (printf(args), 1); })

#endif
