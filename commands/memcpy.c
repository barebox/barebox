/*
 * (C) Copyright 2000
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

/**
 * @file
 * @brief memcpy: memory copy command
 */

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <errno.h>
#include <fs.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/stat.h>
#include <xfuncs.h>

static int do_mem_cp(cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	ulong count;
	ulong	dest, src;
	char *sourcefile = memory_device;
	char *destfile = memory_device;
	int sourcefd, destfd;
	int mode = O_RWSIZE_1;
	struct stat statbuf;
	int ret = 0;

	if (mem_parse_options(argc, argv, "bwls:d:", &mode, &sourcefile, &destfile) < 0)
		return 1;

	if (optind + 2 > argc) {
		u_boot_cmd_usage(cmdtp);
		return 1;
	}

	src = simple_strtoul(argv[optind], NULL, 0);
	dest = simple_strtoul(argv[optind + 1], NULL, 0);

	if (optind + 2 == argc) {
		if (sourcefile == memory_device) {
			printf("source and count not given\n");
			return 1;
		}
		if (stat(sourcefile, &statbuf)) {
			perror("stat");
			return 1;
		}
		count = statbuf.st_size - src;
	} else {
		count = simple_strtoul(argv[optind + 2], NULL, 0);
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
		int now, r, w;

		now = min(RW_BUF_SIZE, count);

		if ((r = read(sourcefd, rw_buf, now)) < 0) {
			perror("read");
			goto out;
		}

		if ((w = write(destfd, rw_buf, r)) < 0) {
			perror("write");
			goto out;
		}

		if (r < now)
			break;

		if (w < r)
			break;

		count -= now;
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

static __maybe_unused char cmd_memcpy_help[] =
"Usage: memcpy [OPTIONS] <src> <dst> <count>\n"
"\n"
"options:\n"
"  -b, -w, -l   use byte, halfword, or word accesses\n"
"  -s <file>    source file (default /dev/mem)\n"
"  -d <file>    destination file (default /dev/mem)\n"
"\n"
"Copy memory at <src> of <count> bytes to <dst>\n";

U_BOOT_CMD_START(memcpy)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_mem_cp,
	.usage		= "memory copy",
	U_BOOT_CMD_HELP(cmd_memcpy_help)
U_BOOT_CMD_END

/**
@page mcpy_command memcpy: Copy something to something

Usage is: memcpy [OPTIONS] \<src> \<dst> \<count>

Options are:
- -s \<file>   source file (default \c /dev/mem)
- -d \<file>   destination file (default \c /dev/mem)
- -b          accesses in bytes
- -w          accesses in halfwords (16bit)
- -l          accesses in words (32bit)

Copy memory at \<src> of \<count> bytes to \<dst>.

*/
