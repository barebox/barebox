/*
 * Copyright (c) 2013 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * under GPLv2 only
 */

#ifndef __BPKFS_H__
#define __BPKFS_H__

#include <linux/types.h>
#include <linux/list.h>

#define BPKFS_TYPE_BL	0x50424c00
#define BPKFS_TYPE_BLV	0x50424c56
#define BPKFS_TYPE_DSC	0x44456343
#define BPKFS_TYPE_KER	0x504b4552
#define BPKFS_TYPE_RFS	0x50524653
#define BPKFS_TYPE_FMV	0x46575600

struct bpkfs_header {
	uint32_t magic;
	uint32_t version;
	uint64_t size;
	uint32_t crc;
	uint64_t spare;
} __attribute__ ((packed)) ;

struct bpkfs_data_header {
	uint32_t type;
	uint64_t size;
	uint32_t crc;
	uint32_t hw_id;
	uint32_t spare;
} __attribute__ ((packed)) ;

struct bpkfs_handle_hw {
	char *name;
	uint32_t hw_id;

	struct list_head list_data;
	struct list_head list_hw_id;
};

struct bpkfs_handle_data {
	char *name;
	uint32_t type;
	uint32_t hw_id;
	uint64_t size;

	int fd;
	size_t offset; /* offset in the image */
	size_t pos; /* pos in the data */
	uint32_t crc;

	char data[8];

	struct list_head list;
};

struct bpkfs_handle {
	struct bpkfs_header header;
	int nb_data_entries;
	char *filename;

	struct list_head list;
};

#endif /* __BPKFS_H__ */
