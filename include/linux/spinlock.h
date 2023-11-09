/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __LINUX_SPINLOCK_H
#define __LINUX_SPINLOCK_H

typedef int   spinlock_t;
#define spin_lock_init(...)
#define spin_lock(...)
#define spin_lock_bh spin_lock
#define spin_unlock(...)
#define spin_unlock_bh spin_unlock
#define spin_lock_irqsave(lock, flags) do { flags = 0; } while (0)
#define spin_unlock_irqrestore(lock, flags) do { flags = flags; } while (0)

#define DEFINE_SPINLOCK(lock) spinlock_t __always_unused lock

#endif /* __LINUX_SPINLOCK_H */
