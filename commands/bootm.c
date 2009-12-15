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
typedef void boot_os_Fcn (cmd_tbl_t *cmdtp, int flag,
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

#ifdef CONFIG_CMD_BOOTM_SHOW_TYPE
static const char *image_os(image_header_t *hdr)
{
	char *os;

	switch (hdr->ih_os) {
	case IH_OS_INVALID:	os = "Invalid OS";		break;
	case IH_OS_NETBSD:	os = "NetBSD";			break;
	case IH_OS_LINUX:	os = "Linux";			break;
	case IH_OS_VXWORKS:	os = "VxWorks";			break;
	case IH_OS_QNX:		os = "QNX";			break;
	case IH_OS_BAREBOX:	os = "barebox";			break;
	case IH_OS_RTEMS:	os = "RTEMS";			break;
#ifdef CONFIG_ARTOS
	case IH_OS_ARTOS:	os = "ARTOS";			break;
#endif
#ifdef CONFIG_LYNXKDI
	case IH_OS_LYNXOS:	os = "LynxOS";			break;
#endif
	default:		os = "Unknown OS";		break;
	}

	return os;
}

static const char *image_arch(image_header_t *hdr)
{
	char *arch;

	switch (hdr->ih_arch) {
	case IH_CPU_INVALID:	arch = "Invalid CPU";		break;
	case IH_CPU_ALPHA:	arch = "Alpha";			break;
	case IH_CPU_ARM:	arch = "ARM";			break;
	case IH_CPU_AVR32:	arch = "AVR32";			break;
	case IH_CPU_I386:	arch = "Intel x86";		break;
	case IH_CPU_IA64:	arch = "IA64";			break;
	case IH_CPU_MIPS:	arch = "MIPS";			break;
	case IH_CPU_MIPS64:	arch = "MIPS 64 Bit";		break;
	case IH_CPU_PPC:	arch = "PowerPC";		break;
	case IH_CPU_S390:	arch = "IBM S390";		break;
	case IH_CPU_SH:		arch = "SuperH";		break;
	case IH_CPU_SPARC:	arch = "SPARC";			break;
	case IH_CPU_SPARC64:	arch = "SPARC 64 Bit";		break;
	case IH_CPU_M68K:	arch = "M68K"; 			break;
	case IH_CPU_MICROBLAZE:	arch = "Microblaze"; 		break;
	case IH_CPU_NIOS:	arch = "Nios";			break;
	case IH_CPU_NIOS2:	arch = "Nios-II";		break;
	default:		arch = "Unknown Architecture";	break;
	}

	return arch;
}

static const char *image_type(image_header_t *hdr)
{
	char *type;

	switch (hdr->ih_type) {
	case IH_TYPE_INVALID:	type = "Invalid Image";		break;
	case IH_TYPE_STANDALONE:type = "Standalone Program";	break;
	case IH_TYPE_KERNEL:	type = "Kernel Image";		break;
	case IH_TYPE_RAMDISK:	type = "RAMDisk Image";		break;
	case IH_TYPE_MULTI:	type = "Multi-File Image";	break;
	case IH_TYPE_FIRMWARE:	type = "Firmware";		break;
	case IH_TYPE_SCRIPT:	type = "Script";		break;
	case IH_TYPE_FLATDT:	type = "Flat Device Tree";	break;
	default:		type = "Unknown Image";		break;
	}
	return type;
}

static const char *image_compression(image_header_t *hdr)
{
	char *comp;

	switch (hdr->ih_comp) {
	case IH_COMP_NONE:	comp = "uncompressed";		break;
	case IH_COMP_GZIP:	comp = "gzip compressed";	break;
	case IH_COMP_BZIP2:	comp = "bzip2 compressed";	break;
	default:		comp = "unknown compression";	break;
	}

	return comp;
}
#endif

int relocate_image(struct image_handle *handle, void *load_address)
{
	image_header_t *hdr = &handle->header;
	unsigned long len  = ntohl(hdr->ih_size);
	unsigned long data = (unsigned long)(handle->data);

#if defined CONFIG_CMD_BOOTM_ZLIB || defined CONFIG_CMD_BOOTM_BZLIB
	uint	unc_len = CFG_BOOTM_LEN;
#endif

	switch (hdr->ih_comp) {
	case IH_COMP_NONE:
		if(ntohl(hdr->ih_load) == data) {
			printf ("   XIP ... ");
		} else {
			memmove ((void *) ntohl(hdr->ih_load), (uchar *)data, len);
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
		printf ("Unimplemented compression type %d\n", hdr->ih_comp);
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

	if (read(fd, header, sizeof(image_header_t)) < 0) {
		printf("could not read: %s\n", errno_str());
		goto err_out;
	}

	if (ntohl(header->ih_magic) != IH_MAGIC) {
		puts ("Bad Magic Number\n");
		goto err_out;
	}

	checksum = ntohl(header->ih_hcrc);
	header->ih_hcrc = 0;

	if (crc32 (0, (uchar *)header, sizeof(image_header_t)) != checksum) {
		puts ("Bad Header Checksum\n");
		goto err_out;
	}
	len  = ntohl(header->ih_size);

	handle->data = memmap(fd, PROT_READ);
	if (handle->data == (void *)-1) {
		handle->data = xmalloc(len);
		handle->flags = IH_MALLOC;
		if (read(fd, handle->data, len) < 0) {
			printf("could not read: %s\n", errno_str());
			goto err_out;
		}
	} else {
		handle->data = (void *)((unsigned long)handle->data + sizeof(image_header_t));
	}

	if (verify) {
		puts ("   Verifying Checksum ... ");
		if (crc32 (0, handle->data, len) != ntohl(header->ih_dcrc)) {
			printf ("Bad Data CRC\n");
			goto err_out;
		}
		puts ("OK\n");
	}

	print_image_hdr(header);

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

LIST_HEAD(handler_list);

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

static int do_bootm (cmd_tbl_t *cmdtp, int argc, char *argv[])
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

	if (os_header->ih_arch != IH_CPU)	{
		printf ("Unsupported Architecture 0x%x\n", os_header->ih_arch);
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
		if (handler->image_type == os_header->ih_os) {
			handler->bootm(&data);
			printf("handler returned!\n");
			goto err_out;
		}
	}

	printf("no image handler found for image type %d\n", os_header->ih_os);

err_out:
	if (os_handle)
		unmap_image(os_handle);
	if (initrd_handle)
		unmap_image(initrd_handle);
	return 1;
}

static const __maybe_unused char cmd_bootm_help[] =
"Usage: bootm [OPTION] image\n"
"Boot application image\n"
" -n             do not verify the images (speeds up boot process)\n"
" -h             show advanced options\n";


BAREBOX_CMD_START(bootm)
	.cmd		= do_bootm,
	.usage		= "boot application image",
	BAREBOX_CMD_HELP(cmd_bootm_help)
BAREBOX_CMD_END

#ifdef CONFIG_CMD_IMI
static int do_iminfo ( cmd_tbl_t *cmdtp, int argc, char *argv[])
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
	memmove (&header, (char *)addr, sizeof(image_header_t));

	if (ntohl(hdr->ih_magic) != IH_MAGIC) {
		puts ("   Bad Magic Number\n");
		return 1;
	}

	data = (ulong)&header;
	len  = sizeof(image_header_t);

	checksum = ntohl(hdr->ih_hcrc);
	hdr->ih_hcrc = 0;

	if (crc32 (0, (uchar *)data, len) != checksum) {
		puts ("   Bad Header Checksum\n");
		return 1;
	}

	/* for multi-file images we need the data part, too */
	print_image_hdr ((image_header_t *)addr);

	data = addr + sizeof(image_header_t);
	len  = ntohl(hdr->ih_size);

	puts ("   Verifying Checksum ... ");
	if (crc32 (0, (uchar *)data, len) != ntohl(hdr->ih_dcrc)) {
		puts ("   Bad Data CRC\n");
		return 1;
	}
	puts ("OK\n");
	return 0;
}

BAREBOX_CMD(
	iminfo,		1,	do_iminfo,
	"iminfo  - print header information for application image\n",
	"addr [addr ...]\n"
	"    - print header information for application image starting at\n"
	"      address 'addr' in memory; this includes verification of the\n"
	"      image contents (magic number, header and payload checksums)\n"
);

#endif	/* CONFIG_CMD_IMI */

void
print_image_hdr (image_header_t *hdr)
{
#if defined(CONFIG_CMD_DATE) || defined(CONFIG_TIMESTAMP)
	time_t timestamp = (time_t)ntohl(hdr->ih_time);
	struct rtc_time tm;
#endif

	printf ("   Image Name:   %.*s\n", IH_NMLEN, hdr->ih_name);
#if defined(CONFIG_CMD_DATE) || defined(CONFIG_TIMESTAMP)
	to_tm (timestamp, &tm);
	printf ("   Created:      %4d-%02d-%02d  %2d:%02d:%02d UTC\n",
		tm.tm_year, tm.tm_mon, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);
#endif	/* CONFIG_CMD_DATE, CONFIG_TIMESTAMP */
#ifdef CONFIG_CMD_BOOTM_SHOW_TYPE
	printf ("   Image Type:   %s %s %s (%s)\n", image_arch(hdr), image_os(hdr),
			image_type(hdr), image_compression(hdr));
#endif
	printf ("   Data Size:    %d Bytes = %s\n"
		"   Load Address: %08x\n"
		"   Entry Point:  %08x\n",
			ntohl(hdr->ih_size),
			size_human_readable(ntohl(hdr->ih_size)),
			ntohl(hdr->ih_load),
			ntohl(hdr->ih_ep));

	if (hdr->ih_type == IH_TYPE_MULTI) {
		int i;
		ulong len;
		ulong *len_ptr = (ulong *)((ulong)hdr + sizeof(image_header_t));

		puts ("   Contents:\n");
		for (i=0; (len = ntohl(*len_ptr)); ++i, ++len_ptr) {
			printf ("   Image %d: %8ld Bytes = %s", i, len,
				size_human_readable (len));
		}
	}
}

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
 * - @subpage m68k_boot_preparation
 */
