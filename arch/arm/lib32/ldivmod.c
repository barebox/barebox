// SPDX-License-Identifier: GPL-2.0-only
/*
 * 64-bit division for the __aeabi_uldivmod() and __aeabi_ldivmod() run-time
 * helpers. barebox C code uses do_div() and friends instead, but the Rust
 * compiler emits calls to these helpers for 64-bit division.
 */

#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/math64.h>

u64 __udivmoddi4(u64 n, u64 d, u64 *rem);
s64 __divmoddi4(s64 n, s64 d, s64 *rem);

u64 __udivmoddi4(u64 n, u64 d, u64 *rem)
{
	u64 q = 0;
	int shift;

	if (!(d >> 32)) {
		/* Division by zero ends up in __div0() like for 32-bit */
		*rem = do_div(n, (u32)d);
		return n;
	}

	if (n < d) {
		*rem = n;
		return 0;
	}

	/* d >= 2^32, so the quotient fits into 32 bits: shift and subtract */
	shift = fls64(n) - fls64(d);
	d <<= shift;

	for (; shift >= 0; shift--) {
		q <<= 1;
		if (n >= d) {
			n -= d;
			q |= 1;
		}
		d >>= 1;
	}

	*rem = n;
	return q;
}

s64 __divmoddi4(s64 n, s64 d, s64 *rem)
{
	u64 un = n < 0 ? -(u64)n : n;
	u64 ud = d < 0 ? -(u64)d : d;
	u64 ur, q;

	q = __udivmoddi4(un, ud, &ur);

	/* The remainder has the sign of the dividend */
	*rem = n < 0 ? -(s64)ur : ur;

	return (n < 0) != (d < 0) ? -(s64)q : q;
}
