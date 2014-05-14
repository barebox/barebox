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
 * data_layout.c - Code to manage the layout of data in the boot device.
 *
 */

#include "data_layout.h"
#include "cbootimage.h"
#include "crypto.h"
#include "set.h"
#include "context.h"
#include "parse.h"
#include <sys/param.h>

typedef struct blk_data_rec
{
	u_int32_t blk_number;
	u_int32_t pages_used; /* pages always used starting from 0. */
	u_int8_t *data;

	/* Pointer to ECC errors? */

	struct blk_data_rec *next;
} block_data;

/* Function prototypes */
static block_data
*new_block(u_int32_t blk_number, u_int32_t block_size);
static block_data
*find_block(u_int32_t blk_number, block_data  *block_list);
static block_data
*add_block(u_int32_t blk_number, block_data **block_list,
			u_int32_t block_size);
static int
erase_block(build_image_context *context, u_int32_t blk_number);

static int
write_page(build_image_context *context,
			u_int32_t blk_number,
			u_int32_t page_number,
			u_int8_t *data);

static void
insert_padding(u_int8_t *data, u_int32_t length);

static void
write_padding(u_int8_t *data, u_int32_t length);

static int write_bct(build_image_context *context,
		u_int32_t block,
		u_int32_t bct_slot);

static void
set_bl_data(build_image_context *context,
			u_int32_t instance,
			u_int32_t start_blk,
			u_int32_t start_page,
			u_int32_t length);

static int write_bootloaders(build_image_context *context);

static void find_new_bct_blk(build_image_context *context);
static int finish_update(build_image_context *context);

u_int32_t
iceil_log2(u_int32_t a, u_int32_t b)
{
	return (a + (1 << b) - 1) >> b;
}

/* Returns the smallest power of 2 >= a */
u_int32_t
ceil_log2(u_int32_t a)
{
	u_int32_t result;

	result = log2(a);
	if ((1UL << result) < a)
		result++;

	return result;
}

static block_data *new_block(u_int32_t blk_number, u_int32_t block_size)
{
	block_data *new_block = malloc(sizeof(block_data));
	if (new_block == NULL)
		return NULL;

	new_block->blk_number = blk_number;
	new_block->pages_used = 0;
	new_block->data = malloc(block_size);
	if (new_block->data == NULL) {
		free(new_block);
		return NULL;
	}
	new_block->next = NULL;

	memset(new_block->data, 0, block_size);

	return new_block;
}

block_data *new_block_list(void)
{
	return NULL;
}

void destroy_block_list(block_data *block_list)
{
	block_data *next;

	while (block_list) {
		next = block_list->next;
		free(block_list->data);
		free(block_list);
		block_list = next;
	}
}

static block_data *find_block(u_int32_t blk_number, block_data  *block_list)
{
	while (block_list) {
		if (block_list->blk_number == blk_number)
			return block_list;

		block_list = block_list->next;
	}

	return NULL;
}

/* Returns pointer to block after adding it to block_list, if needed. */
static block_data *add_block(u_int32_t blk_number,
		block_data **block_list,
		u_int32_t block_size)
{
	block_data *block = find_block(blk_number,*block_list);
	block_data *parent;

	if (block == NULL) {
		block = new_block(blk_number, block_size);
		if (block == NULL)
			return block;

		/* Insert block into the list */
		if ((*block_list == NULL) ||
			(blk_number < (*block_list)->blk_number)) {
			block->next = *block_list;
			*block_list = block;
		} else {
			/* Search for the correct place to insert the block. */
			parent = *block_list;
			while (parent->next != NULL &&
				parent->next->blk_number < blk_number) {
				parent = parent->next;
			}

			block->next = parent->next;
			parent->next = block;
		}
	}

	return block;
}

static int
erase_block(build_image_context *context, u_int32_t blk_number)
{
	block_data   *block;

	assert(context != NULL);

	block = add_block(blk_number, &(context->memory), context->block_size);

	if (block == NULL)
		return -ENOMEM;
	if (block->data == NULL)
		return -ENOMEM;

	memset(block->data, 0, context->block_size);
	block->pages_used = 0;
	return 0;
}

static int
write_page(build_image_context *context,
	u_int32_t blk_number,
	u_int32_t page_number,
	u_int8_t *data)
{
	block_data *block;
	u_int8_t *page_ptr;

	assert(context);

	block = add_block(blk_number, &(context->memory), context->block_size);

	if (block == NULL)
		return -ENOMEM;
	if (block->data == NULL)
		return -ENOMEM;
	assert(((page_number + 1) * context->page_size)
			<= context->block_size);

	if (block->pages_used != page_number) {
		printf("Warning: Writing page in block out of order.\n");
		printf("     block=%d  page=%d\n", blk_number, page_number);
	}

	page_ptr = block->data + (page_number * context->page_size);
	memcpy(page_ptr, data, context->page_size);
	if (block->pages_used < (page_number+1))
		block->pages_used = page_number+1;
	return 0;
}

static void
insert_padding(u_int8_t *data, u_int32_t length)
{
	u_int32_t aes_blks;
	u_int32_t remaining;

	aes_blks = iceil_log2(length, NVBOOT_AES_BLOCK_SIZE_LOG2);
	remaining = (aes_blks << NVBOOT_AES_BLOCK_SIZE_LOG2) - length;

	write_padding(data + length, remaining);
}

static void
write_padding(u_int8_t *p, u_int32_t remaining)
{
	u_int8_t value = 0x80;

	while (remaining) {
		*p++ = value;
		remaining--;
		value = 0x00;
	}
}

static int
write_bct(build_image_context *context,
	u_int32_t block,
	u_int32_t bct_slot)
{
	u_int32_t pagesremaining;
	u_int32_t page;
	u_int32_t pages_per_bct;
	u_int8_t *buffer;
	u_int8_t *data;
	int err = 0;

	assert(context);

	pages_per_bct = iceil_log2(context->bct_size, context->page_size_log2);
	pagesremaining = pages_per_bct;
	page = bct_slot * pages_per_bct;

	/* Create a local copy of the BCT data */
	buffer = malloc(pages_per_bct * context->page_size);
	if (buffer == NULL)
		return -ENOMEM;
	memset(buffer, 0, pages_per_bct * context->page_size);

	memcpy(buffer, context->bct, context->bct_size);

	insert_padding(buffer, context->bct_size);

	/* Encrypt and compute hash */
	err = sign_bct(context, buffer);
	if (err != 0)
		goto fail;

	/* Write the BCT data to the storage device, picking up ECC errors */
	data = buffer;
	while (pagesremaining > 0) {
		err = write_page(context, block, page, data);
		if (err != 0)
			goto fail;
		page++;
		pagesremaining--;
		data += context->page_size;
	}
fail:
	/* Cleanup */
	free(buffer);
	return err;
}

#define SET_BL_FIELD(instance, field, value)                \
do {                                                        \
	g_soc_config->setbl_param(instance,           \
		token_bl_##field,                           \
			&(value),                           \
			context->bct);                      \
} while (0);

#define GET_BL_FIELD(instance, field, ptr)                  \
g_soc_config->getbl_param(instance,                   \
		token_bl_##field,                           \
			ptr,                                \
			context->bct);

#define COPY_BL_FIELD(from, to, field)                      \
do {                                                        \
	u_int32_t v;                                        \
	GET_BL_FIELD(from, field, &v);                      \
	SET_BL_FIELD(to,   field,  v);                      \
} while (0);

static void
set_bl_data(build_image_context *context,
		u_int32_t instance,
		u_int32_t start_blk,
		u_int32_t start_page,
		u_int32_t length)
{
	assert(context);

	SET_BL_FIELD(instance, version, context->version);
	SET_BL_FIELD(instance, start_blk, start_blk);
	SET_BL_FIELD(instance, start_page, start_page);
	SET_BL_FIELD(instance, length, length);
	SET_BL_FIELD(instance, load_addr, context->newbl_load_addr);
	SET_BL_FIELD(instance, entry_point, context->newbl_entry_point);
	SET_BL_FIELD(instance, attribute, context->newbl_attr);
}

/*
 * Load the bootloader image then update it with the information
 * from config file.
 * In the interest of expediency, all BL's allocated from bottom to top start
 * at page 0 of a block, and all BL's allocated from top to bottom end at
 * the end of a block.
 *
 * @param context		The main context pointer
 * @return 0 for success
 */
static int
write_bootloaders(build_image_context *context)
{
	u_int32_t i;
	u_int32_t j;
	u_int32_t bl_instance;
	u_int32_t bl_move_count = 0;
	u_int32_t bl_move_remaining;
	u_int32_t current_blk;
	u_int32_t current_page;
	u_int32_t  pages_in_bl;
	u_int8_t  *bl_storage; /* Holds the Bl after reading */
	u_int8_t  *buffer;	/* Holds the Bl for writing */
	u_int8_t  *src;	/* Scans through the Bl during writing */
	u_int32_t  bl_actual_size; /* In bytes */
	u_int32_t  pagesremaining;
	u_int32_t  virtual_blk;
	u_int32_t  pages_per_blk;
	u_int32_t  bl_0_version;
	u_int32_t  bl_used;
	u_int8_t  *hash_buffer;
	u_int32_t  hash_size;
	u_int32_t  bootloaders_max;
	file_type bl_filetype = file_type_bl;
	int err = 0;

	assert(context);

	pages_per_blk = 1 << (context->block_size_log2
			- context->page_size_log2);

	g_soc_config->get_value(token_hash_size,
			&hash_size, context->bct);
	g_soc_config->get_value(token_bootloaders_max,
			&bootloaders_max, context->bct);

	hash_buffer = calloc(1, hash_size);
	if (hash_buffer == NULL)
		return -ENOMEM;

	if (enable_debug) {
		printf("write_bootloaders()\n");
		printf("  redundancy = %d\n", context->redundancy);
	}

	/* Make room for the Bl(s) in the BCT. */

	/* Determine how many to move.
	 * Note that this code will count Bl[0] only if there is already
	 * a BL in the device.
	 */
	GET_BL_FIELD(0, version, &bl_0_version);
	g_soc_config->get_value(token_bootloader_used,
			&bl_used, context->bct);
	for (bl_instance = 0; bl_instance < bl_used; bl_instance++) {
		u_int32_t bl_version;
		GET_BL_FIELD(bl_instance, version, &bl_version);
		if (bl_version == bl_0_version)
			bl_move_count++;
	}

	/* Adjust the move count, if needed, to avoid overflowing the BL table.
	 * This can happen due to too much redundancy.
	 */
	bl_move_count = MIN(bl_move_count,
			bootloaders_max - context->redundancy);

	/* Move the Bl entries down. */
	bl_move_remaining = bl_move_count;
	while (bl_move_remaining > 0) {
		u_int32_t  inst_from = bl_move_remaining - 1;
		u_int32_t  inst_to   =
			bl_move_remaining + context->redundancy - 1;

		COPY_BL_FIELD(inst_from, inst_to, version);
		COPY_BL_FIELD(inst_from, inst_to, start_blk);
		COPY_BL_FIELD(inst_from, inst_to, start_page);
		COPY_BL_FIELD(inst_from, inst_to, length);
		COPY_BL_FIELD(inst_from, inst_to, load_addr);
		COPY_BL_FIELD(inst_from, inst_to, entry_point);
		COPY_BL_FIELD(inst_from, inst_to, attribute);

		g_soc_config->getbl_param(inst_from,
			token_bl_crypto_hash,
			(u_int32_t*)hash_buffer,
			context->bct);
		g_soc_config->setbl_param(inst_to,
			token_bl_crypto_hash,
			(u_int32_t*)hash_buffer,
			context->bct);
		bl_move_remaining--;
	}

	/* Read the BL into memory. */
	if (read_from_image(context->newbl_filename,
		&bl_storage,
		&bl_actual_size,
		bl_filetype) == 1) {
		printf("Error reading Bootloader %s.\n",
			context->newbl_filename);
		exit(1);
	}

	pages_in_bl = iceil_log2(bl_actual_size, context->page_size_log2);

	current_blk = context->next_bct_blk;
	current_page  = 0;
	for (bl_instance = 0; bl_instance < context->redundancy;
					bl_instance++) {

		pagesremaining = pages_in_bl;
		/* Advance to the next block if needed. */
		if (current_page > 0) {
			current_blk++;
			current_page = 0;
		}

		virtual_blk = 0;

		while (pagesremaining > 0) {
			/* Update the bad block table with relative
			  * bad blocks.
			  */
			if (virtual_blk == 0) {
				set_bl_data(context,
					bl_instance,
					current_blk,
					current_page,
					bl_actual_size);
			}

			if (pagesremaining > pages_per_blk) {
				current_blk++;
				virtual_blk++;
				pagesremaining -= pages_per_blk;
			} else {
				current_page = pagesremaining;
				pagesremaining = 0;
			}
		}
	}

	/* Scan forwards to write each copy. */
	for (bl_instance = 0; bl_instance < context->redundancy;
					bl_instance++) {

		/* Create a local copy of the BCT data */
		buffer = malloc(pages_in_bl * context->page_size);
		if (buffer == NULL)
			return -ENOMEM;

		memset(buffer, 0, pages_in_bl * context->page_size);
		memcpy(buffer, bl_storage, bl_actual_size);
		insert_padding(buffer, bl_actual_size);

		pagesremaining = pages_in_bl;

		GET_BL_FIELD(bl_instance, start_blk, &current_blk);
		GET_BL_FIELD(bl_instance, start_page,  &current_page);

		/* Encrypt and compute hash */
		sign_data_block(buffer,
				bl_actual_size,
				hash_buffer);
		g_soc_config->setbl_param(bl_instance,
				token_bl_crypto_hash,
				(u_int32_t*)hash_buffer,
				context->bct);

		/* Write the BCT data to the storage device,
		 * picking up ECC errors
		 */
		src = buffer;

		/* Write pages as we go. */
		virtual_blk = 0;
		while (pagesremaining) {
			if (current_page == 0) {
				/* Erase the block before writing into it. */
				erase_block(context, current_blk);
			}

			err = write_page(context,
				current_blk, current_page, src);
			if (err != 0)
				goto fail;
			pagesremaining--;
			src += context->page_size;
			current_page++;
			if (current_page >= pages_per_blk) {
				current_page = 0;
				current_blk++;
				virtual_blk++;
			}
			context->last_bl_blk = current_blk;
		}
		free(buffer);
	}

	g_soc_config->set_value(token_bootloader_used,
			context->redundancy + bl_move_count,
			context->bct);

	if (enable_debug) {
		for (i = 0; i < bootloaders_max; i++) {
			u_int32_t version;
			u_int32_t start_blk;
			u_int32_t start_page;
			u_int32_t length;
			u_int32_t load_addr;
			u_int32_t entry_point;

			GET_BL_FIELD(i, version,     &version);
			GET_BL_FIELD(i, start_blk,  &start_blk);
			GET_BL_FIELD(i, start_page,   &start_page);
			GET_BL_FIELD(i, length,      &length);
			GET_BL_FIELD(i, load_addr, &load_addr);
			GET_BL_FIELD(i, entry_point,  &entry_point);

			printf("%sBL[%d]: %d %04d %04d %04d 0x%08x 0x%08x k=",
				i < bl_used ? "  " : "**",
				i,
				version,
				start_blk,
				start_page,
				length,
				load_addr,
				entry_point);

			g_soc_config->getbl_param(i,
				token_bl_crypto_hash,
				(u_int32_t*)hash_buffer,
				context->bct);
			for (j = 0; j < hash_size / 4; j++) {
				printf("%08x",
					*((u_int32_t*)(hash_buffer + 4*j)));
			}
			printf("\n");
		}
	}
	free(bl_storage);
	free(hash_buffer);
	return 0;

fail:
	/* Cleanup. */
	free(buffer);
	free(bl_storage);
	free(hash_buffer);
	printf("Write bootloader failed, error: %d.\n", err);
	return err;
}

void
update_context(struct build_image_context_rec *context)
{
	g_soc_config->get_value(token_partition_size,
			&context->partition_size,
			context->bct);
	g_soc_config->get_value(token_page_size_log2,
			&context->page_size_log2,
			context->bct);
	g_soc_config->get_value(token_block_size_log2,
			&context->block_size_log2,
			context->bct);
	g_soc_config->get_value(token_odm_data,
			&context->odm_data,
			context->bct);

	context->page_size = 1 << context->page_size_log2;
	context->block_size = 1 << context->block_size_log2;
	context->pages_per_blk = 1 << (context->block_size_log2 -
                                           context->page_size_log2);
}

/*
 * Allocate and initialize the memory for bct data.
 *
 * @param context		The main context pointer
 * @return 0 for success
 */
int
init_bct(struct build_image_context_rec *context)
{
	/* Allocate space for the bct.	 */
	context->bct = malloc(context->bct_size);

	if (context->bct == NULL)
		return -ENOMEM;

	memset(context->bct, 0, context->bct_size);
	context->bct_init = 1;

	return 0;
}

/*
 * Read the bct data from given file to allocated memory.
 * Assign the global parse interface to corresponding hardware interface
 *   according to the boot data version in bct file.
 *
 * @param context		The main context pointer
 * @return 0 for success
 */
int
read_bct_file(struct build_image_context_rec *context)
{
	u_int8_t  *bct_storage; /* Holds the Bl after reading */
	u_int32_t  bct_actual_size; /* In bytes */
	file_type bct_filetype = file_type_bct;
	int err = 0;

	if (read_from_image(context->bct_filename,
		&bct_storage,
		&bct_actual_size,
		bct_filetype) == 1) {
		printf("Error reading bct file %s.\n", context->bct_filename);
		exit(1);
	}
	context->bct_size = bct_actual_size;
	if (context->bct_init != 1)
		err = init_bct(context);
	if (err != 0) {
		printf("Context initialization failed.  Aborting.\n");
		return err;
	}
	memcpy(context->bct, bct_storage, context->bct_size);
	free(bct_storage);

	/* get proper soc_config pointer by polling each supported chip */
	if (if_bct_is_t20_get_soc_config(context, &g_soc_config))
		return 0;
	if (if_bct_is_t30_get_soc_config(context, &g_soc_config))
		return 0;
	if (if_bct_is_t114_get_soc_config(context, &g_soc_config))
		return 0;
	if (if_bct_is_t124_get_soc_config(context, &g_soc_config))
		return 0;

	return ENODATA;
}
/*
 * Update the next_bct_blk and make it point to the next
 * new blank block according to bct_copy given.
 *
 * @param context		The main context pointer
 */
static void
find_new_bct_blk(build_image_context *context)
{
	u_int32_t max_bct_search_blks;

	assert(context);

	g_soc_config->get_value(token_max_bct_search_blks,
			&max_bct_search_blks, context->bct);

	if (context->next_bct_blk > max_bct_search_blks) {
		printf("Error: Unable to locate a journal block.\n");
		exit(1);
	}
	context->next_bct_blk++;
}

/*
 * Initialization before bct and bootloader update.
 * Find the new blank block and erase it.
 *
 * @param context		The main context pointer
 * @return 0 for success
 */
int
begin_update(build_image_context *context)
{
	u_int32_t hash_size;
	u_int32_t reserved_size;
	u_int32_t reserved_offset;
	int err = 0;
	int i;

	assert(context);

	/* Ensure that the BCT block & page data is current. */
	if (enable_debug) {
		u_int32_t block_size_log2;
		u_int32_t page_size_log2;

		g_soc_config->get_value(token_block_size_log2,
			&block_size_log2, context->bct);
		g_soc_config->get_value(token_page_size_log2,
			&page_size_log2, context->bct);

		printf("begin_update(): bct data: b=%d p=%d\n",
			block_size_log2, page_size_log2);
	}

	g_soc_config->set_value(token_boot_data_version,
			context->boot_data_version, context->bct);
	g_soc_config->get_value(token_hash_size,
			&hash_size, context->bct);
	g_soc_config->get_value(token_reserved_size,
			&reserved_size, context->bct);
	g_soc_config->get_value(token_reserved_offset,
			&reserved_offset, context->bct);
	/* Set the odm data */
	g_soc_config->set_value(token_odm_data,
			context->odm_data, context->bct);

	/* Initialize the bad block table field. */
	g_soc_config->init_bad_block_table(context);
	/* Fill the reserved data w/the padding pattern. */
	write_padding(context->bct + reserved_offset, reserved_size);

	/* Create the pad before the BCT starting at block 1 */
	for (i = 0; i < context->pre_bct_pad_blocks; i++) {
		find_new_bct_blk(context);
		err = erase_block(context, i);
		if (err != 0)
			goto fail;
	}
	/* Find the next bct block starting at block pre_bct_pad_blocks. */
	for (i = 0; i < context->bct_copy; i++) {
		find_new_bct_blk(context);
		err = erase_block(context, i + context->pre_bct_pad_blocks);
		if (err != 0)
			goto fail;
	}
	return 0;
fail:
	printf("Erase block failed, error: %d.\n", err);
	return err;
}

/*
 * Write the BCT(s) starting at slot 0 of block context->pre_bct_pad_blocks.
 *
 * @param context		The main context pointer
 * @return 0 for success
 */
static int
finish_update(build_image_context *context)
{
	int err = 0;
	int i;

	for (i = 0; i < context->bct_copy; i++) {
		err = write_bct(context, i + context->pre_bct_pad_blocks, 0);
		if (err != 0)
			goto fail;
	}

	return 0;
fail:
	printf("Write BCT failed, error: %d.\n", err);
	return err;
}

/*
 * For now, ignore end state.
 */
int
update_bl(build_image_context *context)
{
	if (enable_debug)
		printf("**update_bl()\n");

	if (begin_update(context) != 0)
		return 1;
	if (write_bootloaders(context) != 0)
		return 1;
	if (finish_update(context) != 0)
		return 1;
	return 0;
}

/*
 * To write the current image:
 *   Loop over all blocks in the block data list:
 *     Write out the data of real blocks.
 *     Write out 0's for unused blocks.
 *     Stop on the last used page of the last used block.
 *
 * @param context		The main context pointer
 * @return 0 for success
 */
int
write_block_raw(build_image_context *context)
{
	block_data *block_list;
	block_data *block;
	u_int32_t blk_number;
	u_int32_t last_blk;
	u_int32_t pages_to_write;
	u_int8_t *data;
	u_int8_t *empty_blk = NULL;

	assert(context != NULL);
	assert(context->memory);

	block_list = context->memory;

	/* Compute the end of the image. */
	block_list = context->memory;
	while (block_list->next)
		block_list = block_list->next;

	last_blk = block_list->blk_number;

	/* Loop over all the storage from block 0, page 0 to
	 *last_blk, Lastpage
	 */
	for (blk_number = 0; blk_number <= last_blk; blk_number++) {
		block = find_block(blk_number, context->memory);
		if (block)	{
			pages_to_write = (blk_number == last_blk) ?
			block->pages_used :
			context->pages_per_blk;
			data = block->data;
		} else {
			/* Allocate empty_blk if needed. */
			if (empty_blk == NULL) {
				empty_blk = malloc(context->block_size);
				if (!empty_blk)
					return -ENOMEM;
				memset(empty_blk, 0, context->block_size);
			}
			pages_to_write = context->pages_per_blk;
			data = empty_blk;
		}
		/* Write the data */
		{
			size_t bytes = pages_to_write * context->page_size;

			if (fwrite(data, 1, bytes, context->raw_file) != bytes)
				return -1;
		}
	}

	free(empty_blk);
	return 0;
}
