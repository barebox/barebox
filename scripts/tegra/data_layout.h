/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: 2012 NVIDIA CORPORATION */

/*
 * data_layout.h - Definitions for the cbootimage data layout code.
 */

#ifndef INCLUDED_DATA_LAYOUT_H
#define INCLUDED_DATA_LAYOUT_H

#include "cbootimage.h"

/* Foward declarations */
struct build_image_context_rec;

typedef struct blk_data_rec *blk_data_handle;

blk_data_handle new_block_list(void);
void destroy_block_list(blk_data_handle);

int
update_bl(struct build_image_context_rec *context);

void
update_context(struct build_image_context_rec *context);

int
read_bct_file(struct build_image_context_rec *context);

int
init_bct(struct build_image_context_rec *context);

int
write_block_raw(struct build_image_context_rec *context);

int
begin_update(build_image_context *context);

#endif /* #ifndef INCLUDED_DATA_LAYOUT_H */
