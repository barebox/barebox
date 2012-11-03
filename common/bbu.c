/*
 * bbu.c - barebox update functions
 *
 * Copyright (c) 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <common.h>
#include <bbu.h>
#include <linux/list.h>
#include <errno.h>
#include <readkey.h>

static LIST_HEAD(bbu_image_handlers);

int bbu_force(struct bbu_data *data, const char *fmt, ...)
{
	va_list args;

	printf("UPDATE: ");

	va_start(args, fmt);

	vprintf(fmt, args);

	va_end(args);

	if (!(data->flags & BBU_FLAG_FORCE))
		goto out;

	if (!data->force)
		goto out;

	data->force--;

	printf(" (forced)\n");

	return 1;
out:
	printf("\n");

	return 0;
}

int bbu_confirm(struct bbu_data *data)
{
	int key;

	if (data->flags & BBU_FLAG_YES)
		return 0;

	printf("update barebox from %s using handler %s to %s (y/n)?\n",
			data->imagefile, data->handler_name,
			data->devicefile);

	key = read_key();

	if (key == 'y')
		return 0;

	return -EINTR;
}

static struct bbu_handler *bbu_find_handler(const char *name)
{
	struct bbu_handler *handler;

	list_for_each_entry(handler, &bbu_image_handlers, list) {
		if (!name) {
			if (handler->flags & BBU_HANDLER_FLAG_DEFAULT)
				return handler;
			continue;
		}

		if (!strcmp(handler->name, name))
			return handler;
	}

	return NULL;
}

/*
 * do a barebox update with data from *data
 */
int barebox_update(struct bbu_data *data)
{
	struct bbu_handler *handler;
	int ret;

	handler = bbu_find_handler(data->handler_name);
	if (!handler)
		return -ENODEV;

	if (!data->handler_name)
		data->handler_name = handler->name;

	if (!data->devicefile)
		data->devicefile = handler->devicefile;

	ret = handler->handler(handler, data);
	if (ret == -EINTR)
		printf("update aborted\n");

	if (!ret)
		printf("update succeeded\n");

	return ret;
}

/*
 * print a list of all registered update handlers
 */
void bbu_handlers_list(void)
{
	struct bbu_handler *handler;

	if (list_empty(&bbu_image_handlers))
		printf("(none)\n");

	list_for_each_entry(handler, &bbu_image_handlers, list)
		printf("%s%-11s -> %-10s\n",
				handler->flags & BBU_HANDLER_FLAG_DEFAULT ?
				"* " : "  ",
				handler->name,
				handler->devicefile);
}

/*
 * register a new update handler
 */
int bbu_register_handler(struct bbu_handler *handler)
{
	if (bbu_find_handler(handler->name))
		return -EBUSY;

	if (handler->flags & BBU_HANDLER_FLAG_DEFAULT &&
			bbu_find_handler(NULL))
		return -EBUSY;

	list_add_tail(&handler->list, &bbu_image_handlers);

	return 0;
}
