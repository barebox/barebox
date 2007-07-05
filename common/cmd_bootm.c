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

 /*cmd_boot.c*/

#if (CONFIG_COMMANDS & CFG_CMD_DATE) || defined(CONFIG_TIMESTAMP)
#include <rtc.h>
#endif

#ifdef CONFIG_HUSH_PARSER
#include <hush.h>
#endif

#ifdef CONFIG_SHOW_BOOT_PROGRESS
# include <status_led.h>
# define SHOW_BOOT_PROGRESS(arg)	show_boot_progress(arg)
#else
# define SHOW_BOOT_PROGRESS(arg)
#endif

#ifdef CFG_INIT_RAM_LOCK
#include <asm/cache.h>
#endif

#ifdef CONFIG_LOGBUFFER
#include <logbuff.h>
#endif

/*
 * Some systems (for example LWMON) have very short watchdog periods;
 * we must make sure to split long operations like memmove() or
 * crc32() into reasonable chunks.
 */
#if defined(CONFIG_HW_WATCHDOG) || defined(CONFIG_WATCHDOG)
# define CHUNKSZ (64 * 1024)
#endif

#ifdef CONFIG_CMD_BOOTM_ZLIB
int  gunzip (void *, int, unsigned char *, unsigned long *);

static void *zalloc(void *, unsigned, unsigned);
static void zfree(void *, void *, unsigned);
#endif

#if (CONFIG_COMMANDS & CFG_CMD_IMI)
static int image_info (unsigned long addr);
#endif

#if (CONFIG_COMMANDS & CFG_CMD_IMLS)
#include <flash.h>
static int do_imls (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
#endif

static void print_type (image_header_t *hdr);

#ifdef __I386__
image_header_t *fake_header(image_header_t *hdr, void *ptr, int size);
#endif

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

#ifdef	DEBUG
extern int do_bdinfo ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
#endif

#ifdef CONFIG_SILENT_CONSOLE
static void fixup_silent_linux (void);
#endif
#if (CONFIG_COMMANDS & CFG_CMD_ELF)
int do_bootvx ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[] );
int do_bootelf (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[] );
#endif /* CFG_CMD_ELF */
#ifdef CONFIG_LYNXKDI
extern void lynxkdi_boot( image_header_t * );
#endif

#ifndef CFG_BOOTM_LEN
#define CFG_BOOTM_LEN	0x800000	/* use 8MByte as default max gunzip size */
#endif

#ifdef CONFIG_OF_FLAT_TREE
#define OPT_OFTREE "o:"
#else
#define OPT_OFTREE
#endif

int relocate_image(image_header_t *hdr, void *load_address)
{
	unsigned long data = (unsigned long)(hdr + 1);
	unsigned long len  = ntohl(hdr->ih_size);
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
		i = BZ2_bzBuffToBuffDecompress (load_address,
						&unc_len, (char *)data, len,
						CFG_MALLOC_LEN < (4096 * 1024), 0);
		if (i != BZ_OK)
			return -1;
		break;
#endif
	default:
		printf ("Unimplemented compression type %d\n", hdr->ih_comp);
		return -1;
	}

	return 0;
}

image_header_t *map_image(const char *filename, int verify)
{
	int fd;
	uint32_t checksum, len;
	void *data;
	image_header_t *header, *tmp_header;

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		printf("could not open: %s\n", errno_str());
		return NULL;
	}

	header = xmalloc(sizeof(image_header_t));

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

	tmp_header = realloc(header, sizeof(image_header_t) + len);
	if (!tmp_header)
		goto err_out;
	header = tmp_header;

	data = (void *)(header + 1);

	if (read(fd, data, len) < 0) {
		printf("could not read: %s\n", errno_str());
		goto err_out;
	}

	if (verify) {
		puts ("   Verifying Checksum ... ");
		if (crc32 (0, data, len) != ntohl(header->ih_dcrc)) {
			printf ("Bad Data CRC\n");
			goto err_out;
		}
		puts ("OK\n");
	}

	print_image_hdr (header);

	close(fd);

	return header;
err_out:
	close(fd);
	free(header);
	return NULL;
}

int do_bootm (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong	iflag;
	int	verify = 1;
	int	opt;
	char	*initrd = NULL;
	image_header_t *os_header;
	image_header_t *initrd_header = NULL;
#ifdef CONFIG_OF_FLAT_TREE
	char	*oftree = NULL;
#endif
	int ignore_load_address = 0;

	getopt_reset();

	while((opt = getopt(argc, argv, "r:ni" OPT_OFTREE)) > 0) {
		switch(opt) {
		case 'r':
			initrd = optarg;
			break;
		case 'n':
			verify = 0;
			break;
		case 'i':
			ignore_load_address = 1;
			break;
#ifdef CONFIG_OF_FLAT_TREE
		case 'o':
			oftree = optarg;
			break;
#endif
		default:
			return 1;
		}
	}

	if (optind == argc) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	os_header = map_image(argv[optind], verify);
	if (!os_header)
		return 1;

	if (os_header->ih_arch != IH_CPU)	{
		printf ("Unsupported Architecture 0x%x\n", os_header->ih_arch);
		goto err_out;
	}

	if (initrd) {
		initrd_header = map_image(initrd, verify);
		if (!initrd_header)
			goto err_out;
	}
#if 0
	switch (os_header->ih_type) {
	case IH_TYPE_STANDALONE:
		name = "Standalone Application";
		/* A second argument overwrites the load address */
		if (argc > 2) {
			os_header->ih_load = htonl(simple_strtoul(argv[2], NULL, 16));
		}
		break;
	case IH_TYPE_KERNEL:
		name = "Kernel Image";
		break;
	case IH_TYPE_MULTI:
		name = "Multi-File Image";
		len  = ntohl(len_ptr[0]);
		/* OS kernel is always the first image */
		data += 8; /* kernel_len + terminator */
		for (i=1; len_ptr[i]; ++i)
			data += 4;
		break;
	default: printf ("Wrong Image Type for %s command\n", cmdtp->name);
		return 1;
	}
#endif
	/*
	 * We have reached the point of no return: we are going to
	 * overwrite all exception vector code, so we cannot easily
	 * recover from any failures any more...
	 */

	iflag = disable_interrupts();

	puts ("OK\n");

	switch (os_header->ih_os) {
	default:			/* handled by (original) Linux case */
	case IH_OS_LINUX:
#ifdef CONFIG_SILENT_CONSOLE
		fixup_silent_linux();
#endif
#ifdef CONFIG_OF_FLAT_TREE
		do_bootm_linux(os_header, initrd_header, oftree);
#else
		do_bootm_linux(os_header, initrd_header);
#endif
	    break;
#ifdef CONFIG_NETBSD
	case IH_OS_NETBSD:
	    do_bootm_netbsd (cmdtp, flag, argc, argv,
			     addr, len_ptr, verify);
	    break;
#endif

#ifdef CONFIG_LYNXKDI
	case IH_OS_LYNXOS:
	    do_bootm_lynxkdi (cmdtp, flag, argc, argv,
			     addr, len_ptr, verify);
	    break;
#endif

#ifdef CONFIG_RTEMS
	case IH_OS_RTEMS:
	    do_bootm_rtems (cmdtp, flag, argc, argv,
			     addr, len_ptr, verify);
	    break;
#endif

#if (CONFIG_COMMANDS & CFG_CMD_ELF)
	case IH_OS_VXWORKS:
	    do_bootm_vxworks (cmdtp, flag, argc, argv,
			      addr, len_ptr, verify);
	    break;
	case IH_OS_QNX:
	    do_bootm_qnxelf (cmdtp, flag, argc, argv,
			      addr, len_ptr, verify);
	    break;
#endif /* CFG_CMD_ELF */
#ifdef CONFIG_ARTOS
	case IH_OS_ARTOS:
	    do_bootm_artos  (cmdtp, flag, argc, argv,
			     addr, len_ptr, verify);
	    break;
#endif
	}

	SHOW_BOOT_PROGRESS (-9);
#ifdef DEBUG
	puts ("\n## Control returned to monitor - resetting...\n");
	do_reset (cmdtp, flag, argc, argv);
#endif

err_out:
	if (os_header)
		free(os_header);
	if (initrd_header)
		free(initrd_header);
	return 1;
}

U_BOOT_CMD_START(bootm)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_bootm,
	.usage		= "boot application image from memory",
	U_BOOT_CMD_HELP(
 	"[addr [arg ...]]\n    - boot application image stored in memory\n"
 	"\tpassing arguments 'arg ...'; when booting a Linux kernel,\n"
 	"\t'arg' can be the address of an initrd image\n"
#ifdef CONFIG_OF_FLAT_TREE
	"\tWhen booting a Linux kernel which requires a flat device-tree\n"
	"\ta third argument is required which is the address of the of the\n"
	"\tdevice-tree blob. To boot that kernel without an initrd image,\n"
	"\tuse a '-' for the second argument. If you do not pass a third\n"
	"\ta bd_info struct will be passed instead\n"
#endif
	)
U_BOOT_CMD_END

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

#ifdef CONFIG_NETBSD
static void
do_bootm_netbsd (cmd_tbl_t *cmdtp, int flag,
		int	argc, char *argv[],
		ulong	addr,
		ulong	*len_ptr,
		int	verify)
{
	image_header_t *hdr = &header;
#if 0
	void	(*loader)(bd_t *, image_header_t *, char *, char *);
#endif
	image_header_t *img_addr;
	char     *consdev;
	char     *cmdline;


	/*
	 * Booting a (NetBSD) kernel image
	 *
	 * This process is pretty similar to a standalone application:
	 * The (first part of an multi-) image must be a stage-2 loader,
	 * which in turn is responsible for loading & invoking the actual
	 * kernel.  The only differences are the parameters being passed:
	 * besides the board info strucure, the loader expects a command
	 * line, the name of the console device, and (optionally) the
	 * address of the original image header.
	 */

	img_addr = 0;
	if ((hdr->ih_type==IH_TYPE_MULTI) && (len_ptr[1]))
		img_addr = (image_header_t *) addr;


	consdev = "";
#if   defined (CONFIG_8xx_CONS_SMC1)
	consdev = "smc1";
#elif defined (CONFIG_8xx_CONS_SMC2)
	consdev = "smc2";
#elif defined (CONFIG_8xx_CONS_SCC2)
	consdev = "scc2";
#elif defined (CONFIG_8xx_CONS_SCC3)
	consdev = "scc3";
#endif

	if (argc > 2) {
		ulong len;
		int   i;

		for (i=2, len=0 ; i<argc ; i+=1)
			len += strlen (argv[i]) + 1;
		cmdline = malloc (len);

		for (i=2, len=0 ; i<argc ; i+=1) {
			if (i > 2)
				cmdline[len++] = ' ';
			strcpy (&cmdline[len], argv[i]);
			len += strlen (argv[i]);
		}
	} else if ((cmdline = (char *)getenv("bootargs")) == NULL) {
		cmdline = "";
	}
#if 0
	loader = (void (*)(bd_t *, image_header_t *, char *, char *)) ntohl(hdr->ih_ep);

	printf ("## Transferring control to NetBSD stage-2 loader (at address %08lx) ...\n",
		(ulong)loader);
#endif
	SHOW_BOOT_PROGRESS (15);

	/*
	 * NetBSD Stage-2 Loader Parameters:
	 *   r3: ptr to board info data
	 *   r4: image address
	 *   r5: console device
	 *   r6: boot args string
	 */
#if 0
	(*loader) (gd->bd, img_addr, consdev, cmdline);
#else
#warning NetBSD Support is broken
#endif
}
#endif

#ifdef CONFIG_ARTOS

/* Function that returns a character from the environment */
extern uchar (*env_get_char)(int);

static void
do_bootm_artos (cmd_tbl_t *cmdtp, int flag,
		int	argc, char *argv[],
		ulong	addr,
		ulong	*len_ptr,
		int	verify)
{
	ulong top;
	char *s, *cmdline;
	char **fwenv, **ss;
	int i, j, nxt, len, envno, envsz;
	bd_t *kbd;
	void (*entry)(bd_t *bd, char *cmdline, char **fwenv, ulong top);
	image_header_t *hdr = &header;

	/*
	 * Booting an ARTOS kernel image + application
	 */

	/* this used to be the top of memory, but was wrong... */

	/* get stack pointer */
	asm volatile ("mr %0,1" : "=r"(top) );

	debug ("## Current stack ends at 0x%08lX ", top);

	top -= 2048;		/* just to be sure */
	if (top > CFG_BOOTMAPSZ)
		top = CFG_BOOTMAPSZ;
	top &= ~0xF;

	debug ("=> set upper limit to 0x%08lX\n", top);

	/* first check the artos specific boot args, then the linux args*/
	if ((s = getenv("abootargs")) == NULL && (s = getenv("bootargs")) == NULL)
		s = "";

	/* get length of cmdline, and place it */
	len = strlen(s);
	top = (top - (len + 1)) & ~0xF;
	cmdline = (char *)top;
	debug ("## cmdline at 0x%08lX ", top);
	strcpy(cmdline, s);

	/* copy bdinfo */
	top = (top - sizeof(bd_t)) & ~0xF;
	debug ("## bd at 0x%08lX ", top);
	kbd = (bd_t *)top;
	memcpy(kbd, gd->bd, sizeof(bd_t));

	/* first find number of env entries, and their size */
	envno = 0;
	envsz = 0;
	for (i = 0; env_get_char(i) != '\0'; i = nxt + 1) {
		for (nxt = i; env_get_char(nxt) != '\0'; ++nxt)
			;
		envno++;
		envsz += (nxt - i) + 1;	/* plus trailing zero */
	}
	envno++;	/* plus the terminating zero */
	debug ("## %u envvars total size %u ", envno, envsz);

	top = (top - sizeof(char **)*envno) & ~0xF;
	fwenv = (char **)top;
	debug ("## fwenv at 0x%08lX ", top);

	top = (top - envsz) & ~0xF;
	s = (char *)top;
	ss = fwenv;

	/* now copy them */
	for (i = 0; env_get_char(i) != '\0'; i = nxt + 1) {
		for (nxt = i; env_get_char(nxt) != '\0'; ++nxt)
			;
		*ss++ = s;
		for (j = i; j < nxt; ++j)
			*s++ = env_get_char(j);
		*s++ = '\0';
	}
	*ss++ = NULL;	/* terminate */

	entry = (void (*)(bd_t *, char *, char **, ulong))ntohl(hdr->ih_ep);
	(*entry)(kbd, cmdline, fwenv, top);
}
#endif

int do_bootd (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int rcode = 0;
#ifndef CONFIG_HUSH_PARSER
	if (run_command (getenv ("bootcmd"), flag) < 0) rcode = 1;
#else
	if (parse_string_outer(getenv("bootcmd"),
		FLAG_PARSE_SEMICOLON | FLAG_EXIT_FROM_LOOP) != 0 ) rcode = 1;
#endif
	return rcode;
}

U_BOOT_CMD_START(boot)
	.maxargs	= 1,
	.cmd		= do_bootd,
	.usage		= "boot default, i.e., run 'bootcmd'",
U_BOOT_CMD_END

#if 0
int do_iminfo ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
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

U_BOOT_CMD(
	iminfo,	CONFIG_MAXARGS,	1,	do_iminfo,
	"iminfo  - print header information for application image\n",
	"addr [addr ...]\n"
	"    - print header information for application image starting at\n"
	"      address 'addr' in memory; this includes verification of the\n"
	"      image contents (magic number, header and payload checksums)\n"
);

#endif	/* CFG_CMD_IMI */

#if (CONFIG_COMMANDS & CFG_CMD_IMLS)
#if 0
/*-----------------------------------------------------------------------
 * List all images found in flash.
 */
int do_imls (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	flash_info_t *info;
	int i, j;
	image_header_t *hdr;
	ulong data, len, checksum;

	for (i=0, info=&flash_info[0]; i<CFG_MAX_FLASH_BANKS; ++i, ++info) {
		if (info->flash_id == FLASH_UNKNOWN)
			goto next_bank;
		for (j=0; j<info->sector_count; ++j) {

			if (!(hdr=(image_header_t *)info->start[j]) ||
			    (ntohl(hdr->ih_magic) != IH_MAGIC))
				goto next_sector;

			/* Copy header so we can blank CRC field for re-calculation */
			memmove (&header, (char *)hdr, sizeof(image_header_t));

			checksum = ntohl(header.ih_hcrc);
			header.ih_hcrc = 0;

			if (crc32 (0, (uchar *)&header, sizeof(image_header_t))
			    != checksum)
				goto next_sector;

			printf ("Image at %08lX:\n", (ulong)hdr);
			print_image_hdr( hdr );

			data = (ulong)hdr + sizeof(image_header_t);
			len  = ntohl(hdr->ih_size);

			puts ("   Verifying Checksum ... ");
			if (crc32 (0, (uchar *)data, len) != ntohl(hdr->ih_dcrc)) {
				puts ("   Bad Data CRC\n");
			}
			puts ("OK\n");
next_sector:		;
		}
next_bank:	;
	}

	return (0);
}

U_BOOT_CMD(
	imls,	1,		1,	do_imls,
	"imls    - list all images found in flash\n",
	"\n"
	"    - Prints information about all images found at sector\n"
	"      boundaries in flash.\n"
);
#endif	/* CFG_CMD_IMLS */
#endif

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
	puts ("   Image Type:   "); print_type(hdr);
	printf ("\n   Data Size:    %d Bytes = ", ntohl(hdr->ih_size));
	print_size (ntohl(hdr->ih_size), "\n");
	printf ("   Load Address: %08x\n"
		"   Entry Point:  %08x\n",
		 ntohl(hdr->ih_load), ntohl(hdr->ih_ep));

	if (hdr->ih_type == IH_TYPE_MULTI) {
		int i;
		ulong len;
		ulong *len_ptr = (ulong *)((ulong)hdr + sizeof(image_header_t));

		puts ("   Contents:\n");
		for (i=0; (len = ntohl(*len_ptr)); ++i, ++len_ptr) {
			printf ("   Image %d: %8ld Bytes = ", i, len);
			print_size (len, "\n");
		}
	}
}


static void
print_type (image_header_t *hdr)
{
	char *os, *arch, *type, *comp;

	switch (hdr->ih_os) {
	case IH_OS_INVALID:	os = "Invalid OS";		break;
	case IH_OS_NETBSD:	os = "NetBSD";			break;
	case IH_OS_LINUX:	os = "Linux";			break;
	case IH_OS_VXWORKS:	os = "VxWorks";			break;
	case IH_OS_QNX:		os = "QNX";			break;
	case IH_OS_U_BOOT:	os = "U-Boot";			break;
	case IH_OS_RTEMS:	os = "RTEMS";			break;
#ifdef CONFIG_ARTOS
	case IH_OS_ARTOS:	os = "ARTOS";			break;
#endif
#ifdef CONFIG_LYNXKDI
	case IH_OS_LYNXOS:	os = "LynxOS";			break;
#endif
	default:		os = "Unknown OS";		break;
	}

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

	switch (hdr->ih_comp) {
	case IH_COMP_NONE:	comp = "uncompressed";		break;
	case IH_COMP_GZIP:	comp = "gzip compressed";	break;
	case IH_COMP_BZIP2:	comp = "bzip2 compressed";	break;
	default:		comp = "unknown compression";	break;
	}

	printf ("%s %s %s (%s)", arch, os, type, comp);
}

#ifdef CONFIG_ZLIB

#define	ZALLOC_ALIGNMENT	16

static void *zalloc(void *x, unsigned items, unsigned size)
{
	void *p;

	size *= items;
	size = (size + ZALLOC_ALIGNMENT - 1) & ~(ZALLOC_ALIGNMENT - 1);

	p = malloc (size);

	return (p);
}

static void zfree(void *x, void *addr, unsigned nb)
{
	free (addr);
}

#define HEAD_CRC	2
#define EXTRA_FIELD	4
#define ORIG_NAME	8
#define COMMENT		0x10
#define RESERVED	0xe0

#define DEFLATED	8

int gunzip(void *dst, int dstlen, unsigned char *src, unsigned long *lenp)
{
	z_stream s;
	int r, i, flags;

	/* skip header */
	i = 10;
	flags = src[3];
	if (src[2] != DEFLATED || (flags & RESERVED) != 0) {
		puts ("Error: Bad gzipped data\n");
		return (-1);
	}
	if ((flags & EXTRA_FIELD) != 0)
		i = 12 + src[10] + (src[11] << 8);
	if ((flags & ORIG_NAME) != 0)
		while (src[i++] != 0)
			;
	if ((flags & COMMENT) != 0)
		while (src[i++] != 0)
			;
	if ((flags & HEAD_CRC) != 0)
		i += 2;
	if (i >= *lenp) {
		puts ("Error: gunzip out of data in header\n");
		return (-1);
	}

	s.zalloc = zalloc;
	s.zfree = zfree;
#if defined(CONFIG_HW_WATCHDOG) || defined(CONFIG_WATCHDOG)
	s.outcb = (cb_func)WATCHDOG_RESET;
#else
	s.outcb = Z_NULL;
#endif	/* CONFIG_HW_WATCHDOG */

	r = inflateInit2(&s, -MAX_WBITS);
	if (r != Z_OK) {
		printf ("Error: inflateInit2() returned %d\n", r);
		return (-1);
	}
	s.next_in = src + i;
	s.avail_in = *lenp - i;
	s.next_out = dst;
	s.avail_out = dstlen;
	r = inflate(&s, Z_FINISH);
	if (r != Z_OK && r != Z_STREAM_END) {
		printf ("Error: inflate() returned %d\n", r);
		return (-1);
	}
	*lenp = s.next_out - (unsigned char *) dst;
	inflateEnd(&s);

	return (0);
}
#endif

#ifdef CONFIG_BZLIB
void bz_internal_error(int errcode)
{
	printf ("BZIP2 internal error %d\n", errcode);
}
#endif /* CONFIG_BZLIB */

#ifdef CONFIG_RTEMS
static void
do_bootm_rtems (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[],
		ulong addr, ulong *len_ptr, int verify)
{
#if 0
	image_header_t *hdr = &header;
	void	(*entry_point)(bd_t *);

	entry_point = (void (*)(bd_t *)) ntohl(hdr->ih_ep);

	printf ("## Transferring control to RTEMS (at address %08lx) ...\n",
		(ulong)entry_point);
#endif
	SHOW_BOOT_PROGRESS (15);

	/*
	 * RTEMS Parameters:
	 *   r3: ptr to board info data
	 */
#if 0
	(*entry_point ) ( gd->bd );
#else
#warning RTEMS support is broken
#endif
}
#endif

#if (CONFIG_COMMANDS & CFG_CMD_ELF)
static void
do_bootm_vxworks (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[],
		  ulong addr, ulong *len_ptr, int verify)
{
	image_header_t *hdr = &header;
	char str[80];

	sprintf(str, "%x", ntohl(hdr->ih_ep)); /* write entry-point into string */
	setenv("loadaddr", str);
	do_bootvx(cmdtp, 0, 0, NULL);
}

static void
do_bootm_qnxelf (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[],
		 ulong addr, ulong *len_ptr, int verify)
{
	image_header_t *hdr = &header;
	char *local_args[2];
	char str[16];

	sprintf(str, "%x", ntohl(hdr->ih_ep)); /* write entry-point into string */
	local_args[0] = argv[0];
	local_args[1] = str;	/* and provide it via the arguments */
	do_bootelf(cmdtp, 0, 2, local_args);
}
#endif /* CFG_CMD_ELF */

#ifdef CONFIG_LYNXKDI
static void
do_bootm_lynxkdi (cmd_tbl_t *cmdtp, int flag,
		 int	argc, char *argv[],
		 ulong	addr,
		 ulong	*len_ptr,
		 int	verify)
{
	lynxkdi_boot( &header );
}

#endif /* CONFIG_LYNXKDI */
