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
#include <linux/sizes.h>

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
	unsigned int flags;
	unsigned int typeflags;
};

struct partition_parser;

struct partition_desc {
	struct list_head partitions;
	struct partition_parser *parser;
	struct block_device *blk;
};

struct partition_parser {
	struct partition_desc *(*parse)(void *buf, struct block_device *blk);
	void (*partition_free)(struct partition_desc *pd);
	struct partition_desc *(*create)(struct block_device *blk);
	int (*mkpart)(struct partition_desc *pd, const char *name, const char *fs_type,
		      uint64_t start, uint64_t end);
	int (*rmpart)(struct partition_desc *pd, struct partition *part);
	int (*write)(struct partition_desc *pd);
	int (*rename)(struct partition *part, const char *name);
	int (*setguid)(struct partition *part, guid_t *guid);
	enum filetype type;

	struct list_head list;

	const char *name;
};

#define PARTITION_ALIGN_SIZE	SZ_1M
#define PARTITION_ALIGN_SECTORS	(PARTITION_ALIGN_SIZE >> SECTOR_SHIFT)

void partition_desc_init(struct partition_desc *pd, struct block_device *blk);
int partition_parser_register(struct partition_parser *p);
struct partition_desc *partition_table_read(struct block_device *blk);
struct partition_desc *partition_table_new(struct block_device *blk, const char *type);
int partition_table_write(struct partition_desc *pdesc);
int partition_create(struct partition_desc *pdesc, const char *name,
		     const char *fs_type, uint64_t lba_start, uint64_t lba_end);
int partition_remove(struct partition_desc *pdesc, int num);
void partition_table_free(struct partition_desc *pdesc);
bool partition_is_free(struct partition_desc *pdesc, uint64_t start, uint64_t size);
int partition_find_free_space(struct partition_desc *pdesc, uint64_t sectors, uint64_t *start);

#endif /* __PARTITIONS_PARSER_H__ */
