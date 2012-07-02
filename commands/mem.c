/*
 * Copyright (c) 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * MA 02111-1307 USA
 */

/*
 * Memory Functions
 *
 * Copied from FADS ROM, Dan Malek (dmalek@jlc.net)
 */

#include <common.h>
#include <command.h>
#include <init.h>
#include <driver.h>
#include <malloc.h>
#include <errno.h>
#include <fs.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/stat.h>
#include <xfuncs.h>

#ifdef	CMD_MEM_DEBUG
#define	PRINTF(fmt,args...)	printf (fmt ,##args)
#else
#define PRINTF(fmt,args...)
#endif

#define RW_BUF_SIZE	4096
static char *rw_buf;

static char *DEVMEM = "/dev/mem";

/* Memory Display
 *
 * Syntax:
 *	md{.b, .w, .l} {addr} {len}
 */
#define DISP_LINE_LEN	16

int memory_display(char *addr, loff_t offs, ulong nbytes, int size)
{
	ulong linebytes, i;
	u_char	*cp;

	/* Print the lines.
	 *
	 * We buffer all read data, so we can make sure data is read only
	 * once, and all accesses are with the specified bus width.
	 */
	do {
		char	linebuf[DISP_LINE_LEN];
		uint	*uip = (uint   *)linebuf;
		ushort	*usp = (ushort *)linebuf;
		u_char	*ucp = (u_char *)linebuf;
		uint	count = 52;

		printf("%08llx:", offs);
		linebytes = (nbytes > DISP_LINE_LEN) ? DISP_LINE_LEN : nbytes;

		for (i = 0; i < linebytes; i += size) {
			if (size == 4) {
				count -= printf(" %08x", (*uip++ = *((uint *)addr)));
			} else if (size == 2) {
				count -= printf(" %04x", (*usp++ = *((ushort *)addr)));
			} else {
				count -= printf(" %02x", (*ucp++ = *((u_char *)addr)));
			}
			addr += size;
			offs += size;
		}

		while(count--)
			putchar(' ');

		cp = (u_char *)linebuf;
		for (i=0; i<linebytes; i++) {
			if ((*cp < 0x20) || (*cp > 0x7e))
				putchar('.');
			else
				printf("%c", *cp);
			cp++;
		}
		putchar('\n');
		nbytes -= linebytes;
		if (ctrlc()) {
			return -EINTR;
		}
	} while (nbytes > 0);

	return 0;
}

static int open_and_lseek(const char *filename, int mode, loff_t pos)
{
	int fd, ret;

	fd = open(filename, mode | O_RDONLY);
	if (fd < 0) {
		perror("open");
		return fd;
	}

	if (!pos)
		return fd;

	ret = lseek(fd, pos, SEEK_SET);
	if (ret == -1) {
		perror("lseek");
		close(fd);
		return -errno;
	}

	return fd;
}

static int mem_parse_options(int argc, char *argv[], char *optstr, int *mode,
		char **sourcefile, char **destfile)
{
	int opt;

	while((opt = getopt(argc, argv, optstr)) > 0) {
		switch(opt) {
		case 'b':
			*mode = O_RWSIZE_1;
			break;
		case 'w':
			*mode = O_RWSIZE_2;
			break;
		case 'l':
			*mode = O_RWSIZE_4;
			break;
		case 's':
			*sourcefile = optarg;
			break;
		case 'd':
			*destfile = optarg;
			break;
		default:
			return -1;
		}
	}

	return 0;
}

static int do_mem_md(int argc, char *argv[])
{
	loff_t	start = 0, size = 0x100;
	int	r, now;
	int	ret = 0;
	int fd;
	char *filename = DEVMEM;
	int mode = O_RWSIZE_4;

	if (mem_parse_options(argc, argv, "bwls:", &mode, &filename, NULL) < 0)
		return 1;

	if (optind < argc) {
		if (parse_area_spec(argv[optind], &start, &size)) {
			printf("could not parse: %s\n", argv[optind]);
			return 1;
		}
		if (size == ~0)
			size = 0x100;
	}

	fd = open_and_lseek(filename, mode | O_RDONLY, start);
	if (fd < 0)
		return 1;

	do {
		now = min(size, (loff_t)RW_BUF_SIZE);
		r = read(fd, rw_buf, now);
		if (r < 0) {
			perror("read");
			goto out;
		}
		if (!r)
			goto out;

		if ((ret = memory_display(rw_buf, start, r, mode >> O_RWSIZE_SHIFT)))
			goto out;

		start += r;
		size  -= r;
	} while (size);

out:
	close(fd);

	return ret ? 1 : 0;
}

static const __maybe_unused char cmd_md_help[] =
"Usage md [OPTIONS] <region>\n"
"display (hexdump) a memory region.\n"
"options:\n"
"  -s <file>   display file (default /dev/mem)\n"
"  -b          output in bytes\n"
"  -w          output in halfwords (16bit)\n"
"  -l          output in words (32bit)\n"
"\n"
"Memory regions:\n"
"Memory regions can be specified in two different forms: start+size\n"
"or start-end, If <start> is omitted it defaults to 0. If end is omitted it\n"
"defaults to the end of the device, except for interactive commands like md\n"
"and mw for which it defaults to 0x100.\n"
"Sizes can be specified as decimal, or if prefixed with 0x as hexadecimal.\n"
"an optional suffix of k, M or G is for kibibytes, Megabytes or Gigabytes,\n"
"respectively\n";


BAREBOX_CMD_START(md)
	.cmd		= do_mem_md,
	.usage		= "memory display",
	BAREBOX_CMD_HELP(cmd_md_help)
BAREBOX_CMD_END

static int do_mem_mw(int argc, char *argv[])
{
	int ret = 0;
	int fd;
	char *filename = DEVMEM;
	int mode = O_RWSIZE_4;
	loff_t adr;

	if (mem_parse_options(argc, argv, "bwld:", &mode, NULL, &filename) < 0)
		return 1;

	if (optind + 1 >= argc)
		return COMMAND_ERROR_USAGE;

	adr = strtoull_suffix(argv[optind++], NULL, 0);

	fd = open_and_lseek(filename, mode | O_WRONLY, adr);
	if (fd < 0)
		return 1;

	while (optind < argc) {
		u8 val8;
		u16 val16;
		u32 val32;
		switch (mode) {
		case O_RWSIZE_1:
			val8 = simple_strtoul(argv[optind], NULL, 0);
			ret = write(fd, &val8, 1);
			break;
		case O_RWSIZE_2:
			val16 = simple_strtoul(argv[optind], NULL, 0);
			ret = write(fd, &val16, 2);
			break;
		case O_RWSIZE_4:
			val32 = simple_strtoul(argv[optind], NULL, 0);
			ret = write(fd, &val32, 4);
			break;
		}
		if (ret < 0) {
			perror("write");
			break;
		}
		ret = 0;
		optind++;
	}

	close(fd);

	return ret ? 1 : 0;
}

static const __maybe_unused char cmd_mw_help[] =
"Usage: mw [OPTIONS] <region> <value(s)>\n"
"Write value(s) to the specifies region.\n"
"options:\n"
"  -b, -w, -l	use byte, halfword, or word accesses\n"
"  -d <file>	write file (default /dev/mem)\n";

BAREBOX_CMD_START(mw)
	.cmd		= do_mem_mw,
	.usage		= "memory write (fill)",
	BAREBOX_CMD_HELP(cmd_mw_help)
BAREBOX_CMD_END

static int do_mem_cmp(int argc, char *argv[])
{
	loff_t	addr1, addr2, count = ~0;
	int	mode  = O_RWSIZE_1;
	char   *sourcefile = DEVMEM;
	char   *destfile = DEVMEM;
	int     sourcefd, destfd;
	char   *rw_buf1;
	int     ret = 1;
	int     offset = 0;
	struct  stat statbuf;

	if (mem_parse_options(argc, argv, "bwls:d:", &mode, &sourcefile, &destfile) < 0)
		return 1;

	if (optind + 2 > argc)
		return COMMAND_ERROR_USAGE;

	addr1 = strtoull_suffix(argv[optind], NULL, 0);
	addr2 = strtoull_suffix(argv[optind + 1], NULL, 0);

	if (optind + 2 == argc) {
		if (sourcefile == DEVMEM) {
			printf("source and count not given\n");
			return 1;
		}
		if (stat(sourcefile, &statbuf)) {
			perror("stat");
			return 1;
		}
		count = statbuf.st_size - addr1;
	} else {
		count = strtoull_suffix(argv[optind + 2], NULL, 0);
	}

	sourcefd = open_and_lseek(sourcefile, mode | O_RDONLY, addr1);
	if (sourcefd < 0)
		return 1;

	destfd = open_and_lseek(destfile, mode | O_RDONLY, addr2);
	if (destfd < 0) {
		close(sourcefd);
		return 1;
	}

	rw_buf1 = xmalloc(RW_BUF_SIZE);

	while (count > 0) {
		int now, r1, r2, i;

		now = min((loff_t)RW_BUF_SIZE, count);

		r1 = read(sourcefd, rw_buf,  now);
		if (r1 < 0) {
			perror("read");
			goto out;
		}

		r2 = read(destfd, rw_buf1, now);
		if (r2 < 0) {
			perror("read");
			goto out;
		}

		if (r1 != now || r2 != now) {
			printf("regions differ in size\n");
			goto out;
		}

		for (i = 0; i < now; i++) {
			if (rw_buf[i] != rw_buf1[i]) {
				printf("files differ at offset %d\n", offset);
				goto out;
			}
			offset++;
		}

		count -= now;
	}

	printf("OK\n");
	ret = 0;
out:
	close(sourcefd);
	close(destfd);
	free(rw_buf1);

	return ret;
}

static const __maybe_unused char cmd_memcmp_help[] =
"Usage: memcmp [OPTIONS] <addr1> <addr2> <count>\n"
"\n"
"options:\n"
"  -b, -w, -l	use byte, halfword, or word accesses\n"
"  -s <file>    source file (default /dev/mem)\n"
"  -d <file>    destination file (default /dev/mem)\n"
"\n"
"Compare memory regions specified with addr1 and addr2\n"
"of size <count> bytes. If source is a file count can\n"
"be left unspecified in which case the whole file is\n"
"compared\n";

BAREBOX_CMD_START(memcmp)
	.cmd		= do_mem_cmp,
	.usage		= "memory compare",
	BAREBOX_CMD_HELP(cmd_memcmp_help)
BAREBOX_CMD_END

static int do_mem_cp(int argc, char *argv[])
{
	loff_t count, dest, src;
	char *sourcefile = DEVMEM;
	char *destfile = DEVMEM;
	int sourcefd, destfd;
	int mode = 0;
	struct stat statbuf;
	int ret = 0;

	if (mem_parse_options(argc, argv, "bwls:d:", &mode, &sourcefile, &destfile) < 0)
		return 1;

	if (optind + 2 > argc)
		return COMMAND_ERROR_USAGE;

	src = strtoull_suffix(argv[optind], NULL, 0);
	dest = strtoull_suffix(argv[optind + 1], NULL, 0);

	if (optind + 2 == argc) {
		if (sourcefile == DEVMEM) {
			printf("source and count not given\n");
			return 1;
		}
		if (stat(sourcefile, &statbuf)) {
			perror("stat");
			return 1;
		}
		count = statbuf.st_size - src;
	} else {
		count = strtoull_suffix(argv[optind + 2], NULL, 0);
	}

	sourcefd = open_and_lseek(sourcefile, mode | O_RDONLY, src);
	if (sourcefd < 0)
		return 1;

	destfd = open_and_lseek(destfile, O_WRONLY | O_CREAT | mode, dest);
	if (destfd < 0) {
		close(sourcefd);
		return 1;
	}

	while (count > 0) {
		int now, r, w, tmp;

		now = min((loff_t)RW_BUF_SIZE, count);

		r = read(sourcefd, rw_buf, now);
		if (r < 0) {
			perror("read");
			goto out;
		}

		if (!r)
			break;

		tmp = 0;
		now = r;
		while (now) {
			w = write(destfd, rw_buf + tmp, now);
			if (w < 0) {
				perror("write");
				goto out;
			}
	                if (!w)
			        break;

			now -= w;
			tmp += w;
		}

		count -= r;

		if (ctrlc())
			goto out;
	}

	if (count) {
		printf("ran out of data\n");
		ret = 1;
	}

out:
	close(sourcefd);
	close(destfd);

	return ret;
}

static const __maybe_unused char cmd_memcpy_help[] =
"Usage: memcpy [OPTIONS] <src> <dst> <count>\n"
"\n"
"options:\n"
"  -b, -w, -l   use byte, halfword, or word accesses\n"
"  -s <file>    source file (default /dev/mem)\n"
"  -d <file>    destination file (default /dev/mem)\n"
"\n"
"Copy memory at <src> of <count> bytes to <dst>\n";

BAREBOX_CMD_START(memcpy)
	.cmd		= do_mem_cp,
	.usage		= "memory copy",
	BAREBOX_CMD_HELP(cmd_memcpy_help)
BAREBOX_CMD_END

static int do_memset(int argc, char *argv[])
{
	loff_t	s, c, n;
	int     fd;
	char   *buf;
	int	mode  = O_RWSIZE_1;
	int     ret = 1;
	char	*file = DEVMEM;

	if (mem_parse_options(argc, argv, "bwld:", &mode, NULL, &file) < 0)
		return 1;

	if (optind + 3 > argc)
		return COMMAND_ERROR_USAGE;

	s = strtoull_suffix(argv[optind], NULL, 0);
	c = strtoull_suffix(argv[optind + 1], NULL, 0);
	n = strtoull_suffix(argv[optind + 2], NULL, 0);

	fd = open_and_lseek(file, mode | O_WRONLY, s);
	if (fd < 0)
		return 1;

	buf = xmalloc(RW_BUF_SIZE);
	memset(buf, c, RW_BUF_SIZE);

	while (n > 0) {
		int now;

		now = min((loff_t)RW_BUF_SIZE, n);

		ret = write(fd, buf, now);
		if (ret < 0) {
			perror("write");
			ret = 1;
			goto out;
		}

		n -= now;
	}

	ret = 0;
out:
	close(fd);
	free(buf);

	return ret;
}

static const __maybe_unused char cmd_memset_help[] =
"Usage: memset [OPTIONS] <src> <c> <n>\n"
"\n"
"options:\n"
"  -b, -w, -l   use byte, halfword, or word accesses\n"
"  -s <file>    source file (default /dev/mem)\n"
"  -d <file>    destination file (default /dev/mem)\n"
"\n"
"Fill the first n bytes of area with byte c\n";

BAREBOX_CMD_START(memset)
	.cmd		= do_memset,
	.usage		= "memory fill",
	BAREBOX_CMD_HELP(cmd_memset_help)
BAREBOX_CMD_END

static struct file_operations memops = {
	.read  = mem_read,
	.write = mem_write,
	.memmap = generic_memmap_rw,
	.lseek = dev_lseek_default,
};

static int mem_probe(struct device_d *dev)
{
	struct cdev *cdev;

	cdev = xzalloc(sizeof (*cdev));
	dev->priv = cdev;

	cdev->name = (char*)dev->resource[0].name;
	cdev->size = (unsigned long)resource_size(&dev->resource[0]);
	cdev->ops = &memops;
	cdev->dev = dev;

	devfs_create(cdev);

	return 0;
}

static struct driver_d mem_drv = {
	.name  = "mem",
	.probe = mem_probe,
};

static int mem_init(void)
{
	rw_buf = malloc(RW_BUF_SIZE);
	if(!rw_buf) {
		printf("%s: Out of memory\n", __FUNCTION__);
		return -1;
	}

	add_mem_device("mem", 0, ~0, IORESOURCE_MEM_WRITEABLE);
	register_driver(&mem_drv);

	return 0;
}

device_initcall(mem_init);
