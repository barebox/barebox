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
#include <filetype.h>
#include <libfile.h>
#include <fs.h>
#include <fcntl.h>
#include <malloc.h>
#include <linux/stat.h>

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

	if (key == 'y') {
		printf("updating barebox...\n");
		return 0;
	}

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

struct bbu_std {
	struct bbu_handler handler;
	enum filetype filetype;
};

static int bbu_std_file_handler(struct bbu_handler *handler,
					struct bbu_data *data)
{
	struct bbu_std *std = container_of(handler, struct bbu_std, handler);
	int fd, ret;
	enum filetype filetype;
	struct stat s;
	unsigned oflags = O_WRONLY;

	filetype = file_detect_type(data->image, data->len);
	if (filetype != std->filetype) {
		if (!bbu_force(data, "incorrect image type. Expected: %s, got %s",
				file_type_to_string(std->filetype),
				file_type_to_string(filetype)))
			return -EINVAL;
	}

	ret = stat(data->devicefile, &s);
	if (ret) {
		oflags |= O_CREAT;
	} else {
		if (!S_ISREG(s.st_mode) && s.st_size < data->len) {
			printf("Image (%lld) is too big for device (%d)\n",
					s.st_size, data->len);
		}
	}

	ret = bbu_confirm(data);
	if (ret)
		return ret;

	fd = open(data->devicefile, oflags);
	if (fd < 0)
		return fd;

	ret = protect(fd, data->len, 0, 0);
	if (ret && ret != -ENOSYS) {
		printf("unprotecting %s failed with %s\n", data->devicefile,
				strerror(-ret));
		goto err_close;
	}

	ret = erase(fd, data->len, 0);
	if (ret && ret != -ENOSYS) {
		printf("erasing %s failed with %s\n", data->devicefile,
				strerror(-ret));
		goto err_close;
	}

	ret = write_full(fd, data->image, data->len);
	if (ret < 0)
		goto err_close;

	protect(fd, data->len, 0, 1);

	ret = 0;

err_close:
	close(fd);

	return ret;
}

/**
 * bbu_register_std_file_update() - register a barebox update handler for a
 *                                  standard file-to-device-copy operation
 * @name:	Name of the handler
 * @flags:	BBU_HANDLER_FLAG_* flags
 * @devicefile: the file to write the update image to
 * @imagetype:	The filetype that the update image must have
 *
 * This update handler us suitable for a standard file-to-device copy operation.
 * The handler checks for a filetype and unprotects/erases the device if
 * necessary. If devicefile belongs to a device then the device is checkd for
 * enough space before touching it.
 *
 * Return: 0 if successful, negative error code otherwise
 */
int bbu_register_std_file_update(const char *name, unsigned long flags,
		char *devicefile, enum filetype imagetype)
{
	struct bbu_std *std;
	struct bbu_handler *handler;
	int ret;

	std = xzalloc(sizeof(*std));
	std->filetype = imagetype;

	handler = &std->handler;

	handler->flags = flags;
	handler->devicefile = devicefile;
	handler->name = name;
	handler->handler = bbu_std_file_handler;

	ret = bbu_register_handler(handler);
	if (ret)
		free(std);

	return ret;
}
