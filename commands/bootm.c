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
#include <rtc.h>
#include <init.h>
#include <magicvar.h>
#include <uncompress.h>
#include <asm-generic/memory_layout.h>

struct image_handle_data* image_handle_data_get_by_num(struct image_handle* handle, int num)
{
	if (!handle || num < 0 || num >= handle->nb_data_entries)
		return NULL;

	return &handle->data_entries[num];
}

int relocate_image(struct image_handle *handle, void *load_address)
{
	image_header_t *hdr = &handle->header;
	unsigned long len  = image_get_size(hdr);
	struct image_handle_data *iha;
	void *data;
	int ret;

	iha = image_handle_data_get_by_num(handle, 0);
	data = iha->data;

	switch (image_get_comp(hdr)) {
	case IH_COMP_NONE:
		if (load_address == data) {
			printf ("   XIP ... ");
		} else {
			memmove(load_address, data, len);
		}
		break;
	default:
		printf ("   Uncompressing ... ");
		ret = uncompress(data, len, NULL, NULL, load_address, NULL,
				uncompress_err_stdout);
		if (ret)
			return ret;
		break;
	}

	return 0;
}
EXPORT_SYMBOL(relocate_image);

static LIST_HEAD(handler_list);

int register_image_handler(struct image_handler *handler)
{
	list_add_tail(&handler->list, &handler_list);
	return 0;
}

/*
 * generate a image_handle from a multi_image
 * this image_handle can be freed by unmap_image
 */
static struct image_handle *get_fake_image_handle(struct image_data *data, int num)
{
	struct image_handle *handle;
	struct image_handle_data* iha;
	image_header_t *header;

	iha = image_handle_data_get_by_num(data->os, num);

	handle = xzalloc(sizeof(struct image_handle));
	header = &handle->header;
	handle->data_entries = gen_image_handle_data(iha->data, iha->len);
	handle->nb_data_entries = 1;
	header->ih_size = cpu_to_uimage(iha->len);
	handle->data = handle->data_entries[0].data;

	return handle;
}

static int do_bootm(struct command *cmdtp, int argc, char *argv[])
{
	int	opt;
	image_header_t *os_header;
	struct image_handle *os_handle = NULL;
	struct image_handler *handler;
	struct image_data data;
	const char *initrdname = NULL;
	int ret = 1;

	memset(&data, 0, sizeof(struct image_data));
	data.verify = 1;
	data.initrd_address = ~0;

	while ((opt = getopt(argc, argv, "nr:L:")) > 0) {
		switch(opt) {
		case 'n':
			data.verify = 0;
			break;
		case 'L':
			data.initrd_address = simple_strtoul(optarg, NULL, 0);
			break;
		case 'r':
			initrdname = optarg;
			break;
		default:
			break;
		}
	}

#ifdef CONFIG_OFTREE
	data.oftree = of_get_fixed_tree();
#endif

	if (optind == argc) {
		ret = COMMAND_ERROR_USAGE;
		goto err_out;
	}

	os_handle = map_image(argv[optind], data.verify);
	if (!os_handle)
		goto err_out;
	data.os = os_handle;

	os_header = &os_handle->header;

	if (image_get_arch(os_header) != IH_ARCH) {
		printf("Unsupported Architecture 0x%x\n",
		       image_get_arch(os_header));
		goto err_out;
	}

	if (initrdname) {
		/* check for multi image @<num> */
		if (initrdname[0] == '@') {
			int num = simple_strtol(optarg + 1, NULL, 0);

			data.initrd = get_fake_image_handle(&data, num);
		} else {
			data.initrd = map_image(optarg, data.verify);
		}
		if (!data.initrd)
			goto err_out;
	}

	/*
	 * We have reached the point of no return: we are going to
	 * overwrite all exception vector code, so we cannot easily
	 * recover from any failures any more...
	 */

	puts ("OK\n");

	/*
	 * FIXME: we do not check at all whether
	 * - we will write the image to sdram
	 * - we overwrite ourselves
	 * - kernel and initrd overlap
	 */
	ret = relocate_image(data.os, (void *)image_get_load(os_header));
	if (ret)
		goto err_out;

	if (data.initrd) {
		if (data.initrd && data.initrd_address == ~0)
			data.initrd_address = uimage_to_cpu(data.initrd->header.ih_load);

		data.initrd_size = image_get_data_size(&data.initrd->header);

		ret = relocate_image(data.initrd, (void *)data.initrd_address);
		if (ret)
			goto err_out;
	}

	/* loop through the registered handlers */
	list_for_each_entry(handler, &handler_list, list) {
		if (image_get_os(os_header) == handler->image_type) {
			handler->bootm(&data);
			printf("handler returned!\n");
			goto err_out;
		}
	}

	printf("no image handler found for image type %d\n",
	       image_get_os(os_header));

err_out:
	if (os_handle)
		unmap_image(os_handle);
	if (data.initrd)
		unmap_image(data.initrd);
	return ret;
}

BAREBOX_CMD_HELP_START(bootm)
BAREBOX_CMD_HELP_USAGE("bootm [OPTIONS] image\n")
BAREBOX_CMD_HELP_SHORT("Boot an application image.\n")
BAREBOX_CMD_HELP_OPT  ("-n",  "Do not verify the image (speeds up boot process)\n")
BAREBOX_CMD_HELP_OPT  ("-r <initrd>","specify an initrd image\n")
BAREBOX_CMD_HELP_OPT  ("-L <load addr>","specify initrd load address")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(bootm)
	.cmd		= do_bootm,
	.usage		= "boot an application image",
	BAREBOX_CMD_HELP(cmd_bootm_help)
BAREBOX_CMD_END

BAREBOX_MAGICVAR(bootargs, "Linux Kernel parameters");

#ifdef CONFIG_BZLIB
void bz_internal_error(int errcode)
{
	printf ("BZIP2 internal error %d\n", errcode);
}
#endif /* CONFIG_BZLIB */

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
