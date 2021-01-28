// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2012 NVIDIA CORPORATION

#include "cbootimage.h"
#include "data_layout.h"
#include "set.h"
#include "context.h"

void
cleanup_context(build_image_context *context)
{
	destroy_block_list(context->memory);
	free(context->bct);
}

int
init_context(build_image_context *context)
{
	/* Set defaults */
	context->memory = new_block_list();
	context->next_bct_blk = 0; /* Default to block 0 */
	context_set_value(context, token_redundancy, 1);
	context_set_value(context, token_version, 1);
	context_set_value(context, token_bct_copy, 2);

	return 0;
}
