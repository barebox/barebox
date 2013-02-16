/*
 * Copyright (C) 2013 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2 only
 */

#ifndef __PARTITIONS_PARSER_H__
#define __PARTITIONS_PARSER_H__

#include <block.h>
#include <filetype.h>
#include <linux/list.h>

#define MAX_PARTITION		8

struct partition {
	uint64_t first_sec;
	uint64_t size;
};

struct partition_desc {
	int used_entries;
	struct partition parts[MAX_PARTITION];
};

struct partition_parser {
	void (*parse)(void *buf, struct block_device *blk, struct partition_desc *pd);
	enum filetype type;

	struct list_head list;
};

int partition_parser_register(struct partition_parser *p);

#endif /* __PARTITIONS_PARSER_H__ */
