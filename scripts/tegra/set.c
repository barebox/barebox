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

/*
 * set.c - State setting support for the cbootimage tool
 */

#include "set.h"
#include "cbootimage.h"
#include "crypto.h"
#include "data_layout.h"

/*
 * Function prototypes
 *
 * ParseXXX() parses XXX in the input
 * SetXXX() sets state based on the parsing results but does not perform
 *      any parsing of its own
 * A ParseXXX() function may call other parse functions and set functions.
 * A SetXXX() function may not call any parseing functions.
 */
#define DEFAULT()                                                     \
	default:                                                      \
		printf("Unexpected token %d at line %d\n",            \
		token, __LINE__);                              \
	return 1

int
read_from_image(char	*filename,
		u_int8_t	**image,
		u_int32_t	*actual_size,
		file_type	f_type)
{
	int result = 0; /* 0 = success, 1 = failure */
	FILE *fp;
	struct stat stats;

	fp = fopen(filename, "r");
	if (fp == NULL) {
		result = 1;
		return result;
	}

	if (stat(filename, &stats) != 0) {
		printf("Error: Unable to query info on bootloader path %s\n",
			filename);
		result = 1;
		goto cleanup;
	}

	*actual_size  = (u_int32_t)stats.st_size;

	if (f_type == file_type_bl && *actual_size > MAX_BOOTLOADER_SIZE) {
		printf("Error: Bootloader file %s is too large.\n",
			filename);
		result = 1;
		goto cleanup;
	}
	*image = malloc(*actual_size);
	if (*image == NULL) {
		result = 1;
		goto cleanup;
	}

	memset(*image, 0, *actual_size);

	if (fread(*image, 1, (size_t)stats.st_size, fp) != stats.st_size) {
		result = 1;
		goto cleanup;
	}

cleanup:
	fclose(fp);
	return result;
}

/*
 * Processes commands to set a bootloader.
 *
 * @param context    	The main context pointer
 * @param filename   	The file name of bootloader
 * @param load_addr  	The load address value for bootloader
 * @param entry_point	The entry point value for bootloader
 * @return 0 and 1 for success and failure
 */
int
set_bootloader(build_image_context	*context,
		char	*filename,
		u_int32_t	load_addr,
		u_int32_t	entry_point)
{
	context->newbl_filename = filename;
	context->newbl_load_addr = load_addr;
	context->newbl_entry_point = entry_point;
	return update_bl(context);
}

#define DEFAULT()                                                     \
	default:                                                      \
		printf("Unexpected token %d at line %d\n",            \
			token, __LINE__);                              \
		return 1

/*
 * General handler for setting values in config files.
 *
 * @param context	The main context pointer
 * @param token  	The parse token value
 * @param value  	The value to set
 * @return 0 for success
 */
int context_set_value(build_image_context *context,
		parse_token token,
		u_int32_t value)
{
	assert(context != NULL);

	switch (token) {
	case token_attribute:
		context->newbl_attr = value;
		break;

	case token_block_size:
		context->block_size = value;
		context->block_size_log2 = log2(value);

		if (context->memory != NULL) {
			printf("Error: Too late to change block size.\n");
			return 1;
		}

		if (value != (u_int32_t)(1 << context->block_size_log2)) {
			printf("Error: Block size must be a power of 2.\n");
			return 1;
		}
		context->pages_per_blk= 1 << (context->block_size_log2-
				context->page_size_log2);
		g_soc_config->set_value(token_block_size_log2,
			context->block_size_log2, context->bct);
		break;

	case token_partition_size:
		if (context->memory != NULL) {
			printf("Error: Too late to change block size.\n");
			return 1;
		}

		context->partition_size= value;
		g_soc_config->set_value(token_partition_size,
			value, context->bct);
		break;

	case token_page_size:
		context->page_size = value;
		context->page_size_log2 = log2(value);
		context->pages_per_blk= 1 << (context->block_size_log2-
			context->page_size_log2);

		g_soc_config->set_value(token_page_size_log2,
			context->page_size_log2, context->bct);
		break;
	case token_redundancy:
		context->redundancy = value;
		break;

	case token_version:
		context->version = value;
		break;

	case token_bct_copy:
		context->bct_copy = value;
		break;

	case token_odm_data:
		context->odm_data = value;
		break;

	case token_pre_bct_pad_blocks:
		if (context->bct_init) {
			printf("Error: Too late to pre-BCT pad.\n");
			return 1;
		}
		context->pre_bct_pad_blocks = value;
		break;

	DEFAULT();
	}

	return 0;
}
