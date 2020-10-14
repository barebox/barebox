/*
 * boot_order.c - configure omap warm boot
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <command.h>
#include <complete.h>
#include <mach/omap4-silicon.h>

struct bootsrc {
	const char *name;
	uint32_t sar;
};

const struct bootsrc src_list[] = {
	{"xip"     , OMAP44XX_SAR_BOOT_XIP     },
	{"xipwait" , OMAP44XX_SAR_BOOT_XIPWAIT },
	{"nand"    , OMAP44XX_SAR_BOOT_NAND    },
	{"onenand" , OMAP44XX_SAR_BOOT_ONENAND },
	{"mmc1"    , OMAP44XX_SAR_BOOT_MMC1    },
	{"mmc2_1"  , OMAP44XX_SAR_BOOT_MMC2_1  },
	{"mmc2_2"  , OMAP44XX_SAR_BOOT_MMC2_2  },
	{"uart"    , OMAP44XX_SAR_BOOT_UART    },
	{"usb_1"   , OMAP44XX_SAR_BOOT_USB_1   },
	{"usb_ulpi", OMAP44XX_SAR_BOOT_USB_ULPI},
	{"usb_2"   , OMAP44XX_SAR_BOOT_USB_2   },
};

static uint32_t parse_device(char *str)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(src_list); i++) {
		if (strcmp(str, src_list[i].name) == 0)
			return src_list[i].sar;
	}
	printf("Unknown device '%s'\n", str);
	return OMAP44XX_SAR_BOOT_VOID;
}

static int cmd_boot_order(int argc, char *argv[])
{
	uint32_t device_list[] = {
		OMAP44XX_SAR_BOOT_VOID,
		OMAP44XX_SAR_BOOT_VOID,
		OMAP44XX_SAR_BOOT_VOID,
		OMAP44XX_SAR_BOOT_VOID,
	};
	int i;

	if (argc < 2)
		return COMMAND_ERROR_USAGE;
	for (i = 0; i + 1 < argc && i < ARRAY_SIZE(device_list); i++) {
		device_list[i] = parse_device(argv[i + 1]);
		if (device_list[i] == OMAP44XX_SAR_BOOT_VOID)
			return COMMAND_ERROR_USAGE;
	}
	omap4_set_warmboot_order(device_list);
	return 0;
}

BAREBOX_CMD_HELP_START(boot_order)
BAREBOX_CMD_HELP_TEXT("Set OMAP warm boot order of up to four devices. Each device can be one of:")
BAREBOX_CMD_HELP_TEXT("xip xipwait nand onenand mmc1 mmc2_1 mmc2_2 uart usb_1 usb_ulpi usb_2")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(boot_order)
	.cmd		= cmd_boot_order,
	BAREBOX_CMD_DESC("set OMAP warm boot order")
	BAREBOX_CMD_OPTS("DEVICE...")
	BAREBOX_CMD_GROUP(CMD_GRP_BOOT)
	BAREBOX_CMD_HELP(cmd_boot_order_help)
	BAREBOX_CMD_COMPLETE(empty_complete)
BAREBOX_CMD_END
