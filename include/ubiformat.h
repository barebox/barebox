#ifndef __UBIFORMAT_H
#define __UBIFORMAT_H

#include <linux/types.h>

struct ubiformat_args {
	unsigned int yes:1;
	unsigned int quiet:1;
	unsigned int verbose:1;
	unsigned int override_ec:1;
	unsigned int novtbl:1;
	unsigned int manual_subpage;
	int subpage_size;
	int vid_hdr_offs;
	int ubi_ver;
	uint32_t image_seq;
	long long ec;

	/*
	 * Either set image if you have a file or set image_buf / image_size
	 * if you have a buffer.
	 */
	const char *image;
	const void *image_buf;
	loff_t image_size;
};

int ubiformat(struct mtd_info *mtd, struct ubiformat_args *args);

int ubiformat_write(struct mtd_info *mtd, const void *buf, size_t count,
		    loff_t offset);

#endif /* __UBIFORMAT_H */
