/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __LINUX_SPINLOCK_H
#define __LINUX_SPINLOCK_H

#include <linux/cleanup.h>

typedef int   spinlock_t;
#define spin_lock_init(...)
#define spin_lock(lock)
#define spin_lock_bh spin_lock
#define spin_lock_irq spin_lock
#define spin_unlock(lock)
#define spin_unlock_bh spin_unlock
#define spin_unlock_irq spin_unlock
#define spin_lock_irqsave(lock, flags) do { flags = 0; } while (0)
#define spin_unlock_irqrestore(lock, flags) do { flags = flags; } while (0)

#define DEFINE_SPINLOCK(lock) spinlock_t __always_unused lock

typedef spinlock_t raw_spinlock_t;
#define raw_spin_lock_init spin_lock_init
#define raw_spin_lock spin_lock
#define raw_spin_unlock spin_unlock
#define raw_spin_lock_irqsave spin_lock_irqsave
#define raw_spin_unlock_irqrestore spin_unlock_irqrestore

#define DEFINE_RAW_SPINLOCK(lock) raw_spinlock_t __always_unused lock

DEFINE_DUMMY_GUARD_1(spinlock, spinlock_t,
		     spin_lock(_T->lock),
		     spin_unlock(_T->lock))

DEFINE_DUMMY_GUARD_1(spinlock_irqsave, spinlock_t,
		     spin_lock_irqsave(_T->lock, _T->flags),
		     spin_unlock_irqrestore(_T->lock, _T->flags),
		     unsigned long flags)

#endif /* __LINUX_SPINLOCK_H */
