/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2021 Ahmad Fatoum, Pengutronix
 */

#ifndef __BTHREAD_H_
#define __BTHREAD_H_

#include <linux/stddef.h>
#include <linux/compiler.h>

struct bthread;

extern struct bthread *current;

struct bthread *bthread_create(void (*threadfn)(void *), void *data, const char *namefmt, ...)
	__printf(3, 4);
void bthread_cancel(struct bthread *bthread);

void bthread_schedule(struct bthread *);
void bthread_wake(struct bthread *bthread);
void bthread_suspend(struct bthread *bthread);
int bthread_should_stop(void);
void __bthread_stop(struct bthread *bthread);
void *bthread_data(struct bthread *bthread);
void bthread_info(void);
const char *bthread_name(struct bthread *bthread);
bool bthread_is_main(struct bthread *bthread);

/**
 * bthread_run - create and wake a thread.
 * @threadfn: the function to run for coming reschedule cycles
 * @data: data ptr for @threadfn.
 * @namefmt: printf-style name for the thread.
 *
 * Description: Convenient wrapper for bthread_create() followed by
 * bthread_wakeup().  Returns the bthread or NULL
 */
#define bthread_run(threadfn, data, namefmt, ...)                          \
({                                                                         \
        struct bthread *__b                                                \
                = bthread_create(threadfn, data, namefmt, ## __VA_ARGS__); \
        if (__b)							   \
                bthread_wake(__b);                                         \
        __b;                                                               \
})

#ifdef CONFIG_BTHREAD
void bthread_reschedule(void);
#else
static inline void bthread_reschedule(void)
{
}
#endif

#endif
