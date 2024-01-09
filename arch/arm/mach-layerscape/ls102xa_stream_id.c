// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2014 Freescale Semiconductor
 */

#include <common.h>
#include <asm/io.h>
#include <soc/fsl/immap_lsch2.h>
#include <mach/layerscape/layerscape.h>

struct smmu_stream_id {
	uint16_t offset;
	uint16_t stream_id;
	char dev_name[32];
};

static struct smmu_stream_id dev_stream_id[] = {
	{ 0x100, 0x01, "ETSEC MAC1" },
	{ 0x104, 0x02, "ETSEC MAC2" },
	{ 0x108, 0x03, "ETSEC MAC3" },
	{ 0x10c, 0x04, "PEX1" },
	{ 0x110, 0x05, "PEX2" },
	{ 0x114, 0x06, "qDMA" },
	{ 0x118, 0x07, "SATA" },
	{ 0x11c, 0x08, "USB3" },
	{ 0x120, 0x09, "QE" },
	{ 0x124, 0x0a, "eSDHC" },
	{ 0x128, 0x0b, "eMA" },
	{ 0x14c, 0x0c, "2D-ACE" },
	{ 0x150, 0x0d, "USB2" },
	{ 0x18c, 0x0e, "DEBUG" },
};

static void
ls102xa_config_smmu_stream_id(struct smmu_stream_id *id, uint32_t num)
{
	void *scfg = (void *)LSCH2_SCFG_ADDR;
	int i;
	u32 icid;

	for (i = 0; i < num; i++) {
		icid = (id[i].stream_id & 0xff) << 24;
		out_be32((u32 *)(scfg + id[i].offset), icid);
	}
}

void ls102xa_smmu_stream_id_init(void)
{
	ls102xa_config_smmu_stream_id(dev_stream_id, ARRAY_SIZE(dev_stream_id));
}
