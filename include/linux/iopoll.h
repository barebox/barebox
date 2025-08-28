/*
 * Copyright (c) 2012-2014 The Linux Foundation. All rights reserved.
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#ifndef _LINUX_IOPOLL_H
#define _LINUX_IOPOLL_H

#include <errno.h>
#include <io.h>
#include <clock.h>
#include <pbl.h>

#if IN_PROPER
# define read_poll_get_time_ns()	get_time_ns()
# define read_poll_is_timeout(s, t)	is_timeout(s, t)
#else
# ifndef read_poll_get_time_ns
#  define read_poll_get_time_ns()	0
# endif
# ifndef read_poll_is_timeout
#  define read_poll_is_timeout(s, t)	((void)(s), (void)(t), 0)
# endif
#endif

/**
 * read_poll_timeout - Periodically poll an address until a condition is met or a timeout occurs
 * @op: accessor function (takes @addr as its only argument)
 * @val: Variable to read the value into
 * @cond: Break condition (usually involving @val)
 * @timeout_us: Timeout in us, 0 means never timeout
 * @args: arguments for @op poll
 *
 * Returns 0 on success and -ETIMEDOUT upon a timeout. In either
 * case, the last read value at @addr is stored in @val.
 *
 * When available, you'll probably want to use one of the specialized
 * macros defined below rather than this macro directly.
 *
 * We do not have timing functions in the PBL, so ignore the timeout value and
 * loop infinitely here.
 */
#define read_poll_timeout(op, val, cond, timeout_us, args...)	\
({ \
	uint64_t __timeout_us = (timeout_us); \
	uint64_t start = __timeout_us ? read_poll_get_time_ns() : 0; \
	for (;;) { \
		(val) = op(args); \
		if (cond) \
			break; \
		if (__timeout_us && \
		    read_poll_is_timeout(start, __timeout_us * USECOND)) { \
			(val) = op(args); \
			break; \
		} \
	} \
	(cond) ? 0 : -ETIMEDOUT; \
})

/**
 * readx_poll_timeout - Periodically poll an address until a condition is met or a timeout occurs
 * @op: accessor function (takes @addr as its only argument)
 * @addr: Address to poll
 * @val: Variable to read the value into
 * @cond: Break condition (usually involving @val)
 * @timeout_us: Timeout in us, 0 means never timeout
 *
 * Returns 0 on success and -ETIMEDOUT upon a timeout. In either
 * case, the last read value at @addr is stored in @val.
 *
 * When available, you'll probably want to use one of the specialized
 * macros defined below rather than this macro directly.
 *
 * We do not have timing functions in the PBL, so ignore the timeout value and
 * loop infinitely here.
 */
#define readx_poll_timeout(op, addr, val, cond, timeout_us)	\
	read_poll_timeout(op, val, cond, timeout_us, addr)

#define readb_poll_timeout(addr, val, cond, timeout_us) \
	readx_poll_timeout(readb, addr, val, cond, timeout_us)

#define readw_poll_timeout(addr, val, cond, timeout_us) \
	readx_poll_timeout(readw, addr, val, cond, timeout_us)

#define readl_poll_timeout(addr, val, cond, timeout_us) \
	readx_poll_timeout(readl, addr, val, cond, timeout_us)

#define readq_poll_timeout(addr, val, cond, timeout_us) \
	readx_poll_timeout(readq, addr, val, cond, timeout_us)

#define readb_relaxed_poll_timeout(addr, val, cond, timeout_us) \
	readx_poll_timeout(readb_relaxed, addr, val, cond, timeout_us)

#define readw_relaxed_poll_timeout(addr, val, cond, timeout_us) \
	readx_poll_timeout(readw_relaxed, addr, val, cond, timeout_us)

#define readl_relaxed_poll_timeout(addr, val, cond, timeout_us) \
	readx_poll_timeout(readl_relaxed, addr, val, cond, timeout_us)

#define readq_relaxed_poll_timeout(addr, val, cond, timeout_us) \
	readx_poll_timeout(readq_relaxed, addr, val, cond, timeout_us)

#endif /* _LINUX_IOPOLL_H */
