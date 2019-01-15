// SPDX-License-Identifier: GPL-2.0-only
/*
 * cpuinfo - show information about MIPS CPU
 *
 * Copyright (C) 2011 Antony Pavlov <antonynpavlov@gmail.com>
 */

#include <common.h>
#include <command.h>
#include <asm/mipsregs.h>
#include <asm/cpu-info.h>

static char *way_string[] = { NULL, "direct mapped", "2-way",
	"3-way", "4-way", "5-way", "6-way", "7-way", "8-way"
};

static int do_cpuinfo(int argc, char *argv[])
{
	unsigned int icache_size, dcache_size, scache_size;
	struct cpuinfo_mips *c = &current_cpu_data;

	printk(KERN_INFO "CPU revision is: %08x (%s)\n",
		current_cpu_data.processor_id, __cpu_name);

	icache_size = c->icache.sets * c->icache.ways * c->icache.linesz;
	dcache_size = c->dcache.sets * c->dcache.ways * c->dcache.linesz;

	printk("Primary instruction cache %ldkB, %s, %s, linesize %d bytes.\n",
	       icache_size >> 10,
	       c->icache.flags & MIPS_CACHE_VTAG ? "VIVT" : "VIPT",
	       way_string[c->icache.ways], c->icache.linesz);

	printk("Primary data cache %ldkB, %s, %s, %s, linesize %d bytes\n",
	       dcache_size >> 10, way_string[c->dcache.ways],
	       (c->dcache.flags & MIPS_CACHE_PINDEX) ? "PIPT" : "VIPT",
	       (c->dcache.flags & MIPS_CACHE_ALIASES) ?
			"cache aliases" : "no aliases",
	       c->dcache.linesz);
	if (c->scache.flags & MIPS_CACHE_NOT_PRESENT)
		return 0;
	scache_size = c->scache.sets * c->scache.ways * c->scache.linesz;
	printk("Secondary data cache %ldkB, %s, %s, %s, linesize %d bytes\n",
	       scache_size >> 10, way_string[c->scache.ways],
	       (c->scache.flags & MIPS_CACHE_PINDEX) ? "PIPT" : "VIPT",
	       (c->scache.flags & MIPS_CACHE_ALIASES) ?
			"cache aliases" : "no aliases",
	       c->scache.linesz);

	return 0;
}

BAREBOX_CMD_START(cpuinfo)
	.cmd            = do_cpuinfo,
	BAREBOX_CMD_DESC("show CPU information")
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
BAREBOX_CMD_END
