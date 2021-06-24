// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Michael Tretter <m.tretter@pengutronix.de>
 */

#include <common.h>
#include <init.h>
#include <linux/types.h>
#include <reset_source.h>

#define ZYNQMP_CRL_APB_BASE		0xff5e0000
#define ZYNQMP_CRL_APB_RESET_REASON	(ZYNQMP_CRL_APB_BASE + 0x220)

/* External POR: The PS_POR_B reset signal pin was asserted. */
#define ZYNQMP_CRL_APB_RESET_REASON_EXTERNAL   BIT(0)
/* Internal POR: A system error triggered a POR reset. */
#define ZYNQMP_CRL_APB_RESET_REASON_INTERNAL   BIT(1)
/* Internal system reset; A system error triggered a system reset. */
#define ZYNQMP_CRL_APB_RESET_REASON_PMU        BIT(2)
/* PS-only reset: Write to PMU_GLOBAL.GLOBAL_RESET [PS_ONLY_RST]. */
#define ZYNQMP_CRL_APB_RESET_REASON_PSONLY     BIT(3)
/* External system reset: The PS_SRST_B reset signal pin was asserted. */
#define ZYNQMP_CRL_APB_RESET_REASON_SRST       BIT(4)
/* Software system reset: Write to RESET_CTRL [soft_reset]. */
#define ZYNQMP_CRL_APB_RESET_REASON_SOFT       BIT(5)
/* Software debugger reset: Write to BLOCKONLY_RST [debug_only]. */
#define ZYNQMP_CRL_APB_RESET_REASON_DEBUG_SYS  BIT(6)

struct zynqmp_reset_reason {
	u32 mask;
	enum reset_src_type type;
};

static const struct zynqmp_reset_reason reset_reasons[] = {
	{ ZYNQMP_CRL_APB_RESET_REASON_DEBUG_SYS,	RESET_JTAG },
	{ ZYNQMP_CRL_APB_RESET_REASON_SOFT,		RESET_RST },
	{ ZYNQMP_CRL_APB_RESET_REASON_SRST,		RESET_POR },
	{ ZYNQMP_CRL_APB_RESET_REASON_PSONLY,		RESET_POR },
	{ ZYNQMP_CRL_APB_RESET_REASON_PMU,		RESET_POR },
	{ ZYNQMP_CRL_APB_RESET_REASON_INTERNAL,		RESET_POR },
	{ ZYNQMP_CRL_APB_RESET_REASON_EXTERNAL,		RESET_POR },
	{ /* sentinel */ }
};

static enum reset_src_type zynqmp_get_reset_src(void)
{
	enum reset_src_type type = RESET_UKWN;
	unsigned int i;
	u32 val;

	val = readl(ZYNQMP_CRL_APB_RESET_REASON);

	for (i = 0; i < ARRAY_SIZE(reset_reasons); i++) {
		if (val & reset_reasons[i].mask) {
			type = reset_reasons[i].type;
			break;
		}
	}

	pr_info("ZynqMP reset reason %s (ZYNQMP_CRL_APB_RESET_REASON: 0x%08x)\n",
		reset_source_to_string(type), val);

	return type;
}

static int zynqmp_init(void)
{
	reset_source_set(zynqmp_get_reset_src());

	return 0;
}
postcore_initcall(zynqmp_init);
