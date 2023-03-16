// SPDX-License-Identifier: GPL-2.0+
// SPDX-FileCopyrightText: (C) Copyright 2023 Ametek Inc.
// SPDX-FileCopyrightText: 2023 Renaud Barbier <renaud.barbier@ametek.com>,

#include <common.h>
#include <init.h>
#include <bbu.h>
#include <net.h>
#include <crc.h>
#include <fs.h>
#include <io.h>
#include <envfs.h>
#include <libfile.h>
#include <asm/memory.h>
#include <linux/sizes.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <asm/system.h>
#include <mach/layerscape/layerscape.h>
#include <of_address.h>
#include <soc/fsl/immap_lsch2.h>

static int iot_mem_init(void)
{
	if (!of_machine_is_compatible("fsl,ls1021a"))
		return 0;

	arm_add_mem_device("ram0", 0x80000000, 0x40000000);

	return 0;
}
mem_initcall(iot_mem_init);

static int iot_postcore_init(void)
{
	struct ls102xa_ccsr_scfg *scfg = IOMEM(LSCH2_SCFG_ADDR);

	if (!of_machine_is_compatible("fsl,ls1021a"))
		return 0;

	/* clear BD & FR bits for BE BD's and frame data */
	clrbits_be32(&scfg->etsecdmamcr, SCFG_ETSECDMAMCR_LE_BD_FR);
	out_be32(&scfg->etsecmcr, SCFG_ETSECCMCR_GE2_CLK125);

	return 0;
}
coredevice_initcall(iot_postcore_init);
