// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *	Authors:
 *		Alan Cox, <gw4pts@gw4pts.ampr.org>
 *		Florian La Roche, <rzsfl@rz.uni-sb.de>
 *
 * Simple skb implementation with tail and headroom handling.
 * If you don't require that, you don't need to use this.
 */

#include <linux/skbuff.h>
#include <xfuncs.h>
#include <linux/compiler.h>
#include <linux/bug.h>
#include <dma.h>
#include <net.h>

struct sk_buff *dev_alloc_skb(unsigned int size)
{
	struct sk_buff *skb;

	skb = malloc(sizeof(*skb));
	if (!skb)
		return NULL;

	skb->head = dma_try_alloc(size);
	if (!skb->head) {
		free(skb);
		return NULL;
	}

	__skb_clear(skb);
	skb->end = size;
	skb->cloned_from = NULL;
	skb->refcnt = 1;

	return skb;
}

/**
 *	skb_clone	-	duplicate an sk_buff
 *	@skb: buffer to clone
 *
 *	Duplicate an &sk_buff. Both copies share the same packet data
 *	but not structure.
 */
struct sk_buff *skb_clone(struct sk_buff *skb)
{
	struct sk_buff *new;

	new = memdup(skb, sizeof(*skb));
	if (!new)
		return NULL;

	new->cloned_from = skb;
	new->refcnt = 1;
	skb->refcnt++;

	return new;
}

/**
 *	skb_copy_expand	-	copy and expand sk_buff
 *	@skb: buffer to copy
 *	@newheadroom: new free bytes at head
 *	@newtailroom: new free bytes at tail
 *
 *	Make a copy of both an &sk_buff and its data and while doing so
 *	allocate additional space.
 *
 *	This is used when the caller wishes to modify the data and needs a
 *	private copy of the data to alter as well as more space for new fields.
 *	Returns %NULL on failure or the pointer to the buffer
 *	on success.
 */
struct sk_buff *skb_copy_expand(const struct sk_buff *skb,
				int newheadroom, int newtailroom)
{
	struct sk_buff *new;

	new = malloc(sizeof(*new));
	if (!new)
		return NULL;

	new->end = newheadroom + skb->len + newtailroom;
	new->head = dma_try_alloc(new->end);
	if (!new->head) {
		free(new);
		return NULL;
	}

	__skb_clear(new);
	new->cloned_from = NULL;
	new->refcnt = 1;

	skb_reserve(new, newheadroom);
	skb_copy_bits(skb, 0, skb_put(new, skb->len), skb->len);

	return new;
}

void dev_kfree_skb_any(struct sk_buff *skb)
{
	if (unlikely(!skb))
		return;

	if (--skb->refcnt)
		return;

	if (skb_cloned(skb))
		dev_kfree_skb_any(skb->cloned_from);
	else
		free(skb->head);
	free(skb);
}

/**
 *	skb_put - add data to a buffer
 *	@skb: buffer to use
 *	@len: amount of data to add
 *
 *	This function extends the used data area of the buffer. If this would
 *	exceed the total buffer size the kernel will panic. A pointer to the
 *	first byte of the extra data is returned.
 */
void *skb_put(struct sk_buff *skb, unsigned len)
{
	void *tmp = skb_tail_pointer(skb);

	skb->tail += len;
	skb->len += len;

	BUG_ON(skb->tail > skb->end);

	return tmp;
}

/**
 *	skb_push - add data to the start of a buffer
 *	@skb: buffer to use
 *	@len: amount of data to add
 *
 *	This function extends the used data area of the buffer at the buffer
 *	start. If this would exceed the total buffer headroom the kernel will
 *	panic. A pointer to the first byte of the extra data is returned.
 */
void *skb_push(struct sk_buff *skb, unsigned len)
{
	skb->data -= len;
	skb->len += len;

	BUG_ON(skb->data < skb->head);

	return skb->data;
}

/**
 *	skb_pull - remove data from the start of a buffer
 *	@skb: buffer to use
 *	@len: amount of data to remove
 *
 *	This function removes data from the start of a buffer, returning
 *	the memory to the headroom. A pointer to the next data in the buffer
 *	is returned. Once the data has been pulled future pushes will overwrite
 *	the old data.
 */
void *skb_pull(struct sk_buff *skb, unsigned len)
{
	BUG_ON(skb->len < len);
	skb->len -= len;
	return skb->data += len;
}

/**
 *	skb_trim - remove end from a buffer
 *	@skb: buffer to alter
 *	@len: new length
 *
 *	Cut the length of a buffer down by removing data from the tail. If
 *	the buffer is already under the length specified it is not modified.
 *	The skb must be linear.
 */
void skb_trim(struct sk_buff *skb, unsigned int len)
{
	if (skb->len > len)
		__skb_set_length(skb, len);
}
