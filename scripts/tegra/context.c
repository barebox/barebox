/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 */

#include "cbootimage.h"
#include "data_layout.h"
#include "set.h"

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
