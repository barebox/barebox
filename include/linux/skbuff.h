/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Simple NET_SKBUFF_DATA_USES_OFFSET implementation with tail and
 * headroom handling.
 * If you don't require that, you don't need to use this.
 */

#ifndef _LINUX_SKBUFF_H
#define _LINUX_SKBUFF_H

#include <linux/list.h>
#include <linux/err.h>
#include <malloc.h>
#include <string.h>

#define sk_buff_head			list_head

struct eth_device;
struct sk_buff;

/* ---------- head
 *  headroom
 * ---------- data
 * ^
 * |
 * l
 * e   Data
 * n
 * |
 * v
 * ----------- &head[tail]
 *  tailroom
 * ----------- &head[end]
 */

struct sk_buff {
	struct list_head list;
	struct sk_buff *cloned_from;
	u8 *head;	/* never incremented */
	u8 *data;
	u16 tail;
	u16 end;	/* never decremented */
	u16 len;
	u16 refcnt;
};

struct sk_buff *dev_alloc_skb(unsigned int size);


/**
 *	skb_reserve - adjust headroom
 *	@skb: buffer to alter
 *	@len: bytes to move
 *
 *	Increase the headroom of an empty &sk_buff by reducing the tail
 *	room. This is only allowed for an empty buffer.
 */
static inline void skb_reserve(struct sk_buff *skb, int len)
{
	skb->data += len;
	skb->tail += len;
}

void *skb_put(struct sk_buff *skb, unsigned len);
void *skb_push(struct sk_buff *skb, unsigned len);
void *skb_pull(struct sk_buff *skb, unsigned len);
void skb_trim(struct sk_buff *skb, unsigned int len);
struct sk_buff *skb_copy_expand(const struct sk_buff *skb,
				int newheadroom, int newtailroom);
struct sk_buff *skb_clone(struct sk_buff *skb);
static inline bool skb_cloned(struct sk_buff *skb)
{
	return skb->cloned_from != NULL;
}

static inline u8 *skb_end_pointer(struct sk_buff *skb)
{
	return skb->head + skb->end;
}

static inline u8 *skb_tail_pointer(struct sk_buff *skb)
{
	return skb->head + skb->tail;
}

static inline u16 skb_headroom(struct sk_buff *skb)
{
	return skb->data - skb->head;
}

static inline u16 skb_tailroom(struct sk_buff *skb)
{
	return skb->end - skb->tail;
}

static inline void skb_reset_tail_pointer(struct sk_buff *skb)
{
	skb->tail = skb->data - skb->head;
}

static inline void skb_set_tail_pointer(struct sk_buff *skb, const int offset)
{
	skb_reset_tail_pointer(skb);
	skb->tail += offset;
}

static inline void __skb_set_length(struct sk_buff *skb, unsigned int len)
{
	skb->len = len;
	skb_set_tail_pointer(skb, len);
}

static inline void __skb_clear(struct sk_buff *skb)
{
	skb->data = skb->head;
	__skb_set_length(skb, 0);
}

/**
 *	skb_copy_bits - copy bits from skb to kernel buffer
 *	@skb: source skb
 *	@offset: offset in source
 *	@to: destination buffer
 *	@len: number of bytes to copy
 *
 *	Copy the specified number of bytes from the source skb to the
 *	destination buffer.
 */
static inline void skb_copy_bits(const struct sk_buff *skb, int offset, void *to, int len)
{
	memcpy(to, skb->data + offset, len);
}

#define skb_queue_tail(head, skb)	list_add_tail(&(skb)->list, head)

void dev_kfree_skb_any(struct sk_buff *skb);
#define dev_consume_skb_any(ptr)	dev_kfree_skb_any(ptr)

#endif	/* _LINUX_SKBUFF_H */
