// SPDX-License-Identifier: GPL-2.0-only
#include <linux/export.h>
#include <linux/uio.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include <linux/scatterlist.h>
#include <linux/uaccess.h>

#define MAX_RW_COUNT (INT_MAX & PAGE_MASK)

static __always_inline
size_t copy_to_user_iter(void __user *iter_to, size_t progress,
			 size_t len, void *from, void *priv2)
{
	if (access_ok(iter_to, len)) {
		from += progress;
		len = raw_copy_to_user(iter_to, from, len);
	}
	return len;
}

static __always_inline
size_t copy_from_user_iter(void __user *iter_from, size_t progress,
			   size_t len, void *to, void *priv2)
{
	size_t res = len;

	if (access_ok(iter_from, len)) {
		to += progress;
		res = raw_copy_from_user(to, iter_from, len);
	}
	return res;
}

static __always_inline
size_t memcpy_to_iter(void *iter_to, size_t progress,
		      size_t len, void *from, void *priv2)
{
	memcpy(iter_to, from + progress, len);
	return 0;
}

static __always_inline
size_t memcpy_from_iter(void *iter_from, size_t progress,
			size_t len, void *to, void *priv2)
{
	memcpy(to + progress, iter_from, len);
	return 0;
}

static __always_inline
size_t zero_to_user_iter(void __user *iter_to, size_t progress,
			 size_t len, void *priv, void *priv2)
{
	return clear_user(iter_to, len);
}

static __always_inline
size_t zero_to_iter(void *iter_to, size_t progress,
		    size_t len, void *priv, void *priv2)
{
	memset(iter_to, 0, len);
	return 0;
}

static void iov_iter_iovec_advance(struct iov_iter *i, size_t size)
{
	const struct iovec *iov, *end;

	if (!i->count)
		return;
	i->count -= size;

	size += i->iov_offset; // from beginning of current segment
	for (iov = iter_iov(i), end = iov + i->nr_segs; iov < end; iov++) {
		if (likely(size < iov->iov_len))
			break;
		size -= iov->iov_len;
	}
	i->iov_offset = size;
	i->nr_segs -= iov - iter_iov(i);
	i->__iov = iov;
}

void iov_iter_advance(struct iov_iter *i, size_t size)
{
	if (unlikely(i->count < size))
		size = i->count;
	if (likely(iter_is_iovec(i) || iov_iter_is_kvec(i))) {
		/* iovec and kvec have identical layouts */
		iov_iter_iovec_advance(i, size);
	}
}
EXPORT_SYMBOL(iov_iter_advance);

void iov_iter_revert(struct iov_iter *i, size_t unroll)
{
	if (!unroll)
		return;
	if (WARN_ON(unroll > MAX_RW_COUNT))
		return;
	i->count += unroll;
	if (unroll <= i->iov_offset) {
		i->iov_offset -= unroll;
		return;
	}
	unroll -= i->iov_offset;
	/* same logics for iovec and kvec */
	const struct iovec *iov = iter_iov(i);
	while (1) {
		size_t n = (--iov)->iov_len;
		i->nr_segs++;
		if (unroll <= n) {
			i->__iov = iov;
			i->iov_offset = n - unroll;
			return;
		}
		unroll -= n;
	}
}
EXPORT_SYMBOL(iov_iter_revert);

/*
 * Return the count of just the current iov_iter segment.
 */
size_t iov_iter_single_seg_count(const struct iov_iter *i)
{
	if (i->nr_segs > 1) {
		if (likely(iter_is_iovec(i) || iov_iter_is_kvec(i)))
			return min(i->count, iter_iov(i)->iov_len - i->iov_offset);
	}
	return i->count;
}
EXPORT_SYMBOL(iov_iter_single_seg_count);

void iov_iter_kvec(struct iov_iter *i, unsigned int direction,
			const struct kvec *kvec, unsigned long nr_segs,
			size_t count)
{
	*i = (struct iov_iter){
		.iter_type = ITER_KVEC,
		.data_source = direction,
		.kvec = kvec,
		.nr_segs = nr_segs,
		.iov_offset = 0,
		.count = count
	};
}
EXPORT_SYMBOL(iov_iter_kvec);

static bool iov_iter_aligned_iovec(const struct iov_iter *i, unsigned addr_mask,
				   unsigned len_mask)
{
	const struct iovec *iov = iter_iov(i);
	size_t size = i->count;
	size_t skip = i->iov_offset;

	do {
		size_t len = iov->iov_len - skip;

		if (len > size)
			len = size;
		if (len & len_mask)
			return false;
		if ((unsigned long)(iov->iov_base + skip) & addr_mask)
			return false;

		iov++;
		size -= len;
		skip = 0;
	} while (size);

	return true;
}

/**
 * iov_iter_is_aligned() - Check if the addresses and lengths of each segments
 * 	are aligned to the parameters.
 *
 * @i: &struct iov_iter to restore
 * @addr_mask: bit mask to check against the iov element's addresses
 * @len_mask: bit mask to check against the iov element's lengths
 *
 * Return: false if any addresses or lengths intersect with the provided masks
 */
bool iov_iter_is_aligned(const struct iov_iter *i, unsigned addr_mask,
			 unsigned len_mask)
{
	if (likely(iter_is_iovec(i) || iov_iter_is_kvec(i)))
		return iov_iter_aligned_iovec(i, addr_mask, len_mask);

	return true;
}
EXPORT_SYMBOL_GPL(iov_iter_is_aligned);

static unsigned long iov_iter_alignment_iovec(const struct iov_iter *i)
{
	const struct iovec *iov = iter_iov(i);
	unsigned long res = 0;
	size_t size = i->count;
	size_t skip = i->iov_offset;

	do {
		size_t len = iov->iov_len - skip;
		if (len) {
			res |= (unsigned long)iov->iov_base + skip;
			if (len > size)
				len = size;
			res |= len;
			size -= len;
		}
		iov++;
		skip = 0;
	} while (size);
	return res;
}

unsigned long iov_iter_alignment(const struct iov_iter *i)
{
	/* iovec and kvec have identical layouts */
	if (likely(iter_is_iovec(i) || iov_iter_is_kvec(i)))
		return iov_iter_alignment_iovec(i);

	return 0;
}
EXPORT_SYMBOL(iov_iter_alignment);

size_t copy_to_iter(const void *addr, size_t bytes, struct iov_iter *i)
{
	if (WARN_ON_ONCE(i->data_source))
		return 0;
	return iterate_and_advance(i, bytes, (void *)addr,
				   copy_to_user_iter, memcpy_to_iter);
}
EXPORT_SYMBOL(copy_to_iter);

size_t copy_from_iter(void *addr, size_t bytes, struct iov_iter *i)
{
	if (WARN_ON_ONCE(!i->data_source))
		return 0;

	return iterate_and_advance(i, bytes, addr,
				   copy_from_user_iter, memcpy_from_iter);
}
EXPORT_SYMBOL(copy_from_iter);
