/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __DEEP_PROBE_H
#define __DEEP_PROBE_H

#include <linux/stringify.h>
#include <linux/compiler_types.h>
#include <linux/types.h>

struct deep_probe_entry {
	const struct of_device_id *device_id;
};

#ifdef CONFIG_OFDEVICE
bool deep_probe_is_supported(void);
#else
static inline bool deep_probe_is_supported(void)
{
	return false;
}
#endif

extern struct deep_probe_entry __barebox_deep_probe_start;
extern struct deep_probe_entry __barebox_deep_probe_end;

#define __BAREBOX_DEEP_PROBE_ENABLE(_entry,_device_id)			\
	static const struct deep_probe_entry _entry			\
	__ll_elem(.barebox_deep_probe_##_entry) = {			\
		.device_id = _device_id,				\
	}

#define BAREBOX_DEEP_PROBE_ENABLE(_device_id)	\
	__BAREBOX_DEEP_PROBE_ENABLE(__UNIQUE_ID(deepprobe),_device_id)

#endif /* __DEEP_PROBE_H */
