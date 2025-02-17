/*
 * Copyright (c) 2014-2022, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef FIPTOOL_H
#define FIPTOOL_H

#include <linux/uuid.h>
#include <fip.h>
#include <linux/list.h>

enum {
	DO_UNSPEC = 0,
	DO_PACK   = 1,
	DO_UNPACK = 2,
	DO_REMOVE = 3
};

struct fip_image_desc {
	uuid_t             uuid;
	char              *name;
	char              *cmdline_name;
	int                action;
	char              *action_arg;
	struct fip_image  *image;
	struct list_head  list;
};

struct fip_image {
	struct fip_toc_entry toc_e;
	void                *buffer;
	bool                 buf_no_free;
};

struct fip_state {
	struct list_head descs;
	size_t nr_image_descs;
	int verbose;
	void *buffer;
};

#define pr_verbose(...) do { \
	if (fip->verbose) { \
		pr_info(__VA_ARGS__); \
	} else { \
		pr_debug(__VA_ARGS__); \
	} \
} while (0)

struct fip_state *fip_new(void);
void fip_free(struct fip_state *fip);

struct fip_image_desc *fip_new_image_desc(const uuid_t *uuid,
			     const char *name, const char *cmdline_name);

void fip_set_image_desc_action(struct fip_image_desc *desc, int action,
    const char *arg);

void fip_free_image_desc(struct fip_image_desc *desc);

void fip_add_image_desc(struct fip_state *fip, struct fip_image_desc *desc);

void fip_fill_image_descs(struct fip_state *fip);

struct fip_image_desc *fip_lookup_image_desc_from_uuid(struct fip_state *fip,
						 const uuid_t *uuid);

struct fip_image_desc *fip_lookup_image_desc_from_opt(struct fip_state *fip, char **arg);

int fip_parse(struct fip_state *fip,
		     const char *filename, fip_toc_header_t *toc_header_out);

int fip_pack_images(struct fip_state *fip,
		const char *filename,
		uint64_t toc_flags, unsigned long align);

int fip_update(struct fip_state *fip);

#define TOC_HEADER_SERIAL_NUMBER 0x12345678

typedef struct toc_entry {
	char         *name;
	uuid_t        uuid;
	char         *cmdline_name;
} toc_entry_t;

extern toc_entry_t toc_entries[];
extern toc_entry_t plat_def_toc_entries[];

#define fip_for_each_desc(fip, e) \
        list_for_each_entry(e, &(fip)->descs, list)

#define fip_for_each_desc_safe(fip, e, tmp) \
        list_for_each_entry_safe(e, tmp, &(fip)->descs, list)

struct fip_state *fip_image_open(const char *filename, size_t offset);

#endif /* FIPTOOL_H */
