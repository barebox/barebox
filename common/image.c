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
 */

#ifdef __BAREBOX__
#include <common.h>
#include <image.h>
#include <rtc.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <fs.h>
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
	{ IH_ARCH_NDS32,	"nds32",	"NDS32",	},
	{ IH_ARCH_OPENRISC,	"or1k",		"OpenRISC 1000",},
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
