/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_MATH_H
#define _LINUX_MATH_H

#include <linux/types.h>
#include <linux/math64.h>

/*
 * This looks more complex than it should be. But we need to
 * get the type for the ~ right in round_down (it needs to be
 * as wide as the result!), and we want to evaluate the macro
 * arguments just once each.
 *
 * NOTE these functions only round to power-of-2 arguments. Use
 * roundup/rounddown for non power-of-2-arguments.
 */
#define __round_mask(x, y) ((__typeof__(x))((y)-1))
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)
#define round_down(x, y) ((x) & ~__round_mask(x, y))

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))

#define DIV_ROUND_DOWN_ULL(ll, d) \
	({ unsigned long long _tmp = (ll); do_div(_tmp, d); _tmp; })

#define DIV_ROUND_UP_ULL(ll, d)		DIV_ROUND_DOWN_ULL((ll) + (d) - 1, (d))

#define DIV_ROUND_CLOSEST(x, divisor)(			\
{							\
	typeof(divisor) __divisor = divisor;		\
	(((x) + ((__divisor) / 2)) / (__divisor));	\
}							\
)
/*
 * Same as above but for u64 dividends. divisor must be a 32-bit
 * number.
 */
#define DIV_ROUND_CLOSEST_ULL(x, divisor)(		\
{							\
	typeof(divisor) __d = divisor;			\
	unsigned long long _tmp = (x) + (__d) / 2;	\
	do_div(_tmp, __d);				\
	_tmp;						\
}							\
)

/* The `const' in roundup() prevents gcc-3.3 from calling __divdi3 */
#define roundup(x, y) (					\
{							\
	const typeof(y) __y = y;			\
	(((x) + (__y - 1)) / __y) * __y;		\
}							\
)
#define rounddown(x, y) (				\
{							\
	typeof(x) __x = (x);				\
	__x - (__x % (y));				\
}							\
)

/* Calculate "x * n / d" without unnecessary overflow or loss of precision. */
#define mult_frac(x, n, d)	\
({				\
	typeof(x) x_ = (x);	\
	typeof(n) n_ = (n);	\
	typeof(d) d_ = (d);	\
				\
	typeof(x_) q = x_ / d_;	\
	typeof(x_) r = x_ % d_;	\
	q * n_ + r * n_ / d_;	\
})

#define abs(x) ({                               \
		long __x = (x);                 \
		(__x < 0) ? -__x : __x;         \
	})

#define abs64(x) ({                             \
		s64 __x = (x);                  \
		(__x < 0) ? -__x : __x;         \
	})

#endif
