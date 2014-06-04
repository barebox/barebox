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
 * parse.c - Parsing support for the cbootimage tool
 */

#include <ctype.h>
#include "parse.h"
#include "cbootimage.h"
#include "data_layout.h"
#include "crypto.h"
#include "set.h"

/*
 * Function prototypes
 *
 * ParseXXX() parses XXX in the input
 * SetXXX() sets state based on the parsing results but does not perform
 *      any parsing of its own
 * A ParseXXX() function may call other parse functions and set functions.
 * A SetXXX() function may not call any parseing functions.
 */

static int
set_array(build_image_context *context,
			u_int32_t index,
			parse_token token,
			u_int32_t value);
static char *parse_u32(char *str, u_int32_t *val);
static char *parse_u8(char *str, u_int32_t *val);
static char *parse_filename(char *str, char *name, int chars_remaining);
static char *parse_enum(build_image_context *context,
			char *str,
			enum_item *table,
			u_int32_t *val);
static char
*parse_field_name(char *rest, field_item *field_table, field_item **field);
static char
*parse_field_value(build_image_context *context,
			char *rest,
			field_item *field,
			u_int32_t *value);
static int
parse_array(build_image_context *context, parse_token token, char *rest);
static int
parse_bootloader(build_image_context *context, parse_token token, char *rest);
static int
parse_value_u32(build_image_context *context, parse_token token, char *rest);
static int
parse_bct_file(build_image_context *context, parse_token token, char *rest);
static char
*parse_end_state(char *str, char *uname, int chars_remaining);
static int
parse_dev_param(build_image_context *context, parse_token token, char *rest);
static int
parse_sdram_param(build_image_context *context, parse_token token, char *rest);

static int process_statement(build_image_context *context,
				char *str,
				u_int8_t simple_parse);

static parse_item parse_simple_items[] =
{
	{ "Bctfile=",       token_bct_file,		parse_bct_file },
	{ "BootLoader=",    token_bootloader,		parse_bootloader },
	{ "Redundancy=",    token_redundancy,		parse_value_u32 },
	{ "Bctcopy=",       token_bct_copy,		parse_value_u32 },
	{ "Version=",       token_version,		parse_value_u32 },
	{ "PreBctPadBlocks=", token_pre_bct_pad_blocks,	parse_value_u32 },
	{ NULL, 0, NULL } /* Must be last */
};

static parse_item s_top_level_items[] = {
	{ "Bctfile=",       token_bct_file,		parse_bct_file },
	{ "Attribute=",     token_attribute,		parse_value_u32 },
	{ "Attribute[",     token_attribute,		parse_array },
	{ "PageSize=",      token_page_size,		parse_value_u32 },
	{ "BlockSize=",     token_block_size,		parse_value_u32 },
	{ "PartitionSize=", token_partition_size,	parse_value_u32 },
	{ "DevType[",       token_dev_type,		parse_array },
	{ "DeviceParam[",   token_dev_param,		parse_dev_param },
	{ "SDRAM[",         token_sdram,		parse_sdram_param },
	{ "BootLoader=",    token_bootloader,		parse_bootloader },
	{ "Redundancy=",    token_redundancy,		parse_value_u32 },
	{ "Bctcopy=",       token_bct_copy,		parse_value_u32 },
	{ "Version=",       token_version,		parse_value_u32 },
	{ "OdmData=",       token_odm_data,		parse_value_u32 },
	{ NULL, 0, NULL } /* Must be last */
};

/* Macro to simplify parser code a bit. */
#define PARSE_COMMA(x) if (*rest != ',') return (x); rest++

/*
 * Parse the given string and find the u32 dec/hex number.
 *
 * @param str	String to parse
 * @param val	Returns value that was parsed
 * @return the remainder of the string after the number was parsed
 */
static char *
parse_u32(char *str, u_int32_t *val)
{
	u_int32_t value = 0;
	u_int32_t digit;

	while (*str == '0')
		str++;

	if (tolower(*str) == 'x') {
		str++;
		while (isxdigit(*str)) {
			value *= 16;
			digit = tolower(*str);
			value += digit <= '9' ? digit - '0' : digit - 'a' + 10;
			str++;
		}
	} else {
		while (*str >= '0' && *str <= '9') {
			value = value*10 + (*str - '0');
			str++;
		}
	}
	*val = value;
	return str;
}

/*
 * Parse the given string and find the u8 dec/hex number.
 *
 * @param str	String to parse
 * @param val	Returns value that was parsed
 * @return the remainder of the string after the number was parsed
 */
static char *
parse_u8(char *str, u_int32_t *val)
{
	char *retval;

	retval = parse_u32(str, val);

	if (*val > 0xff) {
		printf("Warning: Parsed 8-bit value that exceeded 8-bits.\n");
		printf("         Parsed value = %d. Remaining text = %s\n",
			 *val, retval);
	}

	return retval;
}


/*
 * Parse the given string and find the file name then
 * return the rest of the string.
 *
 * @param str            	String to parse
 * @param name           	Returns the filename that was parsed
 * @param chars_remaining	The maximum length of filename
 * @return the remainder of the string after the name was parsed
 */
static char *
parse_filename(char *str, char *name, int chars_remaining)
{
	/*
	 * Check if the filename buffer is out of space, preserving one
	 * character to null terminate the string.
	 */
	while (isalnum(*str) || strchr("\\/~_-+:.", *str)) {

		chars_remaining--;

		if (chars_remaining < 1)
			return NULL;
		*name++ = *str++;
	}

	/* Null terminate the filename. */
	*name = '\0';

	return str;
}

/*
 * Parse the given string and find the match field name listed
 * in field table.
 *
 * @param rest       	String to parse
 * @param field_table	The field table to parse
 * @param field      	Returns the field item that was parsed
 * @return NULL or the remainder of the string after the field item was parsed
 */
static char
*parse_field_name(char *rest, field_item *field_table, field_item **field)
{
	u_int32_t i;
	u_int32_t field_name_len = 0;

	assert(field_table != NULL);
	assert(rest != NULL);
	assert(field != NULL);

	while (rest[field_name_len] != '=')
		field_name_len++;

	/* Parse the field name. */
	for (i = 0; field_table[i].name != NULL; i++) {
		if ((strlen(field_table[i].name) == field_name_len) &&
			!strncmp(field_table[i].name,
			rest,
			field_name_len)) {

			*field = &(field_table[i]);
			rest = rest + field_name_len;
			return rest;
		}
	}

	/* Field wasn't found or a parse error occurred. */
	return NULL;
}

/*
 * Parse the value based on the field table
 *
 * @param context	The main context pointer
 * @param rest   	String to parse
 * @param field  	Field item to parse
 * @param value	Returns the value that was parsed
 * @return the remainder of the string after the value was parsed
 */
static char
*parse_field_value(build_image_context *context,
			char *rest,
			field_item *field,
			u_int32_t *value)
{
	assert(rest != NULL);
	assert(field != NULL);
	assert((field->type != field_type_enum)
		|| (field->enum_table != NULL));

	switch (field->type) {
	case field_type_enum:
		rest = parse_enum(context, rest, field->enum_table, value);
		break;

	case field_type_u32:
		rest = parse_u32(rest, value);
		break;

	case field_type_u8:
		rest = parse_u8(rest, value);
		break;

	default:
		printf("Unexpected field type %d at line %d\n",
			field->type, __LINE__);
		rest = NULL;
		break;
	}

	return rest;
}

/*
 * Parse the given string and find the match enum item listed
 * in table.
 *
 * @param context	The main context pointer
 * @param str    	String to parse
 * @param table  	Enum item table to parse
 * @param value  	Returns the value that was parsed
 * @return the remainder of the string after the item was parsed
 */
static char *
parse_enum(build_image_context *context,
		char *str,
		enum_item *table,
		u_int32_t *val)
{
	int i;
	char *rest;

	for (i = 0; table[i].name != NULL; i++) {
		if (!strncmp(table[i].name, str,
			strlen(table[i].name))) {
			*val = table[i].value;
			rest = str + strlen(table[i].name);
			return rest;
		}
	}
	return parse_u32(str, val);

}

/*
 * Parse the given string and find the bootloader file name, load address and
 * entry point information then call set_bootloader function.
 *
 * @param context	The main context pointer
 * @param token  	The parse token value
 * @param rest   	String to parse
 * @return 0 and 1 for success and failure
 */
static int parse_bootloader(build_image_context *context,
			parse_token token,
			char *rest)
{
	char filename[MAX_BUFFER];
	char e_state[MAX_STR_LEN];
	u_int32_t load_addr;
	u_int32_t entry_point;

	assert(context != NULL);
	assert(rest != NULL);

	if (context->generate_bct != 0)
		return 0;
	/* Parse the file name. */
	rest = parse_filename(rest, filename, MAX_BUFFER);
	if (rest == NULL)
		return 1;

	PARSE_COMMA(1);

	/* Parse the load address. */
	rest = parse_u32(rest, &load_addr);
	if (rest == NULL)
		return 1;

	PARSE_COMMA(1);

	/* Parse the entry point. */
	rest = parse_u32(rest, &entry_point);
	if (rest == NULL)
		return 1;

	PARSE_COMMA(1);

	/* Parse the end state. */
	rest = parse_end_state(rest, e_state, MAX_STR_LEN);
	if (rest == NULL)
		return 1;
	if (strncmp(e_state, "Complete", strlen("Complete")))
		return 1;

	/* Parsing has finished - set the bootloader */
	return set_bootloader(context, filename, load_addr, entry_point);
}

/*
 * Parse the given string and find the array items in config file.
 *
 * @param context	The main context pointer
 * @param token  	The parse token value
 * @param rest   	String to parse
 * @return 0 and 1 for success and failure
 */
static int
parse_array(build_image_context *context, parse_token token, char *rest)
{
	u_int32_t index;
	u_int32_t value;

	assert(context != NULL);
	assert(rest != NULL);

	/* Parse the index. */
	rest = parse_u32(rest, &index);
	if (rest == NULL)
		return 1;

	/* Parse the closing bracket. */
	if (*rest != ']')
		return 1;
	rest++;

	/* Parse the equals sign.*/
	if (*rest != '=')
		return 1;
	rest++;

	/* Parse the value based on the field table. */
	switch (token) {
	case token_attribute:
		rest = parse_u32(rest, &value);
		break;
	case token_dev_type:
		rest = parse_enum(context,
				rest,
				g_soc_config->devtype_table,
				&value);
		break;

	default:
	/* Unknown token */
		return 1;
	}

	if (rest == NULL)
		return 1;

	/* Store the result. */
	return set_array(context, index, token, value);
}

/*
 * Call hw interface to set the value for array item in bct such as device
 * type and bootloader attribute.
 *
 * @param context	The main context pointer
 * @param index  	The index for array
 * @param token  	The parse token value
 * @param value  	The value to set
 * @return 0 and -ENODATA for success and failure
 */

static int
set_array(build_image_context *context,
			u_int32_t index,
			parse_token token,
			u_int32_t value)
{
	int err = 0;

	assert(context != NULL);

	switch (token) {
	case token_attribute:
		err = g_soc_config->setbl_param(index,
					token_bl_attribute,
					&value,
					context->bct);
		break;
	case token_dev_type:
		err = g_soc_config->set_dev_param(context,
						index,
						token_dev_type,
						value);
		break;
	default:
		break;
	}
	return err;
}

/*
 * General handler for setting u_int32_t values in config files.
 *
 * @param context	The main context pointer
 * @param token  	The parse token value
 * @param rest   	String to parse
 * @return 0 and 1 for success and failure
 */
static int parse_value_u32(build_image_context *context,
			parse_token token,
			char *rest)
{
	u_int32_t value;

	assert(context != NULL);
	assert(rest != NULL);

	rest = parse_u32(rest, &value);
	if (rest == NULL)
		return 1;

	return context_set_value(context, token, value);
}

/*
 * Parse the given string and find the bct file name.
 *
 * @param context	The main context pointer
 * @param token  	The parse token value
 * @param rest   	String to parse
 * @return 0 and 1 for success and failure
 */
static int
parse_bct_file(build_image_context *context, parse_token token, char *rest)
{
	char   filename[MAX_BUFFER];

	assert(context != NULL);
	assert(rest != NULL);

	/* Parse the file name. */
	rest = parse_filename(rest, filename, MAX_BUFFER);
	if (rest == NULL)
		return 1;

	/* Parsing has finished - set the bctfile */
	context->bct_filename = filename;
	/* Read the bct file to buffer */
	if (read_bct_file(context))
		return 1;

	update_context(context);
	return 0;
}

static char *
parse_end_state(char *str, char *uname, int chars_remaining)
{
	while (isalpha(*str)) {

		*uname++ = *str++;
		if (--chars_remaining < 0)
			return NULL;
	}
	*uname = '\0';
	return str;
}

/*
 * Parse the given string and find device parameter listed in device table
 * and value for this device parameter. If match, call the corresponding
 * function in the table to set device parameter.
 *
 * @param context	The main context pointer
 * @param token  	The parse token value
 * @param rest   	String to parse
 * @return 0 and 1 for success and failure
 */
static int
parse_dev_param(build_image_context *context, parse_token token, char *rest)
{
	u_int32_t i;
	u_int32_t value;
	field_item *field;
	u_int32_t index;
	parse_subfield_item *device_item = NULL;

	assert(context != NULL);
	assert(rest != NULL);

	/* Parse the index. */
	rest = parse_u32(rest, &index);
	if (rest == NULL)
		return 1;

	/* Parse the closing bracket. */
	if (*rest != ']')
		return 1;
	rest++;

	/* Parse the following '.' */
	if (*rest != '.')
		return 1;
	rest++;

	/* Parse the device name. */
	for (i = 0; g_soc_config->device_type_table[i].prefix != NULL; i++) {
		if (!strncmp(g_soc_config->device_type_table[i].prefix,
			rest, strlen(g_soc_config->device_type_table[i].prefix))) {

			device_item = &(g_soc_config->device_type_table[i]);
			rest = rest + strlen(g_soc_config->device_type_table[i].prefix);

			/* Parse the field name. */
			rest = parse_field_name(rest,
					g_soc_config->device_type_table[i].field_table,
				&field);
			if (rest == NULL)
				return 1;

			/* Parse the equals sign.*/
			if (*rest != '=')
				return 1;
			rest++;

			/* Parse the value based on the field table. */
			rest = parse_field_value(context, rest, field, &value);
			if (rest == NULL)
				return 1;
			return device_item->process(context,
						index, field->token, value);
		}
	}
	return 1;
}

/*
 * Parse the given string and find sdram parameter and value in config
 * file. If match, call the corresponding function set the sdram parameter.
 *
 * @param context	The main context pointer
 * @param token  	The parse token value
 * @param rest   	String to parse
 * @return 0 and 1 for success and failure
 */
static int
parse_sdram_param(build_image_context *context, parse_token token, char *rest)
{
	u_int32_t value;
	field_item *field;
	u_int32_t index;

	assert(context != NULL);
	assert(rest != NULL);

	/* Parse the index. */
	rest = parse_u32(rest, &index);
	if (rest == NULL)
		return 1;

	/* Parse the closing bracket. */
	if (*rest != ']')
		return 1;
	rest++;

	/* Parse the following '.' */
	if (*rest != '.')
		return 1;
	rest++;

	/* Parse the field name. */
	rest = parse_field_name(rest, g_soc_config->sdram_field_table, &field);

	if (rest == NULL)
		return 1;

	/* Parse the equals sign.*/
	if (*rest != '=')
		return 1;
	rest++;

	/* Parse the value based on the field table. */
	rest = parse_field_value(context, rest, field, &value);
	if (rest == NULL)
		return 1;

	/* Store the result. */
	return g_soc_config->set_sdram_param(context,
						index,
						field->token,
						value);
}

/*
 * Compare the given string with item listed in table.
 * Execute the proper process function if match.
 *
 * @param context	The main context pointer
 * @param str    	String to parse
 * @param simple_parse	Simple parse flag
 * @return 0 and 1 for success and failure
 */
static int
process_statement(build_image_context *context,
			char *str,
			u_int8_t simple_parse)
{
	int i;
	char *rest;
	parse_item *cfg_parse_item;

	if (simple_parse == 0)
		cfg_parse_item = s_top_level_items;
	else
		cfg_parse_item = parse_simple_items;

	for (i = 0; cfg_parse_item[i].prefix != NULL; i++) {
		if (!strncmp(cfg_parse_item[i].prefix, str,
			strlen(cfg_parse_item[i].prefix))) {
			rest = str + strlen(cfg_parse_item[i].prefix);

			return cfg_parse_item[i].process(context,
						cfg_parse_item[i].token,
						rest);
		}
	}

	/* If this point was reached, there was a processing error. */
	return 1;
}

/*
 * The main function parse the config file.
 *
 * @param context     	The main context pointer
 * @param simple_parse	Simple parse flag
 */
void process_config_file(build_image_context *context, u_int8_t simple_parse)
{
	char buffer[MAX_BUFFER];
	int  space = 0;
	int current;
	u_int8_t c_eol_comment_start = 0; /* True after first slash */
	u_int8_t comment = 0;
	u_int8_t string = 0;
	u_int8_t equal_encounter = 0;

	assert(context != NULL);
	assert(context->config_file != NULL);

	while ((current = fgetc(context->config_file)) != EOF) {
		if (space >= (MAX_BUFFER-1)) {
			/* if we exceeded the max buffer size, it is likely
			 due to a missing semi-colon at the end of a line */
			printf("Config file parsing error!");
			exit(1);
		}

		/* Handle failure to complete "//" comment token.
		 Insert the '/' into the busffer and proceed with
		 processing the current character. */
		if (c_eol_comment_start && current != '/') {
			c_eol_comment_start = 0;
			buffer[space++] = '/';
		}

		switch (current) {
		case '\"': /* " indicates start or end of a string */
			if (!comment) {
				string ^= 1;
				buffer[space++] = current;
			}
			break;
		case ';':
			if (!string && !comment) {
				buffer[space++] = '\0';

				if (process_statement(context,
							buffer,
							simple_parse))
					goto error;
				space = 0;
				equal_encounter = 0;
			} else if (string)
				buffer[space++] = current;
			break;

		case '/':
			if (!string && !comment) {
				if (c_eol_comment_start) {
					/* EOL comment started. */
					comment = 1;
					c_eol_comment_start = 0;
				} else {
					/* Potential start of eol comment. */
					c_eol_comment_start = 1;
				}
			} else if (!comment)
				buffer[space++] = current;
			break;

		/* ignore whitespace.  uses fallthrough */
		case '\n':
		case '\r': /* carriage returns end comments */
			string  = 0;
			comment = 0;
			c_eol_comment_start = 0;
		case ' ':
		case '\t':
			if (string)
				buffer[space++] = current;
			break;

		case '#':
			if (!string)
				comment = 1;
			else
				buffer[space++] = current;
			break;

		default:
			if (!comment) {
				buffer[space++] = current;
				if (current == '=') {
					if (!equal_encounter)
						equal_encounter = 1;
					else
						goto error;
				}
			}
			break;
		}
	}

	return;

 error:
	printf("Error parsing: %s\n", buffer);
	exit(1);
}
