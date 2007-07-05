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
#include <asm-generic/errno.h>

#ifdef	CMD_MEM_DEBUG
#define	PRINTF(fmt,args...)	printf (fmt ,##args)
#else
#define PRINTF(fmt,args...)
#endif

#define RW_BUF_SIZE	(ulong)4096
static char *rw_buf;

/* Memory Display
 *
 * Syntax:
 *	md{.b, .w, .l} {addr} {len}
 */
#define DISP_LINE_LEN	16

int memory_display(char *addr, ulong offs, ulong nbytes, int size)
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
				putc ('.');
			else
				printf("%c", *cp);
			cp++;
		}
		putc ('\n');
		nbytes -= linebytes;
		if (ctrlc()) {
			return -EINTR;
		}
	} while (nbytes > 0);

	return 0;
}

int do_mem_md ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong	offs, now;
	ulong	nbytes = 0x100;
        struct memarea_info mem;
	int	size, r;
	int	ret = 0;

	if (argc < 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

        if (spec_str_to_info(argv[1], &mem)) {
                printf("-ENOPARSE\n");
                return -ENODEV;
        }

        if (mem.flags & MEMAREA_SIZE_SPECIFIED)
                nbytes = mem.size;
        else
                nbytes = min((ulong)0x100, mem.size);

	if ((size = cmd_get_data_size(argv[0], 4)) < 0)
		return -EINVAL;

	offs = mem.start;

	do {
		now = min(RW_BUF_SIZE, nbytes);
		r = read(mem.device, rw_buf, now, offs, RW_SIZE(size));
		if (r <= 0) {
                        perror("read", r);
			return r;
                }

		if ((ret = memory_display(rw_buf, offs, r, size)))
			return ret;

		if (r < now)
			return 0;

		nbytes -= now;
		offs += now;
	} while (nbytes > 0);

	return 0;
}

int do_mem_mw ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong	addr, writeval, count;

        struct memarea_info mem;
	ulong	size;

	if ((argc < 3) || (argc > 4)) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	/* Check for size specification.
	*/
	if ((size = cmd_get_data_size(argv[0], 4)) < 1)
		return 1;

        if (spec_str_to_info(argv[1], &mem)) {
                printf("-ENOPARSE\n");
                return -1;
        }
        addr = mem.start;

	/* Get the value to write.
	*/
	writeval = simple_strtoul(argv[2], NULL, 16);

	/* Count ? */
	if (argc == 4)
		count = simple_strtoul(argv[3], NULL, 16);
	else
		count = size;

	if (count == size) {
		return write(mem.device, (uchar *)&writeval, count, mem.start, RW_SIZE(size));
	} else {
		printf("write multiple not yet implemented\n");
	}

	return 0;
}

int do_mem_cmp (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong	addr1, addr2, count, ngood;
	int	size;
	int     rcode = 0;

	if (argc != 4) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	/* Check for size specification.
	*/
	if ((size = cmd_get_data_size(argv[0], 4)) < 0)
		return 1;

	addr1 = simple_strtoul(argv[1], NULL, 16);

	addr2 = simple_strtoul(argv[2], NULL, 16);

	count = simple_strtoul(argv[3], NULL, 16);

	ngood = 0;

	while (count-- > 0) {
		if (size == 4) {
			ulong word1 = *(ulong *)addr1;
			ulong word2 = *(ulong *)addr2;
			if (word1 != word2) {
				printf("word at 0x%08lx (0x%08lx) "
					"!= word at 0x%08lx (0x%08lx)\n",
					addr1, word1, addr2, word2);
				rcode = 1;
				break;
			}
		}
		else if (size == 2) {
			ushort hword1 = *(ushort *)addr1;
			ushort hword2 = *(ushort *)addr2;
			if (hword1 != hword2) {
				printf("halfword at 0x%08lx (0x%04x) "
					"!= halfword at 0x%08lx (0x%04x)\n",
					addr1, hword1, addr2, hword2);
				rcode = 1;
				break;
			}
		}
		else {
			u_char byte1 = *(u_char *)addr1;
			u_char byte2 = *(u_char *)addr2;
			if (byte1 != byte2) {
				printf("byte at 0x%08lx (0x%02x) "
					"!= byte at 0x%08lx (0x%02x)\n",
					addr1, byte1, addr2, byte2);
				rcode = 1;
				break;
			}
		}
		ngood++;
		addr1 += size;
		addr2 += size;
	}

	printf("Total of %ld %s%s were the same\n",
		ngood, size == 4 ? "word" : size == 2 ? "halfword" : "byte",
		ngood == 1 ? "" : "s");
	return rcode;
}

int do_mem_cp ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong count, offset, now;
	int ret;
        struct memarea_info dst, src;

	int	size;

	if (argc != 3) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	/* Check for size specification.
	*/
	if ((size = cmd_get_data_size(argv[0], 4)) < 0)
		return 1;

        if (spec_str_to_info(argv[1], &src)) {
                printf("-ENOPARSE\n");
                return -1;
        }

        if (spec_str_to_info(argv[2], &dst)) {
                printf("-ENOPARSE\n");
                return -1;
        }

        if (!src.size || !dst.size)
                count = dst.size | src.size;
        else
                count = min(src.size, dst.size);

	printf("copy from 0x%08x to 0x%08x count %d\n",src.start, dst.start, count);

	offset = 0;
	while (count > 0) {
		now = min(RW_BUF_SIZE, count);

		ret = read(src.device, rw_buf, now, src.start + offset, RW_SIZE(size));
		if (ret <= 0)
			return ret;

		ret = write(dst.device, rw_buf, ret, dst.start + offset, RW_SIZE(size));
		if (ret <= 0)
			return ret;
		if (ret < now)
			return 0;
		offset += now;
		count -= now;
	}

	return 0;
}

#ifndef CONFIG_CRC32_VERIFY

int do_mem_crc (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong addr, length;
	ulong crc;
	ulong *ptr;

	if (argc < 3) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	addr = simple_strtoul (argv[1], NULL, 16);

	length = simple_strtoul (argv[2], NULL, 16);

	crc = crc32 (0, (const uchar *) addr, length);

	printf ("CRC32 for %08lx ... %08lx ==> %08lx\n",
			addr, addr + length - 1, crc);

	if (argc > 3) {
		ptr = (ulong *) simple_strtoul (argv[3], NULL, 16);
		*ptr = crc;
	}

	return 0;
}

#else	/* CONFIG_CRC32_VERIFY */

int do_mem_crc (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong addr, length;
	ulong crc;
	ulong *ptr;
	ulong vcrc;
	int verify;
	int ac;
	char **av;

	if (argc < 3) {
  usage:
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	av = argv + 1;
	ac = argc - 1;
	if (strcmp(*av, "-v") == 0) {
		verify = 1;
		av++;
		ac--;
		if (ac < 3)
			goto usage;
	} else
		verify = 0;

	addr = simple_strtoul(*av++, NULL, 16);
	length = simple_strtoul(*av++, NULL, 16);

	crc = crc32(0, (const uchar *) addr, length);

	if (!verify) {
		printf ("CRC32 for %08lx ... %08lx ==> %08lx\n",
				addr, addr + length - 1, crc);
		if (ac > 2) {
			ptr = (ulong *) simple_strtoul (*av++, NULL, 16);
			*ptr = crc;
		}
	} else {
		vcrc = simple_strtoul(*av++, NULL, 16);
		if (vcrc != crc) {
			printf ("CRC32 for %08lx ... %08lx ==> %08lx != %08lx ** ERROR **\n",
					addr, addr + length - 1, crc, vcrc);
			return 1;
		}
	}

	return 0;

}
#endif	/* CONFIG_CRC32_VERIFY */

static void memcpy_sz(void *_dst, void *_src, ulong count, ulong rwsize)
{
	ulong dst = (ulong)_dst;
	ulong src = (ulong)_src;

	if (!rwsize) {
		memcpy(_dst, _src, count);
		return;
	}

	count /= rwsize;

	while (count-- > 0) {
		switch (rwsize) {
		case 1:
			*((u_char *)dst) = *((u_char *)src);
			break;
		case 2:
			*((ushort *)dst) = *((ushort *)src);
			break;
		case 4:
			*((ulong  *)dst) = *((ulong  *)src);
			break;
		}
		dst += rwsize;
		src += rwsize;
	}
}

ssize_t mem_read(struct device_d *dev, void *buf, size_t count, ulong offset, ulong rwflags)
{
	memcpy_sz(buf, (void *)(dev->map_base + offset), count, rwflags & RW_SIZE_MASK);
	return count;
}

ssize_t mem_write(struct device_d *dev, void *buf, size_t count, ulong offset, ulong rwflags)
{
	memcpy_sz((void *)(dev->map_base + offset), buf, count, rwflags & RW_SIZE_MASK);
	return count;
}

static struct device_d mem_dev = {
        .name  = "mem",
	.id    = "mem",
        .map_base = 0,
        .size   = ~0, /* FIXME: should be 0x100000000, ahem... */
};

static struct driver_d mem_drv = {
        .name  = "mem",
        .probe = dummy_probe,
	.read  = mem_read,
	.write = mem_write,
};

static struct driver_d ram_drv = {
        .name  = "ram",
        .probe = dummy_probe,
	.read  = mem_read,
	.write = mem_write,
	.type  = DEVICE_TYPE_DRAM,
};

static int mem_init(void)
{
	rw_buf = malloc(RW_BUF_SIZE);
	if(!rw_buf) {
		printf("%s: Out of memory\n", __FUNCTION__);
		return -1;
	}

        register_device(&mem_dev);
        register_driver(&mem_drv);
        register_driver(&ram_drv);
        return 0;
}

device_initcall(mem_init);

U_BOOT_CMD(
	md,     3,     0,      do_mem_md,
	"md      - memory display\n",
	"[.b, .w, .l] address [# of objects]\n    - memory display\n"
);

U_BOOT_CMD(
	mw,    4,    0,     do_mem_mw,
	"mw      - memory write (fill)\n",
	"[.b, .w, .l] address value [count]\n    - write memory\n"
);

U_BOOT_CMD(
	cp,    4,    0,    do_mem_cp,
	"cp      - memory copy\n",
	"[.b, .w, .l] source target count\n    - copy memory\n"
);

U_BOOT_CMD(
	cmp,    4,     0,     do_mem_cmp,
	"cmp     - memory compare\n",
	"[.b, .w, .l] addr1 addr2 count\n    - compare memory\n"
);

#ifndef CONFIG_CRC32_VERIFY

U_BOOT_CMD(
	crc32,    4,    0,     do_mem_crc,
	"crc32   - checksum calculation\n",
	"address count [addr]\n    - compute CRC32 checksum [save at addr]\n"
);

#else	/* CONFIG_CRC32_VERIFY */

U_BOOT_CMD(
	crc32,    5,    0,     do_mem_crc,
	"crc32   - checksum calculation\n",
	"address count [addr]\n    - compute CRC32 checksum [save at addr]\n"
	"-v address count crc\n    - verify crc of memory area\n"
);

#endif	/* CONFIG_CRC32_VERIFY */

#ifdef CONFIG_LOOPW
U_BOOT_CMD(
	loopw,    4,    0,    do_mem_loopw,
	"loopw   - infinite write loop on address range\n",
	"[.b, .w, .l] address number_of_objects data_to_write\n"
	"    - loop on a set of addresses\n"
);
#endif /* CONFIG_LOOPW */

#ifdef CONFIG_MX_CYCLIC
U_BOOT_CMD(
	mdc,     4,     0,      do_mem_mdc,
	"mdc     - memory display cyclic\n",
	"[.b, .w, .l] address count delay(ms)\n    - memory display cyclic\n"
);

U_BOOT_CMD(
	mwc,     4,     0,      do_mem_mwc,
	"mwc     - memory write cyclic\n",
	"[.b, .w, .l] address value delay(ms)\n    - memory write cyclic\n"
);
#endif /* CONFIG_MX_CYCLIC */

