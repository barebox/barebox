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
#include <watchdog.h>
#include <driver.h>
#include <command.h>
#include <image.h>
#include <malloc.h>
#include <zlib.h>
#include <bzlib.h>
#include <environment.h>
#include <asm/byteorder.h>
#include <xfuncs.h>
#include <getopt.h>
#include <fcntl.h>
#include <fs.h>
#include <errno.h>
#include <boot.h>
#include <rtc.h>
#include <init.h>
#include <asm-generic/memory_layout.h>

/*
 *  Continue booting an OS image; caller already has:
 *  - copied image header to global variable `header'
 *  - checked header magic number, checksums (both header & image),
 *  - verified image architecture (PPC) and type (KERNEL or MULTI),
 *  - loaded (first part of) image to header load address,
 *  - disabled interrupts.
 */
typedef void boot_os_Fcn(struct command *cmdtp, int flag,
			  int	argc, char *argv[],
			  ulong	addr,		/* of image to boot */
			  ulong	*len_ptr,	/* multi-file image length table */
			  int	verify);	/* getenv("verify")[0] != 'n' */

#ifndef CFG_BOOTM_LEN
#define CFG_BOOTM_LEN	0x800000	/* use 8MByte as default max gunzip size */
#endif

#ifdef CONFIG_SILENT_CONSOLE
static void
fixup_silent_linux ()
{
	char buf[256], *start, *end;
	char *cmdline = getenv ("bootargs");

	/* Only fix cmdline when requested */
	if (!(gd->flags & GD_FLG_SILENT))
		return;

	debug ("before silent fix-up: %s\n", cmdline);
	if (cmdline) {
		if ((start = strstr (cmdline, "console=")) != NULL) {
			end = strchr (start, ' ');
			strncpy (buf, cmdline, (start - cmdline + 8));
			if (end)
				strcpy (buf + (start - cmdline + 8), end);
			else
				buf[start - cmdline + 8] = '\0';
		} else {
			strcpy (buf, cmdline);
			strcat (buf, " console=");
		}
	} else {
		strcpy (buf, "console=");
	}

	setenv ("bootargs", buf);
	debug ("after silent fix-up: %s\n", buf);
}
#endif /* CONFIG_SILENT_CONSOLE */

int relocate_image(struct image_handle *handle, void *load_address)
{
	image_header_t *hdr = &handle->header;
	unsigned long len  = image_get_size(hdr);
	unsigned long data = (unsigned long)(handle->data);

#if defined CONFIG_CMD_BOOTM_ZLIB || defined CONFIG_CMD_BOOTM_BZLIB
	uint	unc_len = CFG_BOOTM_LEN;
#endif

	switch (image_get_comp(hdr)) {
	case IH_COMP_NONE:
		if(image_get_load(hdr) == data) {
			printf ("   XIP ... ");
		} else {
			memmove ((void *) image_get_load(hdr), (uchar *)data, len);
		}
		break;
#ifdef CONFIG_CMD_BOOTM_ZLIB
	case IH_COMP_GZIP:
		printf ("   Uncompressing ... ");
		if (gunzip (load_address, unc_len,
			    (uchar *)data, &len) != 0)
			return -1;
		break;
#endif
#ifdef CONFIG_CMD_BOOTM_BZLIB
	case IH_COMP_BZIP2:
		printf ("   Uncompressing ... ");
		/*
		 * If we've got less than 4 MB of malloc() space,
		 * use slower decompression algorithm which requires
		 * at most 2300 KB of memory.
		 */
		if (BZ2_bzBuffToBuffDecompress (load_address,
						&unc_len, (char *)data, len,
						MALLOC_SIZE < (4096 * 1024), 0)
						!= BZ_OK)
			return -1;
		break;
#endif
	default:
		printf("Unimplemented compression type %d\n",
		       image_get_comp(hdr));
		return -1;
	}

	return 0;
}
EXPORT_SYMBOL(relocate_image);

struct image_handle *map_image(const char *filename, int verify)
{
	int fd;
	uint32_t checksum, len;
	struct image_handle *handle;
	image_header_t *header;

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		printf("could not open: %s\n", errno_str());
		return NULL;
	}

	handle = xzalloc(sizeof(struct image_handle));
	header = &handle->header;

	if (read(fd, header, image_get_header_size()) < 0) {
		printf("could not read: %s\n", errno_str());
		goto err_out;
	}

	if (image_get_magic(header) != IH_MAGIC) {
		puts ("Bad Magic Number\n");
		goto err_out;
	}

	checksum = image_get_hcrc(header);
	header->ih_hcrc = 0;

	if (crc32 (0, (uchar *)header, image_get_header_size()) != checksum) {
		puts ("Bad Header Checksum\n");
		goto err_out;
	}
	len  = image_get_size(header);

	handle->data = memmap(fd, PROT_READ);
	if (handle->data == (void *)-1) {
		handle->data = xmalloc(len);
		handle->flags = IH_MALLOC;
		if (read(fd, handle->data, len) < 0) {
			printf("could not read: %s\n", errno_str());
			goto err_out;
		}
	} else {
		handle->data = (void *)((unsigned long)handle->data +
						       image_get_header_size());
	}

	if (verify) {
		puts ("   Verifying Checksum ... ");
		if (crc32 (0, handle->data, len) != image_get_dcrc(header)) {
			printf ("Bad Data CRC\n");
			goto err_out;
		}
		puts ("OK\n");
	}

	image_print_contents(header);

	close(fd);

	return handle;
err_out:
	close(fd);
	if (handle->flags & IH_MALLOC)
		free(handle->data);
	free(handle);
	return NULL;
}
EXPORT_SYMBOL(map_image);

void unmap_image(struct image_handle *handle)
{
	if (handle->flags & IH_MALLOC)
		free(handle->data);
	free(handle);
}
EXPORT_SYMBOL(unmap_image);

static LIST_HEAD(handler_list);

int register_image_handler(struct image_handler *handler)
{
	list_add_tail(&handler->list, &handler_list);
	return 0;
}

static int initrd_handler_parse_options(struct image_data *data, int opt,
		char *optarg)
{
	switch(opt) {
	case 'r':
		printf("use initrd %s\n", optarg);
		data->initrd = map_image(optarg, data->verify);
		if (!data->initrd)
			return -1;
		return 0;
	default:
		return 1;
	}
}

static struct image_handler initrd_handler = {
	.cmdline_options = "r:",
	.cmdline_parse = initrd_handler_parse_options,
	.help_string = " -r <initrd>    specify an initrd image",
};

static int initrd_register_image_handler(void)
{
	return register_image_handler(&initrd_handler);
}

late_initcall(initrd_register_image_handler);

static int handler_parse_options(struct image_data *data, int opt, char *optarg)
{
	struct image_handler *handler;
	int ret;

	list_for_each_entry(handler, &handler_list, list) {
		if (!handler->cmdline_parse)
			continue;

		ret = handler->cmdline_parse(data, opt, optarg);
		if (ret > 0)
			continue;

		return ret;
	}

	return -1;
}

static int do_bootm(struct command *cmdtp, int argc, char *argv[])
{
	ulong	iflag;
	int	opt;
	image_header_t *os_header;
	struct image_handle *os_handle, *initrd_handle = NULL;
	struct image_handler *handler;
	struct image_data data;
	char options[53]; /* worst case: whole alphabet with colons */

	memset(&data, 0, sizeof(struct image_data));
	data.verify = 1;

	/* Collect options from registered handlers */
	strcpy(options, "nh");
	list_for_each_entry(handler, &handler_list, list) {
		if (handler->cmdline_options)
			strcat(options, handler->cmdline_options);
	}

	while((opt = getopt(argc, argv, options)) > 0) {
		switch(opt) {
		case 'n':
			data.verify = 0;
			break;
		case 'h':
			printf("bootm advanced options:\n");

			list_for_each_entry(handler, &handler_list, list) {
				if (handler->help_string)
					printf("%s\n", handler->help_string);
			}

			return 0;
		default:
			if (!handler_parse_options(&data, opt, optarg))
				continue;

			return 1;
		}
	}

	if (optind == argc)
		return COMMAND_ERROR_USAGE;

	os_handle = map_image(argv[optind], data.verify);
	if (!os_handle)
		return 1;
	data.os = os_handle;

	os_header = &os_handle->header;

	if (image_get_arch(os_header) != IH_ARCH) {
		printf("Unsupported Architecture 0x%x\n",
		       image_get_arch(os_header));
		goto err_out;
	}

	/*
	 * We have reached the point of no return: we are going to
	 * overwrite all exception vector code, so we cannot easily
	 * recover from any failures any more...
	 */

	iflag = disable_interrupts();

	puts ("OK\n");

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
	if (initrd_handle)
		unmap_image(initrd_handle);
	return 1;
}

BAREBOX_CMD_HELP_START(bootm)
BAREBOX_CMD_HELP_USAGE("bootm [-n] image\n")
BAREBOX_CMD_HELP_SHORT("Boot an application image.\n")
BAREBOX_CMD_HELP_OPT  ("-n",  "Do not verify the image (speeds up boot process)\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(bootm)
	.cmd		= do_bootm,
	.usage		= "boot an application image",
	BAREBOX_CMD_HELP(cmd_bootm_help)
BAREBOX_CMD_END

/**
 * @page bootm_command

\todo What does bootm do, what kind of image does it boot?

 */

#ifdef CONFIG_CMD_IMI
static int do_iminfo(struct command *cmdtp, int argc, char *argv[])
{
	int	arg;
	ulong	addr;
	int     rcode=0;

	if (argc < 2) {
		return image_info (load_addr);
	}

	for (arg=1; arg <argc; ++arg) {
		addr = simple_strtoul(argv[arg], NULL, 16);
		if (image_info (addr) != 0) rcode = 1;
	}
	return rcode;
}

static int image_info (ulong addr)
{
	ulong	data, len, checksum;
	image_header_t *hdr = &header;

	printf ("\n## Checking Image at %08lx ...\n", addr);

	/* Copy header so we can blank CRC field for re-calculation */
	memmove (&header, (char *)addr, image_get_header_size());

	if (image_get_magic(hdr) != IH_MAGIC) {
		puts ("   Bad Magic Number\n");
		return 1;
	}

	data = (ulong)&header;
	len  = image_get_header_size();

	checksum = image_get_hcrc(hdr);
	hdr->ih_hcrc = 0;

	if (crc32 (0, (uchar *)data, len) != checksum) {
		puts ("   Bad Header Checksum\n");
		return 1;
	}

	/* for multi-file images we need the data part, too */
	print_image_hdr ((image_header_t *)addr);

	data = addr + image_get_header_size();
	len  = image_get_size(hdr);

	puts ("   Verifying Checksum ... ");
	if (crc32 (0, (uchar *)data, len) != image_get_dcrc(hdr)) {
		puts ("   Bad Data CRC\n");
		return 1;
	}
	puts ("OK\n");
	return 0;
}

BAREBOX_CMD_HELP_START(iminfo)
BAREBOX_CMD_HELP_USAGE("iminfo\n")
BAREBOX_CMD_HELP_SHORT("Print header information for an application image.\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(iminfo)
	.cmd		= do_iminfo,
	.usage		= "print header information for an application image",
	BAREBOX_CMD_HELP(cmd_iminfo_help)
BAREBOX_CMD_END

#endif	/* CONFIG_CMD_IMI */

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
