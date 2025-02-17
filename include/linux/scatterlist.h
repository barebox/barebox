/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_SCATTERLIST_H
#define _LINUX_SCATTERLIST_H

#include <linux/types.h>
#include <linux/string.h>
#include <linux/bug.h>
#include <asm/io.h>


struct scatterlist {
	void		*address;
	unsigned int	flags:2;
#define SG_CHAIN	0x01UL
#define SG_END		0x02UL
	unsigned int	length:30;
};

struct sg_table {
	struct scatterlist *sgl;	/* the list */
	unsigned int nents;		/* number of mapped entries */
	unsigned int orig_nents;	/* original size of list */
};

/*
 * Notes on SG table design.
 *
 * We use the the scatterlist struct to place the address AND encode
 * information about the sg table as well. The two flag bits are reserved
 * for this information.
 *
 * If bit 0 is set, then the address member is a pointer to the next sg
 * table list. Otherwise the next entry is at sg + 1.
 *
 * If bit 1 is set, then this sg entry is the last element in a list.
 *
 * See sg_next().
 *
 */

static inline unsigned int __sg_flags(struct scatterlist *sg)
{
	return sg->flags;
}

static inline bool sg_is_chain(struct scatterlist *sg)
{
	return __sg_flags(sg) & SG_CHAIN;
}

static inline bool sg_is_last(struct scatterlist *sg)
{
	return __sg_flags(sg) & SG_END;
}

static inline struct scatterlist *sg_chain_ptr(struct scatterlist *sg)
{
	return sg->address;
}

int sg_nents(struct scatterlist *sg);
int sg_nents_for_len(struct scatterlist *sg, u64 len);

/**
 * sg_set_buf - Set sg entry to point at given data
 * @sg:		 SG entry
 * @buf:	 Data
 * @buflen:	 Data length
 *
 **/
static inline void sg_set_buf(struct scatterlist *sg, const void *buf,
			      unsigned int buflen)
{
	sg->address = (void *)buf;
	sg->length = buflen;
}

/*
 * Loop over each sg element, following the pointer to a new list if necessary
 */
#define for_each_sg(sglist, sg, nr, __i)	\
	for (__i = 0, sg = (sglist); __i < (nr); __i++, sg = sg_next(sg))

/*
 * Loop over each sg element in the given sg_table object.
 */
#define for_each_sgtable_sg(sgt, sg, i)		\
	for_each_sg((sgt)->sgl, sg, (sgt)->orig_nents, i)

/**
 * sg_mark_end - Mark the end of the scatterlist
 * @sg:		 SG entryScatterlist
 *
 * Description:
 *   Marks the passed in sg entry as the termination point for the sg
 *   table. A call to sg_next() on this entry will return NULL.
 *
 **/
static inline void sg_mark_end(struct scatterlist *sg)
{
	/*
	 * Set termination bit, clear potential chain bit
	 */
	sg->flags |= SG_END;
	sg->flags &= ~SG_CHAIN;
}

/**
 * sg_unmark_end - Undo setting the end of the scatterlist
 * @sg:		 SG entryScatterlist
 *
 * Description:
 *   Removes the termination marker from the given entry of the scatterlist.
 *
 **/
static inline void sg_unmark_end(struct scatterlist *sg)
{
	sg->flags &= ~SG_END;
}

/**
 * sg_init_marker - Initialize markers in sg table
 * @sgl:	   The SG table
 * @nents:	   Number of entries in table
 *
 **/
static inline void sg_init_marker(struct scatterlist *sgl,
				  unsigned int nents)
{
	sg_mark_end(&sgl[nents - 1]);
}

struct scatterlist *sg_next(struct scatterlist *);
struct scatterlist *sg_last(struct scatterlist *s, unsigned int);
void sg_init_table(struct scatterlist *, unsigned int);
void sg_init_one(struct scatterlist *, const void *, unsigned int);

#endif /* _LINUX_SCATTERLIST_H */
