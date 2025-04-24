// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 Kalray Inc.
 */

#include <common.h>
#include <reset_source.h>
#include <mfd/syscon.h>
#include <restart.h>
#include <linux/regmap.h>
#include <init.h>

#include <asm/ftu.h>

static struct regmap *ftu_regmap;

static void __noreturn kvx_restart_soc(struct restart_handler *rst,
				       unsigned long flags)
{
	regmap_write(ftu_regmap, KVX_FTU_SW_RESET_OFFSET, 0x1);

	/* Not reached */
	hang();
}


static int kvx_reset_init(void)
{
	int ret;
	u32 rst_cause;

	ftu_regmap = syscon_regmap_lookup_by_compatible("kalray,kvx-syscon");
	if (IS_ERR(ftu_regmap))
		return PTR_ERR(ftu_regmap);

	ret = regmap_read(ftu_regmap, KVX_FTU_RESET_CAUSE_OFFSET, &rst_cause);
	if (ret < 0) {
		reset_source_set(RESET_UKWN);
		return ret;
	}

	switch (rst_cause) {
	case KVX_FTU_RESET_CAUSE_CODE_DSU:
		reset_source_set(RESET_JTAG);
		break;
	case KVX_FTU_RESET_CAUSE_CODE_SW:
		reset_source_set(RESET_RST);
		break;
	case KVX_FTU_RESET_CAUSE_CODE_MPPA:
		reset_source_set(RESET_POR);
		break;
	case KVX_FTU_RESET_CAUSE_CODE_PCIE:
		reset_source_set(RESET_EXT);
		break;
	case KVX_FTU_RESET_CAUSE_CODE_WDOG_0:
	case KVX_FTU_RESET_CAUSE_CODE_WDOG_1:
	case KVX_FTU_RESET_CAUSE_CODE_WDOG_2:
	case KVX_FTU_RESET_CAUSE_CODE_WDOG_3:
	case KVX_FTU_RESET_CAUSE_CODE_WDOG_4:
		reset_source_set(RESET_WDG);
		break;
	}

	restart_handler_register_fn("soc", kvx_restart_soc);

	return 0;
}
device_initcall(kvx_reset_init);
