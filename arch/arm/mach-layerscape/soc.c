// SPDX-License-Identifier: GPL-2.0-only
#include <soc/fsl/scfg.h>
#include <io.h>
#include <linux/bug.h>

static enum scfg_endianess scfg_endianess = SCFG_ENDIANESS_INVALID;

static void scfg_check_endianess(void)
{
	BUG_ON(scfg_endianess == SCFG_ENDIANESS_INVALID);
}

void scfg_clrsetbits32(void __iomem *addr, u32 clear, u32 set)
{
	scfg_check_endianess();

	if (scfg_endianess == SCFG_ENDIANESS_LITTLE)
		clrsetbits_le32(addr, clear, set);
	else
		clrsetbits_be32(addr, clear, set);
}

void scfg_clrbits32(void __iomem *addr, u32 clear)
{
	scfg_check_endianess();

	if (scfg_endianess == SCFG_ENDIANESS_LITTLE)
		clrbits_le32(addr, clear);
	else
		clrbits_be32(addr, clear);
}

void scfg_setbits32(void __iomem *addr, u32 set)
{
	scfg_check_endianess();

	if (scfg_endianess == SCFG_ENDIANESS_LITTLE)
		setbits_le32(addr, set);
	else
		setbits_be32(addr, set);
}

void scfg_out16(void __iomem *addr, u16 val)
{
	scfg_check_endianess();

	if (scfg_endianess == SCFG_ENDIANESS_LITTLE)
		out_le16(addr, val);
	else
		out_be16(addr, val);
}

void scfg_init(enum scfg_endianess endianess)
{
	scfg_endianess = endianess;
}
