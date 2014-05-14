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
#include "context.h"
#include "parse.h"
#include "t20/nvboot_bct_t20.h"
#include <string.h>

int enable_debug;
cbootimage_soc_config * g_soc_config;

typedef struct {
	parse_token id;
	char const * message;
} value_data;

static value_data const values[] = {
	{ token_boot_data_version,   "Version       = 0x%08x;\n" },
	{ token_block_size_log2,     "BlockSize     = 0x%08x;\n" },
	{ token_page_size_log2,      "PageSize      = 0x%08x;\n" },
	{ token_partition_size,      "PartitionSize = 0x%08x;\n" },
	{ token_odm_data,            "OdmData       = 0x%08x;\n\n" },
	{ token_bootloader_used,     "# Bootloader used       = %d;\n" },
	{ token_bootloaders_max,     "# Bootloaders max       = %d;\n" },
	{ token_bct_size,            "# BCT size              = %d;\n" },
	{ token_hash_size,           "# Hash size             = %d;\n" },
	{ token_crypto_offset,       "# Crypto offset         = %d;\n" },
	{ token_crypto_length,       "# Crypto length         = %d;\n" },
	{ token_max_bct_search_blks, "# Max BCT search blocks = %d;\n" },
};

static value_data const bl_values[] = {
	{ token_bl_version,     "Version      = 0x%08x;\n" },
	{ token_bl_start_blk,   "Start block  = %d;\n" },
	{ token_bl_start_page,  "Start page   = %d;\n" },
	{ token_bl_length,      "Length       = %d;\n" },
	{ token_bl_load_addr,   "Load address = 0x%08x;\n" },
	{ token_bl_entry_point, "Entry point  = 0x%08x;\n" },
	{ token_bl_attribute,   "Attributes   = 0x%08x;\n" },
};

/*****************************************************************************/
static void usage(void)
{
	printf("Usage: bct_dump bctfile\n");
	printf("  bctfile   BCT filename to read and display\n");
}
/*****************************************************************************/
static int max_width(field_item const * table)
{
	int width = 0;
	int i;

	for (i = 0; table[i].name != NULL; ++i) {
		int length = strlen(table[i].name);

		if (width < length)
			width = length;
	}

	return width;
}
/*****************************************************************************/
static enum_item const * find_enum_item(build_image_context *context,
					enum_item const * table,
					u_int32_t value)
{
	int i;

	for (i = 0; table[i].name != NULL; ++i) {
		if (table[i].value == value)
			return table + i;
	}

	return NULL;
}
/*****************************************************************************/
static void display_enum_value(build_image_context *context,
			       enum_item const * table,
			       u_int32_t value)
{
	enum_item const * e_item = find_enum_item(context, table, value);

	if (e_item)
		printf("%s", e_item->name);
	else
		printf("<UNKNOWN ENUM VALUE (%d)>", value);
}
/*****************************************************************************/
static int display_field_value(build_image_context *context,
			       field_item const * item,
			       u_int32_t value)
{
	switch (item->type) {
		case field_type_enum:
			display_enum_value(context, item->enum_table, value);
			break;

		case field_type_u32:
			printf("0x%08x", value);
			break;

		case field_type_u8:
			printf("%d", value);
			break;

		default:
			printf("<UNKNOWN FIELD TYPE (%d)>", item->type);
			return 1;
	}

	return 0;
}
/*****************************************************************************/
int main(int argc, char *argv[])
{
	int e;
	build_image_context context;
	u_int32_t bootloaders_used;
	u_int32_t parameters_used;
	u_int32_t sdram_used;
	nvboot_dev_type type;
	u_int32_t data;
	int i;
	int j;

	if (argc != 2)
		usage();

	memset(&context, 0, sizeof(build_image_context));
	context.bct_filename = argv[1];

	e = read_bct_file(&context);
	if (e != 0)
		return e;

	/* Display root values */
	for (i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
		e = g_soc_config->get_value(values[i].id,
						&data,
						context.bct);

		if (e != 0)
			data = -1;
		else if (values[i].id == token_block_size_log2 ||
			 values[i].id == token_page_size_log2)
			data = 1 << data;

		printf(values[i].message, data);
	}

	/* Display bootloader values */
	e = g_soc_config->get_value(token_bootloader_used,
				     &bootloaders_used,
				     context.bct);

	if ((e == 0) && (bootloaders_used > 0)) {
		int bl_count = sizeof(bl_values) / sizeof(bl_values[0]);

		printf("#\n"
		       "# These values are set by cbootimage using the\n"
		       "# bootloader provided by the Bootloader=...\n"
		       "# configuration option.\n"
		       "#\n");

		for (i = 0; i < bootloaders_used; ++i) {
			for (j = 0; j < bl_count; ++j) {
				e = g_soc_config->getbl_param(i,
							       bl_values[j].id,
							       &data,
							       context.bct);
				printf("# Bootloader[%d].", i);

				if (e != 0)
					data = -1;

				printf(bl_values[j].message, data);
			}
		}
	}

	/* Display flash device parameters */
	e = g_soc_config->get_value(token_num_param_sets,
				     &parameters_used,
				     context.bct);

	for (i = 0; (e == 0) && (i < parameters_used); ++i) {
		field_item const * device_field_table = NULL;
		char const * prefix = NULL;
		field_item const * item;

		e = g_soc_config->get_dev_param(&context,
							i,
							token_dev_type,
							&type);
		printf("\n"
		       "DevType[%d] = ", i);
		display_enum_value(&context, g_soc_config->devtype_table, type);
		printf(";\n");

		switch (type) {
			case nvboot_dev_type_spi:
				device_field_table = g_soc_config->spiflash_table;
				prefix = "SpiFlashParams";
				break;

			case nvboot_dev_type_sdmmc:
				device_field_table = g_soc_config->sdmmc_table;
				prefix = "SdmmcParams";
				break;

			case nvboot_dev_type_nand:
				device_field_table = g_soc_config->nand_table;
				prefix = "NandParams";
				break;

			default:
				device_field_table = NULL;
				prefix = "";
				break;
		}

		if (!device_field_table)
			continue;

		int width = max_width(device_field_table);

		for (item = device_field_table; item->name != NULL; ++item) {
			g_soc_config->get_dev_param(&context,
							i,
							item->token,
							&data);
			printf("DeviceParam[%d].%s.%-*s = ",
			       i, prefix, width, item->name);

			if (e != 0)
				printf("<ERROR reading parameter (%d)>", e);
			else
				display_field_value(&context, item, data);

			printf(";\n");
		}
	}

	/* Display SDRAM parameters */
	e = g_soc_config->get_value(token_num_sdram_sets,
				     &sdram_used,
				     context.bct);

	for (i = 0; (e == 0) && (i < sdram_used); ++i) {
		field_item const *item;

		printf("\n");

		int width = max_width(g_soc_config->sdram_field_table);

		for (item = g_soc_config->sdram_field_table; item->name != NULL; ++item) {
			e = g_soc_config->get_sdram_param(&context,
								i,
								item->token,
								&data);
			printf("SDRAM[%d].%-*s = ", i, width, item->name);

			if (e != 0)
				printf("<ERROR reading parameter (%d)>", e);
			else
				display_field_value(&context, item, data);

			printf(";\n");
		}
	}

	/* Clean up memory. */
	cleanup_context(&context);

	return e;
}
/*****************************************************************************/
