/*
 * (C) Copyright 2008-2010 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef __DIGEST_H__
#define __DIGEST_H__

#include <linux/list.h>

struct digest
{
	char *name;

	int (*init)(struct digest *d);
	int (*update)(struct digest *d, const void *data, unsigned long len);
	int (*final)(struct digest *d, unsigned char *md);

	unsigned int length;

	struct list_head list;
};

/*
 * digest functions
 */
int digest_register(struct digest *d);
void digest_unregister(struct digest *d);

struct digest* digest_get_by_name(char* name);

int digest_file_window(struct digest *d, char *filename,
		       unsigned char *hash,
		       ulong start, ulong size);
int digest_file(struct digest *d, char *filename,
		       unsigned char *hash);
int digest_file_by_name(char *algo, char *filename,
		       unsigned char *hash);

#endif /* __SH_ST_DEVICES_H__ */
