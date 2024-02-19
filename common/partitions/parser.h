/*
 * Copyright (C) 2013 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2 only
 */

#ifndef __PARTITIONS_PARSER_H__
#define __PARTITIONS_PARSER_H__

#include <block.h>
#include <filetype.h>
#include <linux/uuid.h>
#include <linux/list.h>

#define MAX_PARTITION		128
#define MAX_PARTITION_NAME	38

struct partition {
	char name[MAX_PARTITION_NAME];
	char partuuid[MAX_UUID_STR];
	uint64_t first_sec;
	uint64_t size;
	union {
		u8 dos_partition_type;
		guid_t typeuuid;
	};
	struct list_head list;
	int num;
};

struct partition_desc {
	struct list_head partitions;
};

struct partition_parser {
	struct partition_desc *(*parse)(void *buf, struct block_device *blk);
	void (*partition_free)(struct partition_desc *pd);
	enum filetype type;

	struct list_head list;
};

int partition_parser_register(struct partition_parser *p);

#endif /* __PARTITIONS_PARSER_H__ */
