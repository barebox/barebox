// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020-2022 Intel Corporation <www.intel.com>
 *
 */

#include <common.h>
#include <io.h>
#include <errno.h>
#include <mach/socfpga/agilex5-clk.h>
#include <mach/socfpga/soc64-handoff.h>

enum endianness {
	LITTLE_ENDIAN = 0,
	BIG_ENDIAN,
	UNKNOWN_ENDIANNESS
};

static enum endianness check_endianness(u32 handoff)
{
	switch (handoff) {
	case SOC64_HANDOFF_MAGIC_BOOT:
	case SOC64_HANDOFF_MAGIC_MUX:
	case SOC64_HANDOFF_MAGIC_IOCTL:
	case SOC64_HANDOFF_MAGIC_FPGA:
	case SOC64_HANDOFF_MAGIC_DELAY:
	case SOC64_HANDOFF_MAGIC_PERI:
	case SOC64_HANDOFF_MAGIC_SDRAM:
	case SOC64_HANDOFF_MAGIC_CLOCK:
		return BIG_ENDIAN;
	default:
		return UNKNOWN_ENDIANNESS;
	}
}

static int getting_endianness(void __iomem *handoff_address, enum endianness *endian_t)
{
	/* Checking handoff data is little endian ? */
	*endian_t = check_endianness(readl(handoff_address));

	if (*endian_t == UNKNOWN_ENDIANNESS) {
		/* Trying to check handoff data is big endian? */
		*endian_t = check_endianness(swab32(readl(handoff_address)));
		if (*endian_t == UNKNOWN_ENDIANNESS)
			return -EPERM;
	}

	return 0;
}

const struct cm_config * const cm_get_default_config(void)
{
        struct cm_config *cm_handoff_cfg = (struct cm_config *)
                (SOC64_HANDOFF_CLOCK + SOC64_HANDOFF_OFFSET_DATA);
        u32 *conversion = (u32 *)cm_handoff_cfg;
        u32 handoff_clk = readl(SOC64_HANDOFF_CLOCK);
        u32 i;

        if (swab32(handoff_clk) == SOC64_HANDOFF_MAGIC_CLOCK) {
                writel(swab32(handoff_clk), SOC64_HANDOFF_CLOCK);
                for (i = 0; i < (sizeof(*cm_handoff_cfg) / sizeof(u32)); i++)
                        conversion[i] = swab32(conversion[i]);
                return cm_handoff_cfg;
        } else if (handoff_clk == SOC64_HANDOFF_MAGIC_CLOCK) {
                return cm_handoff_cfg;
        }

        return NULL;
}

int socfpga_get_handoff_size(void __iomem *handoff_address)
{
	u32 size;
	int ret;
	enum endianness endian_t;

	ret = getting_endianness(handoff_address, &endian_t);
	if (ret)
		return ret;

	size = readl(handoff_address + SOC64_HANDOFF_OFFSET_LENGTH);
	if (endian_t == BIG_ENDIAN)
		size = swab32(size);

	size = (size - SOC64_HANDOFF_OFFSET_DATA) / sizeof(u32);

	return size;
}

int socfpga_handoff_read(void __iomem *handoff_address, void *table, u32 table_len)
{
	u32 temp;
	u32 *table_x32 = table;
	u32 i = 0;
	int ret;
	enum endianness endian_t;

	ret = getting_endianness(handoff_address, &endian_t);
	if (ret)
		return ret;

	temp = readl(handoff_address + SOC64_HANDOFF_OFFSET_DATA +
		    (i * sizeof(u32)));

	if (endian_t == BIG_ENDIAN)
		*table_x32 = swab32(temp);
	else if (endian_t == LITTLE_ENDIAN)
		*table_x32 = temp;

	for (i = 1; i < table_len; i++) {
		table_x32++;

		temp = readl(handoff_address +
			     SOC64_HANDOFF_OFFSET_DATA +
			     (i * sizeof(u32)));

		if (endian_t == BIG_ENDIAN)
			*table_x32 = swab32(temp);
		else if (endian_t == LITTLE_ENDIAN)
			*table_x32 = temp;
	}

	return 0;
}
