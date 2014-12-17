/*
 * (C) Copyright 2014 Sascha Hauer, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifdef __BAREBOX__
#include <common.h>
#include <image-metadata.h>
#include <libfile.h>
#include <getopt.h>
#include <malloc.h>
#include <fs.h>

#ifndef CONFIG_CMD_IMD
int imd_command_setenv(const char *variable_name, const char *value)
{
	return -ENOSYS;
}
#endif
#endif

/*
 * imd_next - return a pointer to the next metadata field.
 * @imd		The current metadata field
 */
struct imd_header *imd_next(struct imd_header *imd)
{
	int length;

	length = imd_read_length(imd);
	length = ALIGN(length, 4);
	length += 8;

	return (void *)imd + length;
}

struct imd_header *imd_find_type(struct imd_header *imd, uint32_t type)
{
	imd_for_each(imd, imd)
		if (imd_read_type(imd) == type)
			return imd;

	return NULL;
}

static int imd_next_validate(void *buf, int bufsize, int start_ofs)
{
	int length, size;
	struct imd_header *imd = buf + start_ofs;

	size = bufsize - start_ofs;

	if (size < 8) {
		debug("trunkated tag at offset %dd\n", start_ofs);
		return -EINVAL;
	}

	length = imd_read_length(imd);
	length = ALIGN(length, 4);
	length += 8;

	if (size < length) {
		debug("tag at offset %d with size %d exceeds bufsize %d\n",
				start_ofs, size, bufsize);
		return -EINVAL;
	}

	debug("tag at offset %d has length %d\n", start_ofs, length);

	return length;
}

static int imd_validate_tags(void *buf, int bufsize, int start_ofs)
{
	int ret;
	struct imd_header *imd = buf + start_ofs;

	while (1) {
		uint32_t type;

		ret = imd_next_validate(buf, bufsize, start_ofs);
		if (ret < 0) {
			debug("Invalid tag at offset %d\n", start_ofs);
			return -EINVAL;
		}

		imd = (void *)imd + ret;
		start_ofs += ret;

		type = imd_read_type(imd);

		if (!imd_type_valid(type)) {
			debug("Invalid: tag: 0x%08x\n", type);
			return -EINVAL;
		}

		if (type == IMD_TYPE_END)
			return 0;
	}
}

/*
 * imd_search_validate - find valid metadata in a buffer
 * @buf		the buffer
 * @size	buffer size
 * @start	The returned pointer to the metadata
 *
 * This iterates over a buffer and searches for metadata. The metadata
 * is checked for consistency (length fields not exceeding buffer and
 * presence of end header) and returned in @start. The returned pointer
 * is only valid when 0 is returned. The returned data may be unaligned.
 */
static int imd_search_validate(void *buf, int size, struct imd_header **start)
{
	int start_ofs = 0;
	int i, ret;

	for (i = start_ofs; i < size - 32; i++) {
		uint32_t type;

		type = imd_read_le32(buf + i);

		if (type != IMD_TYPE_START)
			continue;

		debug("Start tag found at offset %d\n", i);

		ret = imd_validate_tags(buf, size, i);
		if (!ret) {
			*start = buf + i;
			return 0;
		}
	}

	return -EINVAL;
}

struct imd_type_names {
	uint32_t type;
	const char *name;
};

static struct imd_type_names imd_types[] = {
	{
		.type = IMD_TYPE_RELEASE,
		.name = "release",
	}, {
		.type = IMD_TYPE_BUILD,
		.name = "build",
	}, {
		.type = IMD_TYPE_MODEL,
		.name = "model",
	}, {
		.type = IMD_TYPE_PARAMETER,
		.name = "parameter",
	}, {
		.type = IMD_TYPE_OF_COMPATIBLE,
		.name = "of_compatible",
	},
};

static const char *imd_type_to_name(uint32_t type)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(imd_types); i++)
		if (imd_types[i].type == type)
			return imd_types[i].name;

	return "unknown";
}

static uint32_t imd_name_to_type(const char *name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(imd_types); i++)
		if (!strcmp(imd_types[i].name, name))
			return imd_types[i].type;

	return IMD_TYPE_INVALID;
}

static char *imd_string_data(struct imd_entry_string *imd_string, int index)
{
	int i, total = 0, l = 0;
	int len = imd_read_length(&imd_string->header);
	char *p = imd_string->data;

	for (i = 0; total < len; total += l, p += l) {
		l = strlen(p) + 1;
		if (i++ == index)
			return p;
	}

	return NULL;
}

static char *imd_concat_strings(struct imd_entry_string *imd_string)
{
	int i, len = imd_read_length(&imd_string->header);
	char *str;

	str = malloc(len);
	if (!str)
		return NULL;

	memcpy(str, imd_string->data, len);

	for (i = 0; i < len - 1; i++)
		if (str[i] == 0)
			str[i] = ' ';

	return str;
}

int imd_command_verbose;

int imd_command(int argc, char *argv[])
{
	int ret, opt, strno = -1;
	void *buf;
	size_t size;
	uint32_t type = IMD_TYPE_INVALID;
	struct imd_header *imd_start, *imd;
	const char *filename;
	const char *variable_name = NULL;
	char *str;

	imd_command_verbose = 0;

	while ((opt = getopt(argc, argv, "vt:s:n:")) > 0) {
		switch(opt) {
		case 't':
			type = imd_name_to_type(optarg);
			if (type == IMD_TYPE_INVALID) {
				fprintf(stderr, "no such type: %s\n", optarg);
				return -ENOSYS;
			}
			break;
		case 's':
			variable_name = optarg;
			break;
		case 'v':
			imd_command_verbose = 1;
			break;
		case 'n':
			strno = simple_strtoul(optarg, NULL, 0);
			break;
		default:
			return -ENOSYS;
		}
	}

	if (optind == argc) {
		fprintf(stderr, "No image given\n");
		return -ENOSYS;
	}

	filename = argv[optind];

	ret = read_file_2(filename, &size, &buf, 0x100000);
	if (ret && ret != -EFBIG)
		return -errno;

	ret = imd_search_validate(buf, size, &imd_start);
	if (ret)
		return ret;

	if (type == IMD_TYPE_INVALID) {
		imd_for_each(imd_start, imd) {
			uint32_t type = imd_read_type(imd);

			if (imd_is_string(type)) {
				struct imd_entry_string *imd_string =
					(struct imd_entry_string *)imd;

				str = imd_concat_strings(imd_string);

				printf("%s: %s\n", imd_type_to_name(type), str);
			} else {
				debug("Unknown tag 0x%08x\n", type);
			}
		}
	} else {
		imd = imd_find_type(imd_start, type);
		if (!imd) {
			debug("No tag of type 0x%08x found\n", type);
			return -ENODATA;
		}

		if (imd_is_string(type)) {
			struct imd_entry_string *imd_string =
				(struct imd_entry_string *)imd;

			if (strno >= 0)
				str = imd_string_data(imd_string, strno);
			else
				str = imd_concat_strings(imd_string);

			if (!str)
				return -ENODATA;

			if (variable_name)
				imd_command_setenv(variable_name, str);
			else
				printf("%s\n", str);

			if (strno < 0)
				free(str);
		} else {
			printf("tag 0x%08x present\n", type);
		}
	}

	return 0;
}
