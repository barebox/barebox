// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Michael Tretter <m.tretter@pengutronix.de>
 */

#include <common.h>
#include <init.h>
#include <linux/types.h>
#include <bootsource.h>
#include <reset_source.h>

#define ZYNQMP_CRL_APB_BASE		0xff5e0000
#define ZYNQMP_CRL_APB_BOOT_MODE_USER	(ZYNQMP_CRL_APB_BASE + 0x200)
#define ZYNQMP_CRL_APB_RESET_REASON	(ZYNQMP_CRL_APB_BASE + 0x220)

/* PSJTAG interface, PS dedicated pins. */
#define ZYNQMP_CRL_APB_BOOT_MODE_PSJTAG        0x0
/* SPI 24-bit addressing */
#define ZYNQMP_CRL_APB_BOOT_MODE_QSPI24        0x1
/* SPI 32-bit addressing */
#define ZYNQMP_CRL_APB_BOOT_MODE_QSPI32        0x2
/* SD 2.0 card @ controller 0 */
#define ZYNQMP_CRL_APB_BOOT_MODE_SD0           0x3
/* SPI NAND flash */
#define ZYNQMP_CRL_APB_BOOT_MODE_NAND          0x4
/* SD 2.0 card @ controller 1 */
#define ZYNQMP_CRL_APB_BOOT_MODE_SD1           0x5
/* eMMC @ controller 1 */
#define ZYNQMP_CRL_APB_BOOT_MODE_EMMC          0x6
/* USB 2.0 */
#define ZYNQMP_CRL_APB_BOOT_MODE_USB           0x7
/* PJTAG connection 0 option. */
#define ZYNQMP_CRL_APB_BOOT_MODE_PJTAG0        0x8
/* PJTAG connection 1 option. */
#define ZYNQMP_CRL_APB_BOOT_MODE_PJTAG1        0x9
/* SD 3.0 card (level-shifted) @ controller 1 */
#define ZYNQMP_CRL_APB_BOOT_MODE_SD1LS         0xE

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

static void zynqmp_get_bootsource(enum bootsource *src, int *instance)
{
	u32 v;

	if (!src || !instance)
		return;

	v = readl(ZYNQMP_CRL_APB_BOOT_MODE_USER);
	v &= 0x0F;

	/* cf. Table 11-1 "Boot Modes" in UG1085 Zynq UltraScale+ Device TRM */
	switch (v) {
	case ZYNQMP_CRL_APB_BOOT_MODE_PSJTAG:
	case ZYNQMP_CRL_APB_BOOT_MODE_PJTAG0:
	case ZYNQMP_CRL_APB_BOOT_MODE_PJTAG1:
		*src = BOOTSOURCE_JTAG;
		*instance = 0;
		break;

	case ZYNQMP_CRL_APB_BOOT_MODE_QSPI24:
	case ZYNQMP_CRL_APB_BOOT_MODE_QSPI32:
		*src = BOOTSOURCE_SPI;
		*instance = 0;
		break;

	case ZYNQMP_CRL_APB_BOOT_MODE_SD0:
		*src = BOOTSOURCE_MMC;
		*instance = 0;
		break;

	case ZYNQMP_CRL_APB_BOOT_MODE_NAND:
		*src = BOOTSOURCE_SPI_NAND;
		*instance = 0;
		break;

	case ZYNQMP_CRL_APB_BOOT_MODE_SD1:
	case ZYNQMP_CRL_APB_BOOT_MODE_EMMC:
	case ZYNQMP_CRL_APB_BOOT_MODE_SD1LS:
		*src = BOOTSOURCE_MMC;
		*instance = 1;
		break;

	case ZYNQMP_CRL_APB_BOOT_MODE_USB:
		*src = BOOTSOURCE_USB;
		*instance = 0;
		break;

	default:
		*src = BOOTSOURCE_UNKNOWN;
		*instance = BOOTSOURCE_INSTANCE_UNKNOWN;
		break;
	}
}

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
	enum bootsource boot_src;
	int boot_instance;

	zynqmp_get_bootsource(&boot_src, &boot_instance);
	bootsource_set(boot_src);
	bootsource_set_instance(boot_instance);

	reset_source_set(zynqmp_get_reset_src());

	return 0;
}
postcore_initcall(zynqmp_init);
