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
 * @brief md:  memory display command
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

/* Memory Display
 *
 * Syntax:
 *	md{.b, .w, .l} {addr} {len}
 */
#define DISP_LINE_LEN	16

static int memory_display(char *addr, ulong offs, ulong nbytes, int size)
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

		printf("%08lx:", offs);
		linebytes = (nbytes>DISP_LINE_LEN)?DISP_LINE_LEN:nbytes;

		for (i=0; i<linebytes; i+= size) {
			if (size == 4) {
				printf(" %08x", (*uip++ = *((uint *)addr)));
			} else if (size == 2) {
				printf(" %04x", (*usp++ = *((ushort *)addr)));
			} else {
				printf(" %02x", (*ucp++ = *((u_char *)addr)));
			}
			addr += size;
			offs += size;
		}

		puts ("    ");
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

static int do_mem_md(cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	ulong	start = 0, size = 0x100;
	int	r, now;
	int	ret = 0;
	int	fd;
	char *filename = memory_device;
	int mode = O_RWSIZE_4;

	errno = 0;
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
		now = min(size, RW_BUF_SIZE);
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

	return errno;
}

static __maybe_unused char cmd_md_help[] =
"Usage md [OPTIONS] <region>\n"
"display (hexdump) a memory region.\n"
"options:\n"
"  -s <file>   display file (default /dev/mem)\n"
"  -d <file>   write file (default /dev/mem)\n"
"  -b          output in bytes\n"
"  -w          output in halfwords (16bit)\n"
"  -l          output in words (32bit)\n"
"\n"
"Memory regions:\n"
"Memory regions can be specified in two different forms: start+size\n"
"or start-end, If <start> is ommitted it defaults to 0. If end is ommited it\n"
"defaults to the end of the device, except for interactive commands like md\n"
"and mw for which it defaults to 0x100.\n"
"Sizes can be specified as decimal, or if prefixed with 0x as hexadecimal.\n"
"an optional suffix of k, M or G is for kibibytes, Megabytes or Gigabytes,\n"
"respectively\n";


U_BOOT_CMD_START(md)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_mem_md,
	.usage		= "memory display",
	U_BOOT_CMD_HELP(cmd_md_help)
U_BOOT_CMD_END

/**
@page md_command md: display (hexdump) something

Usage is: md [Options] \<region>

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

\b Examples

Hexdump 100 byes from the fixed physical address 0x400:
@verbatim
uboot:/ md -b 0x400
00000400: 1d 1d 04 1a 53 49 00 e0 01 38 8a 8b d2 0b 02 d1    ....SI...8......
00000410: 00 28 f9 d1 01 e0 00 28 0b d1 6a 20 6b 46 18 70    .(.....(..j kF.p
00000420: 02 f0 ea f8 6b 46 58 70 00 98 01 21 00 f0 f9 f8    ....kFXp...!....
00000430: 20 e0 4d 4d 1e e0 48 4d 1c e0 80 07 80 0f 00 25     .MM..HM.......%
00000440: c6 1c 3b 48 41 6a 04 22 11 43 41 62 81 69 0f 22    ..;HAj.".CAb.i."
00000450: 12 03 91 43 09 22 12 03 89 18 81 61 ff 20 00 f0    ...C.".....a. ..
00000460: 17 fa 01 28 06 d1 30 1c 00 f0 7c fa 01 28 01 d1    ...(..0...|..(..
00000470: 1b 25 ed 06 00 2d 35 d0 02 2c 01 95 05 d0 03 2c    .%...-5..,.....,
00000480: 03 d0 04 2c 01 d0 07 2c 06 d1 01 20 80 02 29 18    ...,...,... ..).
00000490: 04 30 28 18 01 22 05 e0 01 20 00 03 29 18 04 30    .0(.."... ..)..0
000004a0: 28 18 00 22 6b 46 5b 7a 08 2b 10 d0 00 92 02 1c    (.."kF[z.+......
000004b0: 01 23 01 a8 ff f7 4e fe 6b 46 02 90 18 7a f0 28    .#....N.kF...z.(
000004c0: 0d d0 02 98 00 21 00 f0 ac f8 04 e0 0a e0 6b 46    .....!........kF
000004d0: 18 7a f0 28 03 d0 6b 46 58 7a 01 28 02 d1 01 98    .z.(..kFXz.(....
000004e0: 02 f0 4a fb ff f7 17 ff fe bc 08 bc 18 47 38 b5    ..J..........G8.
000004f0: 04 1c 0a 48 0d 1c 40 68 00 90 02 f0 7d f8 08 28    ...H..@h....}..(
@endverbatim

Hexdump words from fixed physical address 0x000 up to 0x00f:
@verbatim
uboot:/ md -w 0x000-0x00f
00000000: f00c e59f f11c e51f f11c e51f f11c e51f    ................
@endverbatim

The examples above are using the generic \c /dev/mem device. But the \c md
command can operate on any device. With the \c -s \c \<device> parameter you
can give a different device to operate on.

To access the flash memory you must not worry about where it is located in
CPU's address space (this would be reqired when the \c /dev/mem is in use).

Instead let \c md operate on the flash device itself. Or on partitions
(devices on top of devices).

Hexdump 100 bytes from the start of the flash memory:
@verbatim
uboot:/ md -b -s /dev/nor0
00000000: 14 00 00 ea 14 f0 9f e5 14 f0 9f e5 14 f0 9f e5    ................
00000010: 14 f0 9f e5 14 f0 9f e5 14 f0 9f e5 14 f0 9f e5    ................
00000020: 00 01 f0 a7 60 01 f0 a7 c0 01 f0 a7 20 02 f0 a7    ....`....... ...
00000030: 80 02 f0 a7 e0 02 f0 a7 40 03 f0 a7 ef be ad de    ........@.......
00000040: 00 00 f0 a7 00 00 f0 a7 00 00 b0 a7 00 20 ae a7    ............. ..
00000050: 30 89 f1 a7 80 d7 f1 a7 00 00 0f e1 1f 00 c0 e3    0...............
00000060: d3 00 80 e3 00 f0 29 e1 00 00 a0 e3 17 0f 07 ee    ......).........
00000070: 17 0f 08 ee 10 0f 11 ee 23 0c c0 e3 87 00 c0 e3    ........#.......
00000080: 02 00 80 e3 01 0a 80 e3 10 0f 01 ee 5c 49 00 eb    ............\I..
00000090: 98 00 4f e2 5c 10 1f e5 01 00 50 e1 07 00 00 0a    ..O.\.....P.....
000000a0: 64 20 1f e5 5c 30 1f e5 02 20 43 e0 02 20 80 e0    d ..\0... C.. ..
000000b0: f8 07 b0 e8 f8 07 a1 e8 02 00 50 e1 fb ff ff da    ..........P.....
000000c0: 80 00 1f e5 0c d0 40 e2 80 00 1f e5 80 10 1f e5    ......@.........
000000d0: 00 20 a0 e3 00 20 80 e5 04 00 80 e2 01 00 50 e1    . ... ........P.
000000e0: fb ff ff da 04 f0 1f e5 54 46 f0 a7 00 00 00 00    ........TF......
000000f0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00    ................
@endverbatim
This command works on the whole flash device.

In our example here the start of the flash contains the U-Boot-v2 code. This
can also be displayed by using the U-Boot partition:

@verbatim
uboot:/ md -b -s /dev/self0
00000000: 14 00 00 ea 14 f0 9f e5 14 f0 9f e5 14 f0 9f e5    ................
00000010: 14 f0 9f e5 14 f0 9f e5 14 f0 9f e5 14 f0 9f e5    ................
00000020: 00 01 f0 a7 60 01 f0 a7 c0 01 f0 a7 20 02 f0 a7    ....`....... ...
00000030: 80 02 f0 a7 e0 02 f0 a7 40 03 f0 a7 ef be ad de    ........@.......
00000040: 00 00 f0 a7 00 00 f0 a7 00 00 b0 a7 00 20 ae a7    ............. ..
00000050: 30 89 f1 a7 80 d7 f1 a7 00 00 0f e1 1f 00 c0 e3    0...............
00000060: d3 00 80 e3 00 f0 29 e1 00 00 a0 e3 17 0f 07 ee    ......).........
00000070: 17 0f 08 ee 10 0f 11 ee 23 0c c0 e3 87 00 c0 e3    ........#.......
00000080: 02 00 80 e3 01 0a 80 e3 10 0f 01 ee 5c 49 00 eb    ............\I..
00000090: 98 00 4f e2 5c 10 1f e5 01 00 50 e1 07 00 00 0a    ..O.\.....P.....
000000a0: 64 20 1f e5 5c 30 1f e5 02 20 43 e0 02 20 80 e0    d ..\0... C.. ..
000000b0: f8 07 b0 e8 f8 07 a1 e8 02 00 50 e1 fb ff ff da    ..........P.....
000000c0: 80 00 1f e5 0c d0 40 e2 80 00 1f e5 80 10 1f e5    ......@.........
000000d0: 00 20 a0 e3 00 20 80 e5 04 00 80 e2 01 00 50 e1    . ... ........P.
000000e0: fb ff ff da 04 f0 1f e5 54 46 f0 a7 00 00 00 00    ........TF......
000000f0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00    ................
@endverbatim

Hexdump the first 100 bytes from the environment partition (also in flash):
@verbatim
uboot:/ md -b -s /dev/env0
00000000: 79 ba 8f 79 00 00 00 00 7f 2e 91 a8 8c 00 00 00    y..y............
00000010: 00 00 00 00 00 00 00 00 4c 36 18 d3 8d c7 a8 67    ........L6.....g
00000020: 71 00 00 00 0a 00 00 00 2f 62 69 6e 2f 69 6e 69    q......./bin/ini
00000030: 74 00 00 00 65 74 68 30 2e 65 74 68 61 64 64 72    t...eth0.ethaddr
00000040: 3d 38 30 3a 38 31 3a 38 32 3a 38 33 3a 38 34 3a    =80:81:82:83:84:
00000050: 38 36 0a 65 74 68 30 2e 73 65 72 76 65 72 69 70    86.eth0.serverip
00000060: 3d 31 39 32 2e 31 36 38 2e 32 33 2e 32 0a 65 74    =192.168.23.2.et
00000070: 68 30 2e 6e 65 74 6d 61 73 6b 3d 32 35 35 2e 32    h0.netmask=255.2
00000080: 35 35 2e 32 35 35 2e 30 0a 65 74 68 30 2e 69 70    55.255.0.eth0.ip
00000090: 61 64 64 72 3d 31 39 32 2e 31 36 38 2e 32 33 2e    addr=192.168.23.
000000a0: 31 39 37 0a 0a 00 00 00 ff ff ff ff ff ff ff ff    197.............
000000b0: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff    ................
000000c0: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff    ................
000000d0: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff    ................
000000e0: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff    ................
000000f0: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff    ................
@endverbatim
*/
