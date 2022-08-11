#ifndef __PBL_BIO_H__
#define __PBL_BIO_H__

#include <linux/types.h>

struct pbl_bio {
	void *priv;
	int (*read)(struct pbl_bio *bio, off_t block_off, void *buf, unsigned nblocks);
};

static inline int pbl_bio_read(struct pbl_bio *bio, off_t block_off,
			       void *buf, unsigned nblocks)
{
	return bio->read(bio, block_off, buf, nblocks);
}

ssize_t pbl_fat_load(struct pbl_bio *, const char *filename, void *dest, size_t len);

#endif /* __PBL_H__ */
