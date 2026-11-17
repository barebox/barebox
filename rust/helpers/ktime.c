// SPDX-License-Identifier: GPL-2.0

#include <linux/ktime.h>

ktime_t rust_helper_ktime_get(void)
{
	return ktime_get();
}

