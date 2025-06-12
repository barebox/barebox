/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 *	Berkeley style UIO structures	-	Alan Cox 1994.
 */
#ifndef __LINUX_UIO_H
#define __LINUX_UIO_H

#include <linux/kernel.h>
#include <uapi/linux/uio.h>

struct page;

typedef unsigned int __bitwise iov_iter_extraction_t;

struct kvec {
	void *iov_base; /* and that should *never* hold a userland pointer */
	size_t iov_len;
};

enum iter_type {
	/* iter types */
	ITER_KVEC,
};

#define ITER_SOURCE	1	// == WRITE
#define ITER_DEST	0	// == READ

struct iov_iter_state {
	size_t iov_offset;
	size_t count;
	unsigned long nr_segs;
};

struct iov_iter {
	u8 iter_type;
	bool nofault;
	bool data_source;
	size_t iov_offset;
	struct {
		union {
			/* use iter_iov() to get the current vec */
			const struct iovec *__iov;
			const struct kvec *kvec;
		};
		size_t count;
	};
	unsigned long nr_segs;
};

static inline const struct iovec *iter_iov(const struct iov_iter *iter)
{
	return iter->__iov;
}

static inline enum iter_type iov_iter_type(const struct iov_iter *i)
{
	return i->iter_type;
}

static inline void iov_iter_save_state(struct iov_iter *iter,
				       struct iov_iter_state *state)
{
	state->iov_offset = iter->iov_offset;
	state->count = iter->count;
	state->nr_segs = iter->nr_segs;
}

static inline bool iter_is_iovec(const struct iov_iter *i)
{
	return false;
}

static inline bool iov_iter_is_kvec(const struct iov_iter *i)
{
	return iov_iter_type(i) == ITER_KVEC;
}

/*
 * Total number of bytes covered by an iovec.
 *
 * NOTE that it is not safe to use this function until all the iovec's
 * segment lengths have been validated.  Because the individual lengths can
 * overflow a size_t when added together.
 */
static inline size_t iov_length(const struct iovec *iov, unsigned long nr_segs)
{
	unsigned long seg;
	size_t ret = 0;

	for (seg = 0; seg < nr_segs; seg++)
		ret += iov[seg].iov_len;
	return ret;
}

void iov_iter_advance(struct iov_iter *i, size_t bytes);
void iov_iter_revert(struct iov_iter *i, size_t bytes);
size_t fault_in_iov_iter_readable(const struct iov_iter *i, size_t bytes);
size_t fault_in_iov_iter_writeable(const struct iov_iter *i, size_t bytes);
size_t iov_iter_single_seg_count(const struct iov_iter *i);

__must_check size_t copy_to_iter(const void *addr, size_t bytes, struct iov_iter *i);
__must_check size_t copy_from_iter(void *addr, size_t bytes, struct iov_iter *i);
__must_check size_t copy_from_iter_nocache(void *addr, size_t bytes, struct iov_iter *i);

static __always_inline __must_check
bool copy_to_iter_full(const void *addr, size_t bytes, struct iov_iter *i)
{
	size_t copied = copy_to_iter(addr, bytes, i);
	if (likely(copied == bytes))
		return true;
	iov_iter_revert(i, copied);
	return false;
}

static __always_inline __must_check
bool copy_from_iter_full(void *addr, size_t bytes, struct iov_iter *i)
{
	size_t copied = copy_from_iter(addr, bytes, i);
	if (likely(copied == bytes))
		return true;
	iov_iter_revert(i, copied);
	return false;
}

static __always_inline __must_check
bool copy_from_iter_full_nocache(void *addr, size_t bytes, struct iov_iter *i)
{
	size_t copied = copy_from_iter_nocache(addr, bytes, i);
	if (likely(copied == bytes))
		return true;
	iov_iter_revert(i, copied);
	return false;
}

size_t iov_iter_zero(size_t bytes, struct iov_iter *);
bool iov_iter_is_aligned(const struct iov_iter *i, unsigned addr_mask,
			unsigned len_mask);
unsigned long iov_iter_alignment(const struct iov_iter *i);
unsigned long iov_iter_gap_alignment(const struct iov_iter *i);
void iov_iter_kvec(struct iov_iter *i, unsigned int direction, const struct kvec *kvec,
			unsigned long nr_segs, size_t count);
int iov_iter_npages(const struct iov_iter *i, int maxpages);

const void *dup_iter(struct iov_iter *new, struct iov_iter *old, gfp_t flags);

static inline size_t iov_iter_count(const struct iov_iter *i)
{
	return i->count;
}

/*
 * Cap the iov_iter by given limit; note that the second argument is
 * *not* the new size - it's upper limit for such.  Passing it a value
 * greater than the amount of data in iov_iter is fine - it'll just do
 * nothing in that case.
 */
static inline void iov_iter_truncate(struct iov_iter *i, u64 count)
{
	/*
	 * count doesn't have to fit in size_t - comparison extends both
	 * operands to u64 here and any value that would be truncated by
	 * conversion in assignement is by definition greater than all
	 * values of size_t, including old i->count.
	 */
	if (i->count > count)
		i->count = count;
}

/*
 * reexpand a previously truncated iterator; count must be no more than how much
 * we had shrunk it.
 */
static inline void iov_iter_reexpand(struct iov_iter *i, size_t count)
{
	i->count = count;
}

static inline int
iov_iter_npages_cap(struct iov_iter *i, int maxpages, size_t max_bytes)
{
	size_t shorted = 0;
	int npages;

	if (iov_iter_count(i) > max_bytes) {
		shorted = iov_iter_count(i) - max_bytes;
		iov_iter_truncate(i, max_bytes);
	}
	npages = iov_iter_npages(i, maxpages);
	if (shorted)
		iov_iter_reexpand(i, iov_iter_count(i) + shorted);

	return npages;
}

struct iovec *iovec_from_user(const struct iovec __user *uvector,
		unsigned long nr_segs, unsigned long fast_segs,
		struct iovec *fast_iov, bool compat);
ssize_t import_iovec(int type, const struct iovec __user *uvec,
		 unsigned nr_segs, unsigned fast_segs, struct iovec **iovp,
		 struct iov_iter *i);
ssize_t __import_iovec(int type, const struct iovec __user *uvec,
		 unsigned nr_segs, unsigned fast_segs, struct iovec **iovp,
		 struct iov_iter *i, bool compat);

struct sg_table;
ssize_t extract_iter_to_sg(struct iov_iter *iter, size_t len,
			   struct sg_table *sgtable, unsigned int sg_max,
			   iov_iter_extraction_t extraction_flags);

typedef size_t (*iov_step_f)(void *iter_base, size_t progress, size_t len,
			     void *priv, void *priv2);
typedef size_t (*iov_ustep_f)(void __user *iter_base, size_t progress, size_t len,
			      void *priv, void *priv2);

/*
 * Handle ITER_KVEC.
 */
static __always_inline
size_t iterate_kvec(struct iov_iter *iter, size_t len, void *priv, void *priv2,
		    iov_step_f step)
{
	const struct kvec *p = iter->kvec;
	size_t progress = 0, skip = iter->iov_offset;

	do {
		size_t remain, consumed;
		size_t part = min(len, p->iov_len - skip);

		if (likely(part)) {
			remain = step(p->iov_base + skip, progress, part, priv, priv2);
			consumed = part - remain;
			progress += consumed;
			skip += consumed;
			len -= consumed;
			if (skip < p->iov_len)
				break;
		}
		p++;
		skip = 0;
	} while (len);

	iter->nr_segs -= p - iter->kvec;
	iter->kvec = p;
	iter->iov_offset = skip;
	iter->count -= progress;
	return progress;
}

/**
 * iterate_and_advance2 - Iterate over an iterator
 * @iter: The iterator to iterate over.
 * @len: The amount to iterate over.
 * @priv: Data for the step functions.
 * @priv2: More data for the step functions.
 * @ustep: Function for UBUF/IOVEC iterators; given __user addresses.
 * @step: Function for other iterators; given kernel addresses.
 *
 * Iterate over the next part of an iterator, up to the specified length.  The
 * buffer is presented in segments, which for kernel iteration are broken up by
 * physical pages and mapped, with the mapped address being presented.
 *
 * Two step functions, @step and @ustep, must be provided, one for handling
 * mapped kernel addresses and the other is given user addresses which have the
 * potential to fault since no pinning is performed.
 *
 * The step functions are passed the address and length of the segment, @priv,
 * @priv2 and the amount of data so far iterated over (which can, for example,
 * be added to @priv to point to the right part of a second buffer).  The step
 * functions should return the amount of the segment they didn't process (ie. 0
 * indicates complete processsing).
 *
 * This function returns the amount of data processed (ie. 0 means nothing was
 * processed and the value of @len means processes to completion).
 */
static __always_inline
size_t iterate_and_advance2(struct iov_iter *iter, size_t len, void *priv,
			    void *priv2, iov_ustep_f ustep, iov_step_f step)
{
	if (unlikely(iter->count < len))
		len = iter->count;
	if (unlikely(!len))
		return 0;

	return iterate_kvec(iter, len, priv, priv2, step);
}


/**
 * iterate_and_advance - Iterate over an iterator
 * @iter: The iterator to iterate over.
 * @len: The amount to iterate over.
 * @priv: Data for the step functions.
 * @ustep: Function for UBUF/IOVEC iterators; given __user addresses.
 * @step: Function for other iterators; given kernel addresses.
 *
 * As iterate_and_advance2(), but priv2 is always NULL.
 */
static __always_inline
size_t iterate_and_advance(struct iov_iter *iter, size_t len, void *priv,
			   iov_ustep_f ustep, iov_step_f step)
{
	return iterate_and_advance2(iter, len, priv, NULL, ustep, step);
}

#endif
