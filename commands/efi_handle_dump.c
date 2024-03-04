// SPDX-License-Identifier: GPL-2.0
/*
 * efi-device.c - barebox EFI payload support
 *
 * Copyright (c) 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */

#include <common.h>
#include <command.h>
#include <efi.h>
#include <efi/efi-device.h>
#include <efi/efi-mode.h>

static void efi_devpath(efi_handle_t handle)
{
	efi_status_t efiret;
	void *devpath;
	char *dev_path_str;

	efiret = BS->open_protocol(handle, &efi_device_path_protocol_guid,
				   &devpath, NULL, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	if (EFI_ERROR(efiret))
		return;

	dev_path_str = device_path_to_str(devpath);
	if (dev_path_str) {
		printf("  Devpath: \n  %s\n", dev_path_str);
		free(dev_path_str);
	}
}

static void efi_dump(efi_handle_t *handles, unsigned long handle_count)
{
	int i, j;
	unsigned long num_guids;
	efi_guid_t **guids;

	if (!handles || !handle_count)
		return;

	for (i = 0; i < handle_count; i++) {
		printf("handle-%p\n", handles[i]);

		BS->protocols_per_handle(handles[i], &guids, &num_guids);
		printf("  Protocols:\n");
		for (j = 0; j < num_guids; j++)
			printf("  %d: %pUl: %s\n", j, guids[j],
					efi_guid_string(guids[j]));
		efi_devpath(handles[i]);
	}
	printf("\n");
}

static unsigned char to_digit(unsigned char c)
{
	if (c >= '0' && c <= '9')
		c -= '0';
	else if (c >= 'A' && c <= 'F')
		c -= 'A' - 10;
	else
		c -= 'a' - 10;

	return c;
}

#define read_xbit(src, dest, bit) 					\
	do {								\
		int __i;						\
		for (__i = (bit - 4); __i >= 0; __i -= 4, src++)	\
			dest |= to_digit(*src) << __i;			\
	} while (0)

static int do_efi_protocol_dump(int argc, char **argv)
{
	unsigned long handle_count = 0;
	efi_handle_t *handles = NULL;
	int ret;
	efi_guid_t guid;
	u32 a = 0;
	u16 b = 0;
	u16 c = 0;
	u8 d0 = 0;
	u8 d1 = 0;
	u8 d2 = 0;
	u8 d3 = 0;
	u8 d4 = 0;
	u8 d5 = 0;
	u8 d6 = 0;
	u8 d7 = 0;

	/* Format 220e73b6-6bdb-4413-8405-b974b108619a */
	if (argc == 1) {
		char *s = argv[0];
		int len = strlen(s);

		if (len != 36)
			return -EINVAL;

		read_xbit(s, a, 32);
		if (*s != '-')
			return -EINVAL;
		s++;
		read_xbit(s, b, 16);
		if (*s != '-')
			return -EINVAL;
		s++;
		read_xbit(s, c, 16);
		if (*s != '-')
			return -EINVAL;
		s++;
		read_xbit(s, d0, 8);
		read_xbit(s, d1, 8);
		if (*s != '-')
			return -EINVAL;
		s++;
		read_xbit(s, d2, 8);
		read_xbit(s, d3, 8);
		read_xbit(s, d4, 8);
		read_xbit(s, d5, 8);
		read_xbit(s, d6, 8);
		read_xbit(s, d7, 8);
	} else if (argc == 11) {
		/* Format :
		 *	220e73b6 6bdb 4413 84 05 b9 74 b1 08 61 9a
		 *   or
		 *	0x220e73b6 0x6bdb 0x14413 0x84 0x05 0xb9 0x74 0xb1 0x08 0x61 0x9a
		 */
		a = simple_strtoul(argv[0], NULL, 16);
		b = simple_strtoul(argv[1], NULL, 16);
		c = simple_strtoul(argv[2], NULL, 16);
		d0 = simple_strtoul(argv[3], NULL, 16);
		d1 = simple_strtoul(argv[4], NULL, 16);
		d2 = simple_strtoul(argv[5], NULL, 16);
		d3 = simple_strtoul(argv[6], NULL, 16);
		d4 = simple_strtoul(argv[7], NULL, 16);
		d5 = simple_strtoul(argv[8], NULL, 16);
		d6 = simple_strtoul(argv[9], NULL, 16);
		d7 = simple_strtoul(argv[10], NULL, 16);
	} else {
		return -EINVAL;
	}

	guid = EFI_GUID(a, b, c, d0, d1, d2, d3, d4, d5, d6, d7);

	printf("Searching for:\n");
	printf("  %pUl: %s\n", &guid, efi_guid_string(&guid));

	ret = __efi_locate_handle(BS, BY_PROTOCOL, &guid, NULL, &handle_count, &handles);
	if (!ret)
		efi_dump(handles, handle_count);

	return 0;
}

static int do_efi_handle_dump(int argc, char *argv[])
{
	unsigned long handle_count = 0;
	efi_handle_t *handles = NULL;
	int ret;

	if (argc > 1)
		return do_efi_protocol_dump(--argc, ++argv);

	ret = __efi_locate_handle(BS, ALL_HANDLES, NULL, NULL, &handle_count, &handles);
	if (!ret)
		efi_dump(handles, handle_count);

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
