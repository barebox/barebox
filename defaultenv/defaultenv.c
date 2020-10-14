/*
 * Copyright (C) 2014 Sascha Hauer, Pengutronix
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
#include <common.h>
#include <envfs.h>
#include <filetype.h>
#include <uncompress.h>
#include <malloc.h>
#include <init.h>
#include <asm/unaligned.h>
#include "barebox_default_env.h"

static LIST_HEAD(defaultenv_list);

struct defaultenv {
	struct list_head list;
	const char *name;
	void *buf;
	size_t size;
};

static void defaultenv_add_base(void)
{
	static int base_added;

	if (base_added)
		return;

	base_added = 1;

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC_NEW))
		defaultenv_append_directory(defaultenv_2_base);
	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC_NEW_MENU))
		defaultenv_append_directory(defaultenv_2_menu);
	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC_NEW_DFU))
		defaultenv_append_directory(defaultenv_2_dfu);
	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC_NEW_REBOOT_MODE))
		defaultenv_append_directory(defaultenv_2_reboot_mode);
	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC))
		defaultenv_append_directory(defaultenv_1);
}

static void defaultenv_add_external(void)
{
	static int external_added;

	if (external_added)
		return;

	external_added = 1;

	/*
	 * The traditional or external environment given with
	 * CONFIG_DEFAULT_ENVIRONMENT_PATH.
	 */
	defaultenv_append((void *)default_environment,
			default_environment_size, "defaultenv");
}

/*
 * defaultenv_append - append a envfs buffer to the default environment
 * @buf:	The buffer containing the binary environment. If it is
 *              not a plain buffer it is assumed to be a compressed buffer.
 * @size:	The length of the buffer
 *
 * This adds an overlay to the default environment. New files will be created,
 * existing files will be overwritten with the overlay.
 */
void defaultenv_append(void *buf, unsigned int size, const char *name)
{
	struct defaultenv *df;

	defaultenv_add_base();

	df = xzalloc(sizeof(*df));
	df->buf = buf;
	df->size = size;
	df->name = name;

	list_add_tail(&df->list, &defaultenv_list);
}

static int defaultenv_load_one(struct defaultenv *df, const char *dir,
		unsigned flags)
{
	void *freep = NULL;
	void *buf;
	enum filetype ft = file_detect_type(df->buf, df->size);
	uint32_t size;
	int ret;

	pr_debug("loading %s\n", df->name);

	if (!IS_ENABLED(CONFIG_DEFAULT_COMPRESSION_NONE) &&
			ft != filetype_barebox_env) {
		size = get_unaligned_le32(df->buf + df->size - 4);
		freep = malloc(size);
		if (!freep)
			return -ENOMEM;

		ret = uncompress(df->buf, df->size,
				NULL, NULL,
				freep, NULL, uncompress_err_stdout);
		if (ret) {
			free(freep);
			pr_err("Failed to uncompress: %s\n", strerror(-ret));
			return ret;
		}

		buf = freep;
	} else {
		buf = df->buf;
		size = df->size;
	}

	ret = envfs_load_from_buf(buf, size, dir, flags);

	free(freep);

	if (ret)
		pr_err("Failed to load defaultenv: %s\n", strerror(-ret));

	return ret;
}

/*
 * defaultenv_load - load the default environment
 * @dir:	The directory the default environment should be loaded to.
 *
 * This loads all environment snippets previously registered with
 * defaultenv_append to the directory given with @dir.
 *
 * Return: 0 for success, negative error code otherwise.
 */
int defaultenv_load(const char *dir, unsigned flags)
{
	struct defaultenv *df;
	int ret;

	defaultenv_add_base();

	defaultenv_add_external();

	list_for_each_entry(df, &defaultenv_list, list) {
		ret = defaultenv_load_one(df, dir, flags);
		if (ret)
			return ret;
	}

	return 0;
}
