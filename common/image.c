/*
 * (C) Copyright 2008 Semihalf
 *
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

#ifdef __BAREBOX__
#include <common.h>
#include <image.h>
#include <rtc.h>
#else
#include <time.h>
#endif

#if defined(CONFIG_CMD_BOOTM_SHOW_TYPE) || !defined(__BAREBOX__)
typedef struct table_entry {
	int	id;		/* as defined in image.h	*/
	char	*sname;		/* short (input) name		*/
	char	*lname;		/* long (output) name		*/
} table_entry_t;

static table_entry_t arch_name[] = {
	{ IH_ARCH_INVALID,	NULL,		"Invalid CPU",	},
	{ IH_ARCH_ALPHA,	"alpha",	"Alpha",	},
	{ IH_ARCH_ARM,		"arm",		"ARM",		},
	{ IH_ARCH_I386,		"x86",		"Intel x86",	},
	{ IH_ARCH_IA64,		"ia64",		"IA64",		},
	{ IH_ARCH_M68K,		"m68k",		"MC68000",	},
	{ IH_ARCH_MICROBLAZE,	"microblaze",	"MicroBlaze",	},
	{ IH_ARCH_MIPS,		"mips",		"MIPS",		},
	{ IH_ARCH_MIPS64,	"mips64",	"MIPS 64 Bit",	},
	{ IH_ARCH_NIOS,		"nios",		"NIOS",		},
	{ IH_ARCH_NIOS2,	"nios2",	"NIOS II",	},
	{ IH_ARCH_PPC,		"ppc",		"PowerPC",	},
	{ IH_ARCH_S390,		"s390",		"IBM S390",	},
	{ IH_ARCH_SH,		"sh",		"SuperH",	},
	{ IH_ARCH_SPARC,	"sparc",	"SPARC",	},
	{ IH_ARCH_SPARC64,	"sparc64",	"SPARC 64 Bit",	},
	{ IH_ARCH_BLACKFIN,	"blackfin",	"Blackfin",	},
	{ IH_ARCH_AVR32,	"avr32",	"AVR32",	},
	{ -1,			"",		"",		},
};

static table_entry_t os_name[] = {
	{ IH_OS_INVALID,	NULL,		"Invalid OS",	},
#ifndef __BAREBOX__
	{ IH_OS_4_4BSD,		"4_4bsd",	"4_4BSD",	},
	{ IH_OS_ARTOS,		"artos",	"ARTOS",	},
	{ IH_OS_DELL,		"dell",		"Dell",		},
	{ IH_OS_ESIX,		"esix",		"Esix",		},
	{ IH_OS_FREEBSD,	"freebsd",	"FreeBSD",	},
	{ IH_OS_IRIX,		"irix",		"Irix",		},
#endif
	{ IH_OS_LINUX,		"linux",	"Linux",	},
#ifndef __BAREBOX__
	{ IH_OS_LYNXOS,		"lynxos",	"LynxOS",	},
	{ IH_OS_NCR,		"ncr",		"NCR",		},
	{ IH_OS_NETBSD,		"netbsd",	"NetBSD",	},
	{ IH_OS_OPENBSD,	"openbsd",	"OpenBSD",	},
	{ IH_OS_PSOS,		"psos",		"pSOS",		},
	{ IH_OS_QNX,		"qnx",		"QNX",		},
	{ IH_OS_RTEMS,		"rtems",	"RTEMS",	},
	{ IH_OS_SCO,		"sco",		"SCO",		},
	{ IH_OS_SOLARIS,	"solaris",	"Solaris",	},
	{ IH_OS_SVR4,		"svr4",		"SVR4",		},
#endif
	{ IH_OS_BAREBOX,	"barebox",	"barebox",	},
#ifndef __BAREBOX__
	{ IH_OS_VXWORKS,	"vxworks",	"VxWorks",	},
#endif
	{ -1,			"",		"",		},
};

static table_entry_t type_name[] = {
	{ IH_TYPE_INVALID,	NULL,		"Invalid Image",	},
	{ IH_TYPE_FILESYSTEM,	"filesystem",	"Filesystem Image",	},
	{ IH_TYPE_FIRMWARE,	"firmware",	"Firmware",		},
	{ IH_TYPE_KERNEL,	"kernel",	"Kernel Image",		},
	{ IH_TYPE_MULTI,	"multi",	"Multi-File Image",	},
	{ IH_TYPE_RAMDISK,	"ramdisk",	"RAMDisk Image",	},
	{ IH_TYPE_SCRIPT,	 "script",	"Script",		},
	{ IH_TYPE_STANDALONE,	"standalone",	"Standalone Program",	},
	{ IH_TYPE_FLATDT,	"flat_dt",	"Flat Device Tree",	},
	{ -1,			"",		"",			},
};

static table_entry_t comp_name[] = {
	{ IH_COMP_NONE,		"none",		"uncompressed",		},
	{ IH_COMP_BZIP2,	"bzip2",	"bzip2 compressed",	},
	{ IH_COMP_GZIP,		"gzip",		"gzip compressed",	},
	{ -1,			"",		"",			},
};

static char *get_table_entry_name(table_entry_t *table, char *msg, int id)
{
	for (; table->id >= 0; ++table) {
		if (table->id == id)
			return table->lname;
	}

	return msg;
}

const char *image_get_os_name(uint8_t os)
{
	return get_table_entry_name(os_name, "Unknown OS", os);
}

const char *image_get_arch_name(uint8_t arch)
{
	return get_table_entry_name(arch_name, "Unknown Architecture", arch);
}

const char *image_get_type_name(uint8_t type)
{
	return get_table_entry_name(type_name, "Unknown Image", type);
}

const char *image_get_comp_name(uint8_t comp)
{
	return get_table_entry_name(comp_name, "Unknown Compression", comp);
}
#endif

/**
 * image_multi_count - get component (sub-image) count
 * @hdr: pointer to the header of the multi component image
 *
 * image_multi_count() returns number of components in a multi
 * component image.
 *
 * Note: no checking of the image type is done, caller must pass
 * a valid multi component image.
 *
 * returns:
 *     number of components
 */
ulong image_multi_count(const image_header_t *hdr)
{
	ulong i, count = 0;
	uint32_t *size;

	/* get start of the image payload, which in case of multi
	 * component images that points to a table of component sizes */
	size = (uint32_t *)image_get_data (hdr);

	/* count non empty slots */
	for (i = 0; size[i]; ++i)
		count++;

	return count;
}

/**
 * image_multi_getimg - get component data address and size
 * @hdr: pointer to the header of the multi component image
 * @idx: index of the requested component
 * @data: pointer to a ulong variable, will hold component data address
 * @len: pointer to a ulong variable, will hold component size
 *
 * image_multi_getimg() returns size and data address for the requested
 * component in a multi component image.
 *
 * Note: no checking of the image type is done, caller must pass
 * a valid multi component image.
 *
 * returns:
 *     data address and size of the component, if idx is valid
 *     0 in data and len, if idx is out of range
 */
void image_multi_getimg(const image_header_t *hdr, ulong idx,
			ulong *data, ulong *len)
{
	int i;
	uint32_t *size;
	ulong offset, count, img_data;

	/* get number of component */
	count = image_multi_count(hdr);

	/* get start of the image payload, which in case of multi
	 * component images that points to a table of component sizes */
	size = (uint32_t *)image_get_data(hdr);

	/* get address of the proper component data start, which means
	 * skipping sizes table (add 1 for last, null entry) */
	img_data = image_get_data(hdr) + (count + 1) * sizeof (uint32_t);

	if (idx < count) {
		*len = uimage_to_cpu(size[idx]);
		offset = 0;

		/* go over all indices preceding requested component idx */
		for (i = 0; i < idx; i++) {
			/* add up i-th component size, rounding up to 4 bytes */
			offset += (uimage_to_cpu(size[i]) + 3) & ~3 ;
		}

		/* calculate idx-th component data address */
		*data = img_data + offset;
	} else {
		*len = 0;
		*data = 0;
	}
}

static void image_print_type(const image_header_t *hdr)
{
	const char *os, *arch, *type, *comp;

	os = image_get_os_name(image_get_os(hdr));
	arch = image_get_arch_name(image_get_arch(hdr));
	type = image_get_type_name(image_get_type(hdr));
	comp = image_get_comp_name(image_get_comp(hdr));

	printf ("%s %s %s (%s)\n", arch, os, type, comp);
}

#if defined(CONFIG_TIMESTAMP) || defined(CONFIG_CMD_DATE) || !defined(__BAREBOX__)
static void image_print_time(time_t timestamp)
{
#if defined(__BAREBOX__)
	struct rtc_time tm;

	to_tm(timestamp, &tm);
	printf("%4d-%02d-%02d  %2d:%02d:%02d UTC\n",
			tm.tm_year, tm.tm_mon, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec);
#else
	printf("%s", ctime(&timestamp));
#endif
}
#endif /* CONFIG_TIMESTAMP || CONFIG_CMD_DATE || !__BAREBOX__ */

void image_print_size(uint32_t size)
{
#ifdef __BAREBOX__
	printf("%d Bytes = %s\n", size, size_human_readable(size));
#else
	printf("%d Bytes = %.2f kB = %.2f MB\n",
			size, (double)size / 1.024e3,
			(double)size / 1.048576e6);
#endif
}

void image_print_contents(const void *ptr)
{
	const image_header_t *hdr = (const image_header_t *)ptr;
	const char *p;
	int type;

#ifdef __BAREBOX__
	p = "   ";
#else
	p = "";
#endif

	printf("%sImage Name:   %.*s\n", p, IH_NMLEN, image_get_name(hdr));
#if defined(CONFIG_TIMESTAMP) || defined(CONFIG_CMD_DATE) || !defined(__BAREBOX__)
	printf("%sCreated:      ", p);
	image_print_time((time_t)image_get_time(hdr));
#endif
	printf ("%sImage Type:   ", p);
	image_print_type(hdr);
	printf ("%sData Size:    ", p);
	image_print_size(image_get_data_size(hdr));
	printf ("%sLoad Address: %08x\n", p, image_get_load(hdr));
	printf ("%sEntry Point:  %08x\n", p, image_get_ep(hdr));

	type = image_get_type(hdr);
	if (type == IH_TYPE_MULTI || type == IH_TYPE_SCRIPT) {
		int i;
		ulong data, len;
		ulong count = image_multi_count(hdr);

		printf ("%sContents:\n", p);
		for (i = 0; i < count; i++) {
			image_multi_getimg(hdr, i, &data, &len);

			printf("%s   Image %d: ", p, i);
			image_print_size(len);

			if (image_get_type(hdr) != IH_TYPE_SCRIPT && i > 0) {
				/*
				 * the user may need to know offsets
				 * if planning to do something with
				 * multiple files
				 */
				printf("%s    Offset = 0x%08lx\n", p, data);
			}
		}
	}
}
