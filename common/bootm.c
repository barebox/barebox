/*
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
#include <boot.h>
#include <fs.h>
#include <malloc.h>
#include <memory.h>
#include <globalvar.h>
#include <init.h>

static LIST_HEAD(handler_list);

int register_image_handler(struct image_handler *handler)
{
	list_add_tail(&handler->list, &handler_list);

	return 0;
}

static struct image_handler *bootm_find_handler(enum filetype filetype,
		struct image_data *data)
{
	struct image_handler *handler;

	list_for_each_entry(handler, &handler_list, list) {
		if (filetype != filetype_uimage &&
				handler->filetype == filetype)
			return handler;
		if  (filetype == filetype_uimage &&
				handler->ih_os == data->os->header.ih_os)
			return handler;
	}

	return NULL;
}

static int bootm_open_os_uimage(struct image_data *data)
{
	int ret;

	data->os = uimage_open(data->os_file);
	if (!data->os)
		return -EINVAL;

	if (data->verify) {
		ret = uimage_verify(data->os);
		if (ret) {
			printf("Checking data crc failed with %s\n",
					strerror(-ret));
			uimage_close(data->os);
			return ret;
		}
	}

	uimage_print_contents(data->os);

	if (data->os->header.ih_arch != IH_ARCH) {
		printf("Unsupported Architecture 0x%x\n",
		       data->os->header.ih_arch);
		uimage_close(data->os);
		return -EINVAL;
	}

	if (data->os_address == UIMAGE_SOME_ADDRESS)
		data->os_address = data->os->header.ih_load;

	if (data->os_address != UIMAGE_INVALID_ADDRESS) {
		data->os_res = uimage_load_to_sdram(data->os, 0,
				data->os_address);
		if (!data->os_res) {
			uimage_close(data->os);
			return -ENOMEM;
		}
	}

	return 0;
}

static int bootm_open_initrd_uimage(struct image_data *data)
{
	int ret;

	if (!data->initrd_file)
		return 0;

	if (strcmp(data->os_file, data->initrd_file)) {
		data->initrd = uimage_open(data->initrd_file);
		if (!data->initrd)
			return -EINVAL;

		if (data->verify) {
			ret = uimage_verify(data->initrd);
			if (ret) {
				printf("Checking data crc failed with %s\n",
					strerror(-ret));
			}
		}
		uimage_print_contents(data->initrd);
	} else {
		data->initrd = data->os;
	}

	return 0;
}

static int bootm_open_oftree(struct image_data *data, const char *oftree, int num)
{
	enum filetype ft;
	struct fdt_header *fdt;
	size_t size;

	printf("Loading devicetree from '%s'\n", oftree);

	ft = file_name_detect_type(oftree);
	if ((int)ft < 0) {
		printf("failed to open %s: %s\n", oftree, strerror(-(int)ft));
		return ft;
	}

	if (ft == filetype_uimage) {
#ifdef CONFIG_CMD_BOOTM_OFTREE_UIMAGE
		struct uimage_handle *of_handle;
		int release = 0;

		if (!strcmp(data->os_file, oftree)) {
			of_handle = data->os;
		} else if (!strcmp(data->initrd_file, oftree)) {
			of_handle = data->initrd;
		} else {
			of_handle = uimage_open(oftree);
			if (!of_handle)
				return -ENODEV;
			uimage_print_contents(of_handle);
			release = 1;
		}

		fdt = uimage_load_to_buf(of_handle, num, &size);

		if (release)
			uimage_close(of_handle);
#else
		return -EINVAL;
#endif
	} else {
		fdt = read_file(oftree, &size);
		if (!fdt) {
			perror("open");
			return -ENODEV;
		}
	}

	ft = file_detect_type(fdt, size);
	if (ft != filetype_oftree) {
		printf("%s is not an oftree but %s\n", oftree,
				file_type_to_string(ft));
	}

	data->of_root_node = of_unflatten_dtb(NULL, fdt);
	if (!data->of_root_node) {
		pr_err("unable to unflatten devicetree\n");
		free(fdt);
		return -EINVAL;
	}

	free(fdt);

	return 0;
}

static void bootm_print_info(struct image_data *data)
{
	if (data->os_res)
		printf("OS image is at 0x%08x-0x%08x\n",
				data->os_res->start,
				data->os_res->end);
	else
		printf("OS image not yet relocated\n");

	if (data->initrd_file) {
		enum filetype initrd_type = file_name_detect_type(data->initrd_file);

		printf("Loading initrd %s '%s'",
				file_type_to_string(initrd_type),
				data->initrd_file);
		if (initrd_type == filetype_uimage &&
				data->initrd->header.ih_type == IH_TYPE_MULTI)
			printf(", multifile image %d", data->initrd_num);
		printf("\n");
		if (data->initrd_res)
			printf("initrd is at 0x%08x-0x%08x\n",
				data->initrd_res->start,
				data->initrd_res->end);
		else
			printf("initrd image not yet relocated\n");
	}
}

static char *bootm_image_name_and_no(const char *name, int *no)
{
	char *at, *ret;

	if (!name || !*name)
		return NULL;

	*no = 0;

	ret = xstrdup(name);
	at = strchr(ret, '@');
	if (!at)
		return ret;

	*at++ = 0;

	*no = simple_strtoul(at, NULL, 10);

	return ret;
}

/*
 * bootm_boot - Boot an application image described by bootm_data
 */
int bootm_boot(struct bootm_data *bootm_data)
{
	struct image_data *data;
	struct image_handler *handler;
	int ret;
	enum filetype os_type, initrd_type = filetype_unknown;

	if (!bootm_data->os_file) {
		printf("no image given\n");
		return -ENOENT;
	}

	data = xzalloc(sizeof(*data));

	data->os_file = bootm_image_name_and_no(bootm_data->os_file, &data->os_num);
	data->oftree_file = bootm_image_name_and_no(bootm_data->oftree_file, &data->oftree_num);
	data->initrd_file = bootm_image_name_and_no(bootm_data->initrd_file, &data->initrd_num);
	data->verbose = bootm_data->verbose;
	data->verify = bootm_data->verify;
	data->force = bootm_data->force;
	data->dryrun = bootm_data->dryrun;
	data->initrd_address = bootm_data->initrd_address;
	data->os_address = bootm_data->os_address;
	data->os_entry = bootm_data->os_entry;

	os_type = file_name_detect_type(data->os_file);
	if ((int)os_type < 0) {
		printf("could not open %s: %s\n", data->os_file,
				strerror(-os_type));
		ret = (int)os_type;
		goto err_out;
	}

	if (!data->force && os_type == filetype_unknown) {
		printf("Unknown OS filetype (try -f)\n");
		ret = -EINVAL;
		goto err_out;
	}

	if (os_type == filetype_uimage) {
		ret = bootm_open_os_uimage(data);
		if (ret) {
			printf("loading os image failed with %s\n",
					strerror(-ret));
			goto err_out;
		}
	}

	if (IS_ENABLED(CONFIG_CMD_BOOTM_INITRD) && data->initrd_file) {

		initrd_type = file_name_detect_type(data->initrd_file);
		if ((int)initrd_type < 0) {
			printf("could not open %s: %s\n", data->initrd_file,
					strerror(-initrd_type));
			ret = (int)initrd_type;
			goto err_out;
		}
		if (initrd_type == filetype_uimage) {
			ret = bootm_open_initrd_uimage(data);
			if (ret) {
				printf("loading initrd failed with %s\n",
						strerror(-ret));
				goto err_out;
			}
		}
	}

	printf("\nLoading OS %s '%s'", file_type_to_string(os_type),
			data->os_file);
	if (os_type == filetype_uimage &&
			data->os->header.ih_type == IH_TYPE_MULTI)
		printf(", multifile image %d", data->os_num);
	printf("\n");

	if (IS_ENABLED(CONFIG_OFTREE)) {
		if (data->oftree_file) {
			ret = bootm_open_oftree(data, data->oftree_file, data->oftree_num);
			if (ret)
				goto err_out;
		} else {
			data->of_root_node = of_get_root_node();
			if (data->of_root_node)
				printf("using internal devicetree\n");
		}
	}

	if (data->os_address == UIMAGE_SOME_ADDRESS)
		data->os_address = UIMAGE_INVALID_ADDRESS;

	handler = bootm_find_handler(os_type, data);
	if (!handler) {
		printf("no image handler found for image type %s\n",
			file_type_to_string(os_type));
		if (os_type == filetype_uimage)
			printf("and os type: %d\n", data->os->header.ih_os);
		ret = -ENODEV;
		goto err_out;
	}

	if (bootm_verbose(data)) {
		bootm_print_info(data);
		printf("Passing control to %s handler\n", handler->name);
	}

	if (data->dryrun)
		ret = 0;
	else
		ret = handler->bootm(data);
err_out:
	if (data->os_res)
		release_sdram_region(data->os_res);
	if (data->initrd_res)
		release_sdram_region(data->initrd_res);
	if (data->initrd && data->initrd != data->os)
		uimage_close(data->initrd);
	if (data->os)
		uimage_close(data->os);
	if (data->of_root_node && data->of_root_node != of_get_root_node())
		of_delete_node(data->of_root_node);

	free(data->os_file);
	free(data->oftree_file);
	free(data->initrd_file);
	free(data);

	return ret;
}

static int bootm_init(void)
{
	globalvar_add_simple("bootm.image", NULL);
	globalvar_add_simple("bootm.image.loadaddr", NULL);
	globalvar_add_simple("bootm.oftree", NULL);
	if (IS_ENABLED(CONFIG_CMD_BOOTM_INITRD)) {
		globalvar_add_simple("bootm.initrd", NULL);
		globalvar_add_simple("bootm.initrd.loadaddr", NULL);
	}

	return 0;
}
late_initcall(bootm_init);
