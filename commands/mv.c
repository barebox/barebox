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
 * @brief mw: memory write command
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

static int do_mem_mw ( cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	int ret = 0;
	int fd;
	char *filename = memory_device;
	int mode = O_RWSIZE_4;
	ulong adr;

	errno = 0;

	if (mem_parse_options(argc, argv, "bwld:", &mode, NULL, &filename) < 0)
		return 1;

	if (optind + 1 >= argc) {
		u_boot_cmd_usage(cmdtp);
		return 1;
	}

	adr = strtoul_suffix(argv[optind++], NULL, 0);

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
		optind++;
	}

	return errno;
}

static __maybe_unused char cmd_mw_help[] =
"Usage: mw [OPTIONS] <region> <value(s)>\n"
"Write value(s) to the specifies region.\n"
"see 'help md' for supported options.\n";

U_BOOT_CMD_START(mw)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_mem_mw,
	.usage		= "memory write (fill)",
	U_BOOT_CMD_HELP(cmd_mw_help)
U_BOOT_CMD_END

/**
@page mw_command mw: write something somewhere

Usage is: mw [Options] \<region>

Options are:
- -s \<file>   display file (default /dev/mem)
- -d \<file>   write file (default /dev/mem)
- -b          output in bytes
- -w          output in halfwords (16bit)
- -l          output in words (32bit)

Memory regions:
- Memory regions can be specified in two different forms: \b \<start>+\<size>
  or \b \<start>-\<end>
- If \b \<start> is ommitted it defaults to 0.
- If \b \<end> is ommited it defaults to the end of the device, except for
  interactive commands like \c md and \c mw for which it defaults to 0x100.
- Sizes can be specified as decimal, or if prefixed with 0x as hexadecimal.
- an optional suffix of k, M or G is for kibibytes, Megabytes or Gigabytes,
  respectively

For usage examples refer the \c md command.
*/
