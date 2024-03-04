// SPDX-License-Identifier: GPL-2.0
/*
 * efi-device.c - barebox EFI payload support
 *
 * Copyright (c) 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */

#include <common.h>
#include <command.h>
#include <efi.h>
#include <linux/uuid.h>
#include <efi/efi-mode.h>
#include <efi/efi-device.h>

static void efi_devpath(struct efi_boot_services *bs,
			efi_handle_t handle,
			const efi_guid_t *guid,
			const char *desc)
{
	efi_status_t efiret;
	void *devpath;
	char *dev_path_str;

	efiret = bs->open_protocol(handle, guid, &devpath, NULL, NULL,
				   EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	if (EFI_ERROR(efiret))
		return;

	dev_path_str = device_path_to_str(devpath);
	if (dev_path_str) {
		printf("  %s: \n  %s\n", desc, dev_path_str);
		free(dev_path_str);
	}
}

static void efi_dump(struct efi_boot_services *bs, efi_handle_t *handles, unsigned long handle_count)
{
	int i, j;
	unsigned long num_guids;
	efi_guid_t **guids;

	if (!handles || !handle_count)
		return;

	for (i = 0; i < handle_count; i++) {
		printf("handle-%p\n", handles[i]);

		bs->protocols_per_handle(handles[i], &guids, &num_guids);
		printf("  Protocols:\n");
		for (j = 0; j < num_guids; j++)
			printf("  %d: %pUl: %s\n", j, guids[j],
					efi_guid_string(guids[j]));

		efi_devpath(bs, handles[i], &efi_device_path_protocol_guid,
			    "Devpath");
		efi_devpath(bs, handles[i], &efi_loaded_image_device_path_guid,
			    "Image Devpath");
	}
	printf("\n");
}

static int do_efi_protocol_dump(struct efi_boot_services *bs, int argc, char **argv)
{
	unsigned long handle_count = 0;
	efi_handle_t *handles = NULL;
	int ret;
	efi_guid_t guid;

	/* Format 220e73b6-6bdb-4413-8405-b974b108619a */
	if (argc == 1) {
		ret = guid_parse(argv[0], &guid);
	} else if (argc == 11) {
		u32 a;
		u16 b, c;
		u8 d0, d1, d2, d3, d4, d5, d6, d7;
		/* Format :
		 *	220e73b6 6bdb 4413 84 05 b9 74 b1 08 61 9a
		 *   or
		 *	0x220e73b6 0x6bdb 0x14413 0x84 0x05 0xb9 0x74 0xb1 0x08 0x61 0x9a
		 */
		ret =        kstrtou32(argv[0], 16, &a);
		ret = ret ?: kstrtou16(argv[1], 16, &b);
		ret = ret ?: kstrtou16(argv[2], 16, &c);
		ret = ret ?: kstrtou8(argv[3],  16, &d0);
		ret = ret ?: kstrtou8(argv[4],  16, &d1);
		ret = ret ?: kstrtou8(argv[5],  16, &d2);
		ret = ret ?: kstrtou8(argv[6],  16, &d3);
		ret = ret ?: kstrtou8(argv[7],  16, &d4);
		ret = ret ?: kstrtou8(argv[8],  16, &d5);
		ret = ret ?: kstrtou8(argv[9],  16, &d6);
		ret = ret ?: kstrtou8(argv[10], 16, &d7);
		if (!ret)
			guid = EFI_GUID(a, b, c, d0, d1, d2, d3, d4, d5, d6, d7);
	} else {
		ret = -EINVAL;
	}

	if (ret)
		return ret;

	printf("Searching for:\n");
	printf("  %pUl: %s\n", &guid, efi_guid_string(&guid));

	ret = __efi_locate_handle(bs, BY_PROTOCOL, &guid, NULL, &handle_count, &handles);
	if (!ret)
		efi_dump(bs, handles, handle_count);

	return 0;
}

static int do_efi_handle_dump(int argc, char *argv[])
{
	unsigned long handle_count = 0;
	efi_handle_t *handles = NULL;
	struct efi_boot_services *bs;
	int ret;

	bs = efi_get_boot_services();
	if (!bs) {
		printf("EFI not yet initialized\n");
		return COMMAND_ERROR;
	}

	if (argc > 1)
		return do_efi_protocol_dump(bs, --argc, ++argv);

	ret = __efi_locate_handle(bs, ALL_HANDLES, NULL, NULL, &handle_count, &handles);
	if (!ret)
		efi_dump(bs, handles, handle_count);

	return 0;
}

BAREBOX_CMD_HELP_START(efi_handle_dump)
BAREBOX_CMD_HELP_TEXT("Dump all the efi handle with protocol and devpath\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(efi_handle_dump)
	.cmd = do_efi_handle_dump,
	BAREBOX_CMD_DESC("Usage: efi_handle_dump")
	BAREBOX_CMD_OPTS("[a-b-c-d0d1-d3d4d5d6d7] or [a b c d0 d1 d2 d3 d4 d5 d6 d7]")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_efi_handle_dump_help)
BAREBOX_CMD_END
