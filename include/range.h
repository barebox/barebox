/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _RANGE_H__
#define _RANGE_H__

#include <linux/types.h>
#include <linux/compiler.h>
#include <linux/build_bug.h>

/**
 * region_overlap_end_inclusive - check whether a pair of [start, end] ranges overlap
 *
 * @starta: start of the first range
 * @enda:   end of the first range (inclusive)
 * @startb: start of the second range
 * @endb:   end of the second range (inclusive)
 */
static inline bool region_overlap_end_inclusive(u64 starta, u64 enda,
						u64 startb, u64 endb)
{
	if (enda < startb)
		return false;
	if (endb < starta)
		return false;
	return true;
}

/**
 * region_overlap_end_exclusive - check whether a pair of [start, end) ranges overlap
 *
 * @starta: start of the first range
 * @enda:   end of the first range (exclusive)
 * @startb: start of the second range
 * @endb:   end of the second range (exclusive)
 *
 * NOTE: end of zero is always interpreted to mean including the maximum
 * value of the type.
 */
#define region_overlap_end_exclusive(starta, enda, startb, endb)	\
({									\
	u64 __starta = (starta), __enda = (enda) - 1;			\
	u64 __startb = (startb), __endb = (endb) - 1;			\
									\
	static_assert(__same_type((starta), (enda)));			\
	static_assert(__same_type((enda), (startb)));			\
	static_assert(__same_type((startb), (endb)));			\
	static_assert(((typeof(endb))-1) > 0);				\
									\
	/* Empty ranges don't overlap */				\
	(__starta <= __enda && __startb <= __endb)			\
		 ? region_overlap_end_inclusive(__starta, __enda,	\
						__startb, __endb)	\
		 : false;						\
})


/**
 * region_overlap_end - check whether a pair of [start, end] ranges overlap
 *
 * @starta: start of the first range
 * @lena:   length of the first range
 * @startb: start of the second range
 * @lenb:   length of the second range
 */
static inline bool region_overlap_size(u64 starta, u64 lena,
				       u64 startb, u64 lenb)
{
	if (!lena || !lenb)
		return false;

	return region_overlap_end_exclusive(starta, starta + lena,
					    startb, startb + lenb);
}

/**
 * region_identical - check whether a pair of [start, extend] ranges are identical
 *
 * @starta:  start of the first range
 * @extenta: end or length of the first range
 * @startb:  start of the first range
 * @extentb: end or length of the second range
 */
static inline bool region_identical(u64 starta, u64 extenta,
				    u64 startb, u64 extentb)
{
	return starta == startb && extenta == extentb;
}

#endif
