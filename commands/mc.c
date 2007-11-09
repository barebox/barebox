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
 * @brief memcmp: memory compare command
 */

#include <common.h>
#include <command.h>
#include <fcntl.h>
#include <linux/stat.h> /* FIXME */
#include <fs.h>
#include <errno.h>
#include <xfuncs.h>	/* for xmalloc */
#include <malloc.h>	/* for free */
#include <getopt.h>

static int do_mem_cmp(cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	ulong	addr1, addr2, count = ~0;
	int	mode  = O_RWSIZE_1;
	char   *sourcefile = memory_device;
	char   *destfile = memory_device;
	int     sourcefd, destfd;
	char   *rw_buf1;
	int     ret = 1;
	int     offset = 0;
	struct  stat statbuf;

	if (mem_parse_options(argc, argv, "bwls:d:", &mode, &sourcefile, &destfile) < 0)
		return 1;

	if (optind + 2 > argc) {
		u_boot_cmd_usage(cmdtp);
		return 1;
	}

	addr1 = simple_strtoul(argv[optind], NULL, 0);
	addr2 = simple_strtoul(argv[optind + 1], NULL, 0);

	if (optind + 2 == argc) {
		if (sourcefile == memory_device) {
			printf("source and count not given\n");
			return 1;
		}
		if (stat(sourcefile, &statbuf)) {
			perror("stat");
			return 1;
		}
		count = statbuf.st_size - addr1;
	} else {
		count = simple_strtoul(argv[optind + 2], NULL, 0);
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

		now = min(RW_BUF_SIZE, count);

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

static __maybe_unused char cmd_memcmp_help[] =
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

U_BOOT_CMD_START(memcmp)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_mem_cmp,
	.usage		= "memory compare",
	U_BOOT_CMD_HELP(cmd_memcmp_help)
U_BOOT_CMD_END

/**
@page mc_command memcmp: compare something with something other

Usage is: memcmp [OPTIONS] \<addr1> \<addr2> \<count>

Options are:
- -s \<file>   source file (default /dev/mem)
- -d \<file>   destination file (default /dev/mem)
- -b          access in bytes
- -w          access in halfwords (16bit)
- -l          access in words (32bit)

Compare memory regions specified with \<addr1> and \<addr2> of size \<count>
bytes. If source is a file \<count> can be left unspecified in which case the
whole file is compared.

@note The access type might be important in some physical I/O areas.

Usage examples:

Compare two 64kiB memory areas in CPU's physical address range:
@verbatim
uboot:/ memcmp -b 0x400 0xc0000000 0x10000
@endverbatim

Compare a file content with a physical memory area content (to check if
programming the flash memory was successfully):
@verbatim
uboot:/ memcmp -b -s /my_file.bin -d /dev/mem 0x0 0xc0000000
@endverbatim

Compare two files (also to check if programming the flash memory was
successfully):
@verbatim
uboot:/ memcmp -b -s /my_file.bin -d /dev/nor0.partition1
@endverbatim
@note In this case \c my_file.bin could be smaller than the \c nor0.partition1.
\c memcmp stops to compare when the end of the shorter file ends.
*/
