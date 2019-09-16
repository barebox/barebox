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
const struct imd_header *imd_next(const struct imd_header *imd)
{
	int length;

	length = imd_read_length(imd);
	length = ALIGN(length, 4);
	length += 8;

	return (const void *)imd + length;
}

const struct imd_header *imd_find_type(const struct imd_header *imd,
				       uint32_t type)
{
	imd_for_each(imd, imd)
		if (imd_read_type(imd) == type)
			return imd;

	return NULL;
}

static int imd_next_validate(const void *buf, int bufsize, int start_ofs)
{
	int length, size;
	const struct imd_header *imd = buf + start_ofs;

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

static int imd_validate_tags(const void *buf, int bufsize, int start_ofs)
{
	int ret;
	const struct imd_header *imd = buf + start_ofs;

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
 * imd_get - find valid metadata in a buffer
 * @buf		the buffer
 * @size	buffer size
 *
 * This iterates over a buffer and searches for metadata. The metadata
 * is checked for consistency (length fields not exceeding buffer and
 * presence of end header) and returned.
 *
 * Return: a pointer to the image metadata or a ERR_PTR
 */
const struct imd_header *imd_get(const void *buf, int size)
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
		if (!ret)
			return buf + i;
	}

	return ERR_PTR(-EINVAL);
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

/**
 * imd_type_to_name - convert a imd type to a name
 * @type: The imd type
 *
 * This function returns a string representation of the imd type
 */
const char *imd_type_to_name(uint32_t type)
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

/**
 * imd_string_data - get string data
 * @imd: The IMD entry
 * @index: The index of the string
 *
 * This function returns the string in @imd indexed by @index.
 *
 * Return: A pointer to the string or NULL if the string is not found
 */
const char *imd_string_data(const struct imd_header *imd, int index)
{
	int i, total = 0, l = 0;
	int len = imd_read_length(imd);
	char *p = (char *)(imd + 1);

	if (!imd_is_string(imd_read_type(imd)))
		return NULL;

	for (i = 0; total < len; total += l, p += l) {
		l = strlen(p) + 1;
		if (i++ == index)
			return p;
	}

	return NULL;
}

/**
 * imd_concat_strings - get string data
 * @imd: The IMD entry
 *
 * This function returns the concatenated strings in @imd. The string
 * returned is allocated with malloc() and the caller has to free() it.
 *
 * Return: A pointer to the string or NULL if the string is not found
 */
char *imd_concat_strings(const struct imd_header *imd)
{
	int i, len = imd_read_length(imd);
	char *str;
	char *data = (char *)(imd + 1);

	if (!imd_is_string(imd_read_type(imd)))
		return NULL;

	str = malloc(len);
	if (!str)
		return NULL;

	memcpy(str, data, len);

	for (i = 0; i < len - 1; i++)
		if (str[i] == 0)
			str[i] = ' ';

	return str;
}

/**
 * imd_get_param - get a parameter
 * @imd: The IMD entry
 * @name: The name of the parameter.
 *
 * Parameters have the IMD type IMD_TYPE_PARAMETER and the form
 * "key=value". This function iterates over the IMD entries and when
 * it finds a parameter with name "key" it returns the value found.
 *
 * Return: A pointer to the value or NULL if the string is not found
 */
const char *imd_get_param(const struct imd_header *imd, const char *name)
{
	const struct imd_header *cur;
	int namelen = strlen(name);

	imd_for_each(imd, cur) {
		char *data = (char *)(cur + 1);

		if (cur->type != IMD_TYPE_PARAMETER)
			continue;
		if (strncmp(name, data, namelen))
			continue;
		if (data[namelen] != '=')
			continue;
		return data + namelen + 1;
	}

	return NULL;
}

int imd_command_verbose;

int imd_command(int argc, char *argv[])
{
	int ret, opt, strno = -1;
	void *buf;
	size_t size;
	uint32_t type = IMD_TYPE_INVALID;
	const struct imd_header *imd_start, *imd;
	const char *filename;
	const char *variable_name = NULL;
	char *str;

	imd_command_verbose = 0;

	while ((opt = getopt(argc, argv, "vt:s:n:")) > 0) {
		switch(opt) {
		case 't':
			type = imd_name_to_type(optarg);
			if (type == IMD_TYPE_INVALID) {
				eprintf("no such type: %s\n", optarg);
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
		eprintf("No image given\n");
		return -ENOSYS;
	}

	filename = argv[optind];

	ret = read_file_2(filename, &size, &buf, 0x100000);
	if (ret && ret != -EFBIG)
		return -errno;

	imd_start = imd_get(buf, size);
	if (IS_ERR(imd_start)) {
		ret = PTR_ERR(imd_start);
		goto out;
	}

	if (type == IMD_TYPE_INVALID) {
		imd_for_each(imd_start, imd) {
			uint32_t type = imd_read_type(imd);

			if (imd_is_string(type)) {
				str = imd_concat_strings(imd);

				printf("%s: %s\n", imd_type_to_name(type), str);
			} else {
				debug("Unknown tag 0x%08x\n", type);
			}
		}
	} else {
		imd = imd_find_type(imd_start, type);
		if (!imd) {
			debug("No tag of type 0x%08x found\n", type);
			ret = -ENODATA;
			goto out;
		}

		if (imd_is_string(type)) {
			if (strno >= 0) {
				const char *s = imd_string_data(imd, strno);
				if (s)
					str = strdup(s);
				else
					str = NULL;
			} else {
				str = imd_concat_strings(imd);
			}

			if (!str) {
				ret = -ENODATA;
				goto out;
			}

			if (variable_name)
				imd_command_setenv(variable_name, str);
			else
				printf("%s\n", str);

			free(str);
		} else {
			printf("tag 0x%08x present\n", type);
		}
	}

	ret = 0;
out:
	free(buf);
	return ret;
}
