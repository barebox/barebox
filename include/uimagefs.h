/*
 * Copyright (c) 2013 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * under GPLv2 only
 */

#ifndef __UIMAGEFS_H__
#define __UIMAGEFS_H__

#include <linux/types.h>
#include <linux/list.h>
#include <image.h>
#include <ioctl.h>

#define UIMAGEFS_METADATA	_IOR('U', 100, struct image_header)

enum uimagefs_type {
	UIMAGEFS_DATA,
	UIMAGEFS_DATA_CRC,
	UIMAGEFS_NAME,
	UIMAGEFS_TIME,
	UIMAGEFS_LOAD,
	UIMAGEFS_EP,
	UIMAGEFS_OS,
	UIMAGEFS_ARCH,
	UIMAGEFS_TYPE,
	UIMAGEFS_COMP,
};

struct uimagefs_handle_data {
	char *name;
	enum uimagefs_type type;
	uint64_t size;

	int fd;
	size_t offset; /* offset in the image */
	size_t pos; /* pos in the data */

	char *data;

	struct list_head list;
};

struct uimagefs_handle {
	struct image_header header;
	int nb_data_entries;
	char *filename;
	char *copy;

	struct list_head list;
};

#endif /* __UIMAGEFS_H__ */
