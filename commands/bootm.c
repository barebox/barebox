/*
 * (C) Copyright 2000-2006
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * Boot support
 */
#include <common.h>
#include <driver.h>
#include <command.h>
#include <image.h>
#include <malloc.h>
#include <environment.h>
#include <asm/byteorder.h>
#include <xfuncs.h>
#include <getopt.h>
#include <fcntl.h>
#include <fs.h>
#include <errno.h>
#include <boot.h>
#include <of.h>
#include <libfdt.h>
#include <rtc.h>
#include <init.h>
#include <of.h>
#include <magicvar.h>
#include <uncompress.h>
#include <memory.h>
#include <filetype.h>
#include <asm-generic/memory_layout.h>

static LIST_HEAD(handler_list);

#ifdef CONFIG_CMD_BOOTM_INITRD
static inline int bootm_initrd(struct image_data *data)
{
	return data->initrd ? 1 : 0;
}
#else
static inline int bootm_initrd(struct image_data *data)
{
	return 0;
}
#endif

int register_image_handler(struct image_handler *handler)
{
	list_add_tail(&handler->list, &handler_list);
	return 0;
}

#define UIMAGE_SOME_ADDRESS (UIMAGE_INVALID_ADDRESS - 1)

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
			return ret;
		}
	}

	uimage_print_contents(data->os);

	if (data->os->header.ih_arch != IH_ARCH) {
		printf("Unsupported Architecture 0x%x\n",
		       data->os->header.ih_arch);
		return -EINVAL;
	}

	if (data->os_address == UIMAGE_SOME_ADDRESS)
		data->os_address = data->os->header.ih_load;

	if (data->os_address != UIMAGE_INVALID_ADDRESS) {
		data->os_res = uimage_load_to_sdram(data->os, 0,
				data->os_address);
		if (!data->os_res)
			return -ENOMEM;
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

	if (data->initrd_address == UIMAGE_SOME_ADDRESS)
		data->initrd_address = data->initrd->header.ih_load;

	if (data->initrd_address != UIMAGE_INVALID_ADDRESS) {
		data->initrd_res = uimage_load_to_sdram(data->initrd,
				data->initrd_num,
				data->initrd_address);
		if (!data->initrd_res)
			return -ENOMEM;
	}

	return 0;
}

#ifdef CONFIG_OFTREE
static int bootm_open_oftree(struct image_data *data, char *oftree, int num)
{
	enum filetype ft;
	struct fdt_header *fdt, *fixfdt;
	int ret;
	size_t size;

	if (bootm_verbose(data))
		printf("Loading oftree from '%s'\n", oftree);

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

	ft = file_detect_type(fdt);
	if (ft != filetype_oftree) {
		printf("%s is not a oftree but %s\n", oftree,
				file_type_to_string(ft));
	}

	fixfdt = xmemalign(4096, size + 0x8000);
	memcpy(fixfdt, fdt, size);

	free(fdt);

	ret = fdt_open_into(fixfdt, fixfdt, size + 0x8000);
	if (ret) {
		printf("unable to parse %s\n", oftree);
		return -ENODEV;
	}

	ret = of_fix_tree(fixfdt);
	if (ret)
		return ret;

	if (bootm_verbose(data) > 1)
		fdt_print(fixfdt, "/");

	data->oftree = fixfdt;

	return ret;
}
#endif

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

static void bootm_image_name_and_no(char *name, int *no)
{
	char *at;

	*no = 0;

	at = strchr(name, '@');
	if (!at)
		return;

	*at++ = 0;

	*no = simple_strtoul(at, NULL, 10);
}

static int do_bootm(int argc, char *argv[])
{
	int opt;
	struct image_handler *handler;
	struct image_data data;
	int ret = 1;
	enum filetype os_type, initrd_type = filetype_unknown;
	char *oftree = NULL;
	int fallback = 0;

	memset(&data, 0, sizeof(struct image_data));

	data.initrd_address = UIMAGE_SOME_ADDRESS;
	data.os_address = UIMAGE_SOME_ADDRESS;
	data.verify = 0;
	data.verbose = 0;

	while ((opt = getopt(argc, argv, "cL:r:a:e:vo:f")) > 0) {
		switch(opt) {
		case 'c':
			data.verify = 1;
			break;
#ifdef CONFIG_CMD_BOOTM_INITRD
		case 'L':
			data.initrd_address = simple_strtoul(optarg, NULL, 0);
			break;
		case 'r':
			data.initrd_file = optarg;
			break;
#endif
		case 'a':
			data.os_address = simple_strtoul(optarg, NULL, 0);
			break;
		case 'e':
			data.os_entry = simple_strtoul(optarg, NULL, 0);
			break;
		case 'v':
			data.verbose++;
			break;
		case 'o':
			oftree = optarg;
			break;
		case 'f':
			fallback = 1;
			break;
		default:
			break;
		}
	}

	if (optind == argc)
		return COMMAND_ERROR_USAGE;

	data.os_file = argv[optind];

	bootm_image_name_and_no(data.os_file, &data.os_num);

	os_type = file_name_detect_type(data.os_file);
	if ((int)os_type < 0) {
		printf("could not open %s: %s\n", data.os_file,
				strerror(-os_type));
		goto err_out;
	}

	if (!fallback && os_type == filetype_unknown) {
		printf("Unknown OS filetype (try -f)\n");
		goto err_out;
	}

	if (os_type == filetype_uimage) {
		ret = bootm_open_os_uimage(&data);
		if (ret) {
			printf("loading os image failed with %s\n",
					strerror(-ret));
			goto err_out;
		}
	}

	if (bootm_initrd(&data)) {
		bootm_image_name_and_no(data.initrd_file, &data.initrd_num);

		initrd_type = file_name_detect_type(data.initrd_file);
		if ((int)initrd_type < 0) {
			printf("could not open %s: %s\n", data.initrd_file,
					strerror(-initrd_type));
			goto err_out;
		}
		if (initrd_type == filetype_uimage) {
			ret = bootm_open_initrd_uimage(&data);
			if (ret) {
				printf("loading initrd failed with %s\n",
						strerror(-ret));
				goto err_out;
			}
		}
	}

	if (bootm_verbose(&data)) {
		printf("\nLoading OS %s '%s'", file_type_to_string(os_type),
				data.os_file);
		if (os_type == filetype_uimage &&
				data.os->header.ih_type == IH_TYPE_MULTI)
			printf(", multifile image %d", data.os_num);
		printf("\n");
		if (data.os_res)
			printf("OS image is at 0x%08x-0x%08x\n",
					data.os_res->start,
					data.os_res->start +
					data.os_res->size - 1);
		else
			printf("OS image not yet relocated\n");

		if (data.initrd_file) {
			printf("Loading initrd %s '%s'",
					file_type_to_string(initrd_type),
					data.initrd_file);
			if (initrd_type == filetype_uimage &&
					data.initrd->header.ih_type == IH_TYPE_MULTI)
				printf(", multifile image %d", data.initrd_num);
			printf("\n");
			if (data.initrd_res)
				printf("initrd is at 0x%08x-0x%08x\n",
					data.initrd_res->start,
					data.initrd_res->start +
					data.initrd_res->size - 1);
			else
				printf("initrd image not yet relocated\n");
		}
	}

#ifdef CONFIG_OFTREE
	if (oftree) {
		int oftree_num;

		bootm_image_name_and_no(oftree, &oftree_num);

		ret = bootm_open_oftree(&data, oftree, oftree_num);
		if (ret)
			goto err_out;
	}
#endif
	if (data.os_address == UIMAGE_SOME_ADDRESS)
		data.os_address = UIMAGE_INVALID_ADDRESS;
	if (data.initrd_address == UIMAGE_SOME_ADDRESS)
		data.initrd_address = UIMAGE_INVALID_ADDRESS;

	handler = bootm_find_handler(os_type, &data);
	if (!handler) {
		printf("no image handler found for image type %s\n",
			file_type_to_string(os_type));
		if (os_type == filetype_uimage)
			printf("and os type: %d\n", data.os->header.ih_os);
		goto err_out;
	}

	if (bootm_verbose(&data))
		printf("Passing control to %s handler\n", handler->name);

	ret = handler->bootm(&data);

	printf("handler failed with %s\n", strerror(-ret));

err_out:
	if (data.os_res)
		release_sdram_region(data.os_res);
	if (data.initrd_res)
		release_sdram_region(data.initrd_res);
	if (data.initrd && data.initrd != data.os)
		uimage_close(data.initrd);
	if (data.os)
		uimage_close(data.os);
	return 1;
}

BAREBOX_CMD_HELP_START(bootm)
BAREBOX_CMD_HELP_USAGE("bootm [OPTIONS] image\n")
BAREBOX_CMD_HELP_SHORT("Boot an application image.\n")
BAREBOX_CMD_HELP_OPT  ("-c",  "crc check uImage data\n")
#ifdef CONFIG_CMD_BOOTM_INITRD
BAREBOX_CMD_HELP_OPT  ("-r <initrd>","specify an initrd image\n")
BAREBOX_CMD_HELP_OPT  ("-L <load addr>","specify initrd load address\n")
#endif
BAREBOX_CMD_HELP_OPT  ("-a <load addr>","specify os load address\n")
BAREBOX_CMD_HELP_OPT  ("-e <ofs>","entry point to the image relative to start (0)\n")
#ifdef CONFIG_OFTREE
BAREBOX_CMD_HELP_OPT  ("-o <oftree>","specify oftree\n")
#endif
#ifdef CONFIG_CMD_BOOTM_VERBOSE
BAREBOX_CMD_HELP_OPT  ("-v","verbose\n")
#endif
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(bootm)
	.cmd		= do_bootm,
	.usage		= "boot an application image",
	BAREBOX_CMD_HELP(cmd_bootm_help)
BAREBOX_CMD_END

BAREBOX_MAGICVAR(bootargs, "Linux Kernel parameters");

/**
 * @file
 * @brief Boot support for Linux
 */

/**
 * @page boot_preparation Preparing for Boot
 *
 * This chapter describes what's to be done to forward the control from
 * barebox to Linux. This part describes the generic part, below you can find
 * the architecture specific part.
 *
 * - @subpage arm_boot_preparation
 * - @subpage ppc_boot_preparation
 * - @subpage x86_boot_preparation
 */
