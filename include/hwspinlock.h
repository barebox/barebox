/* SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause */
/*
 * Copyright (C) 2018, STMicroelectronics - All Rights Reserved
 */

#ifndef __HWSPINLOCK_H
#define __HWSPINLOCK_H

struct hwspinlock { /* TODO to be implemented */ };

static inline int hwspinlock_get_by_index(struct device_d *dev,
					  int index,
					  struct hwspinlock *hws)
{
	return -ENOSYS;
}

static inline int hwspinlock_lock_timeout(struct hwspinlock *hws,
					  int timeout_ms)
{
	return -ENOSYS;
}

static inline int hwspinlock_unlock(struct hwspinlock *hws)
{
	return -ENOSYS;
}

struct hwspinlock_ops { /* TODO to be implemented */ };

#endif /* __HWSPINLOCK_H */
