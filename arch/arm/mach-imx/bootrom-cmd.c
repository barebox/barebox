/* SPDX-License-Identifier: GPL-2.0-only */

#include <command.h>
#include <errno.h>
#include <getopt.h>
#include <printk.h>
#include <linux/bitops.h>
#include <linux/bitfield.h>
#include <mach/imx/imx8m-regs.h>
#include <mach/imx/romapi.h>
#include <linux/kernel.h>

/* i.MX7 and later ID field is swapped compared to i.MX6 */
#define ROM_EVENT_FORMAT_V0_RES	GENMASK(31, 24)
#define ROM_EVENT_FORMAT_V0_ID	GENMASK(23, 0)
#define ROM_EVENT_FORMAT_V1_ID	GENMASK(31, 24)
#define		ROM_EVENT_FORMAT_V1_ID_TYPE	GENMASK(31, 28)
#define		ROM_EVENT_FORMAT_V1_ID_IDX	GENMASK(27, 24)
#define ROM_EVENT_FORMAT_V1_RES	GENMASK(23, 0)

static const char *lookup(const char *table[], size_t table_size, size_t idx)
{
	const char *str = NULL;

	if (idx < table_size)
		str = table[idx];

	return str ?: "unknown";
}

#define LOOKUP(table, idx) lookup(table, ARRAY_SIZE(table), idx)

static const char *boot_mode_0x1y[] = {
	"Fuse", "Serial Download", "Internal Download", "Test Mode"
};

static const char *secure_config_0x2y[] = {
	"FAB", "Field Return", "Open", "Closed"
};

static const char *boot_image_0x5y[] = {
	"primary", "secondary"
};

static const char *boot_device_0x6y[] = {
	"RAW NAND", "SD or EMMC", NULL, NULL, "ECSPI NOR", NULL, NULL, "QSPI NOR"
};

/* Parse the ROM event ID defintion version 1 log, see AN12853 */
static int imx8m_bootrom_decode_log(const u32 *rom_log)
{
	int i;

	if (!rom_log)
		return -ENODATA;

	for (i = 0; i < 128; i++) {
		u8 event_id = FIELD_GET(ROM_EVENT_FORMAT_V1_ID, rom_log[i]);
		u8 event_id_idx = FIELD_GET(ROM_EVENT_FORMAT_V1_ID_IDX, rom_log[i]);
		const char *arg = NULL;

		printf("[%02x] ", event_id);
		switch (event_id) {
		case 0x0:
			printf("End of list\n");
			return 0;
		case 0x01:
			printf("ROM event version 0x%02x\n", rom_log[i] & 0xFF);
			continue;

		case 0x10 ... 0x13:
			printf("Boot mode is Boot from %s\n",
			       LOOKUP(boot_mode_0x1y, event_id_idx));
			continue;

		case 0x20 ... 0x23:
			printf("Secure config is %s\n",
			       LOOKUP(secure_config_0x2y, event_id_idx));
			continue;

		case 0x30 ... 0x31:
		case 0xe0:
			printf("Internal use\n");
			continue;

		case 0x40 ... 0x41:
			printf("FUSE_SEL_VALUE Fuse is %sblown\n",
			       event_id_idx ? "" : "not ");
			continue;

		case 0x50 ... 0x51:
			printf("Boot from the %s boot image\n",
			       LOOKUP(boot_image_0x5y, event_id_idx));
			continue;

		case 0x74:
			arg = "SPI NAND";
			fallthrough;
		case 0x60 ... 0x67:
			printf("Primary boot from %s device\n",
			       arg ?: LOOKUP(boot_device_0x6y, event_id_idx));
			continue;

		case 0x71:
			printf("Recovery boot from ECSPI NOR device\n");
			continue;
		case 0x72:
			printf("No Recovery boot device\n");
			continue;
		case 0x73:
			printf("Manufacture boot from SD or EMMC\n");
			continue;

		case 0x80:
			printf("Start to perform the device initialization: @%u ticks\n",
			       rom_log[++i]);
			continue;
		case 0x81:
			printf("The boot device initialization completes: @%u ticks\n",
			       rom_log[++i]);
			continue;
		case 0x82:
			printf("Start to execute boot device driver pre-config @%u ticks\n",
			       rom_log[++i]);
			continue;
		case 0x83:
			printf("Boot device driver pre-config completes\n");
			continue;
		case 0x8E:
			printf("Boot device driver pre-config fails\n");
			continue;
		case 0x8f:
			printf("boot device initialization fails: @%u ticks\n",
			       rom_log[++i]);
			continue;

		case 0x90:
			printf("Start to read data from boot device: @ offset %08x\n",
			       rom_log[++i]);
			continue;
		case 0x91:
			printf("Reading data from boot device completes: @%u ticks\n",
			       rom_log[++i]);
			continue;
		case 0x9f:
			printf("Reading data from boot device fails: @%u ticks\n",
			       rom_log[++i]);
			continue;

		case 0xa0:
			printf("Image authentication result: %s(0x%08x) @%u ticks\n",
			       (rom_log[i+1] & 0xFF) == 0xF0 ? "PASS " : "",
			       rom_log[i+1], rom_log[i+2]);
			i += 2;
			continue;
		case 0xa1:
			printf("IVT header is not valid\n");
			continue;

		case 0xc0:
			printf("Jump to the boot image soon: @ offset 0x%08x @ %u ticks\n",
			       rom_log[i+1], rom_log[i+2]);
			i += 2;
			continue;

		case 0xd0:
			printf("Enters serial download processing\n");
			continue;

		case 0xf0:
			printf("Enters ROM exception handler\n");
			continue;
		default:
			printf("Unknown\n");
			continue;
		}
	}

	return -EILSEQ;
}

static int do_bootrom(int argc, char *argv[])
{
	union {
		const u32 *ptr;
		ulong addr;
	} rom_log = { NULL };
	bool log = false;
	int ret, opt;

	while((opt = getopt(argc, argv, "la:")) > 0) {
		switch(opt) {
		case 'a':
			ret = kstrtoul(optarg, 0, &rom_log.addr);
			if (ret)
				return ret;
		case 'l':
			log = true;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (!rom_log.addr)
		rom_log.ptr = imx8m_get_bootrom_log();

	if (log)
		return imx8m_bootrom_decode_log(rom_log.ptr);

	return COMMAND_ERROR_USAGE;
}

BAREBOX_CMD_HELP_START(bootrom)
BAREBOX_CMD_HELP_TEXT("List information about the specified files or directories.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-l",  "list event log")
BAREBOX_CMD_HELP_OPT ("-a ADDR",  "event log address (default PBL scratch space)")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(bootrom)
	.cmd            = do_bootrom,
	BAREBOX_CMD_DESC("Interact with BootROM on i.MX8M")
	BAREBOX_CMD_OPTS("[-la]")
	BAREBOX_CMD_HELP(cmd_bootrom_help)
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
BAREBOX_CMD_END
