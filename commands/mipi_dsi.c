// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2025 Ahmad Fatoum

#include <command.h>
#include <getopt.h>
#include <console.h>
#include <video/mipi_dsi.h>
#include <video/mipi_display.h>

#define MIPI_DSI_CMD_MAX_LEN	64

struct dcs_read {
	u8 cmd;
	u8 len;
};

/*
 * Panels execute the command byte of a DCS read request like that of
 * a write, so only ever send commands known to be reads.
 */
static const struct dcs_read dcs_reads[] = {
	{ MIPI_DCS_GET_DISPLAY_ID, 3 },
	{ MIPI_DCS_GET_RED_CHANNEL, 1 },
	{ MIPI_DCS_GET_GREEN_CHANNEL, 1 },
	{ MIPI_DCS_GET_BLUE_CHANNEL, 1 },
	{ MIPI_DCS_GET_DISPLAY_STATUS, 4 },
	{ MIPI_DCS_GET_POWER_MODE, 1 },
	{ MIPI_DCS_GET_ADDRESS_MODE, 1 },
	{ MIPI_DCS_GET_DISPLAY_MODE, 1 },
	{ MIPI_DCS_GET_SIGNAL_MODE, 1 },
	{ MIPI_DCS_GET_DIAGNOSTIC_RESULT, 1 },
	{ MIPI_DCS_GET_SCANLINE, 2 },
	{ MIPI_DCS_GET_DISPLAY_BRIGHTNESS, 1 },
	{ MIPI_DCS_GET_CONTROL_DISPLAY, 1 },
	{ MIPI_DCS_GET_CABC_MIN_BRIGHTNESS, 1 },
};

/* display ID, DDB and the nonstandard, but widespread ID1-ID3 */
static const struct dcs_read dcs_ids[] = {
	{ MIPI_DCS_GET_DISPLAY_ID, 3 },
	{ MIPI_DCS_READ_DDB_START, 5 },
	{ 0xda, 1 },
	{ 0xdb, 1 },
	{ 0xdc, 1 },
};

static const struct dcs_read *dcs_read_find(const struct dcs_read *reads,
					    size_t nreads, u8 cmd)
{
	for (int i = 0; i < nreads; i++) {
		if (reads[i].cmd == cmd)
			return &reads[i];
	}

	return NULL;
}

static unsigned int dcs_read_len(u8 cmd)
{
	const struct dcs_read *reg;

	reg = dcs_read_find(dcs_reads, ARRAY_SIZE(dcs_reads), cmd) ?:
	      dcs_read_find(dcs_ids, ARRAY_SIZE(dcs_ids), cmd);

	return reg ? reg->len : 1;
}

/*
 * DCS reads send CMD, generic reads send CMD and up to one more
 * parameter byte in @tx.
 */
static int mipi_dsi_command_show(struct mipi_dsi_device *dsi, bool generic,
				 const u8 *tx, size_t tx_len, size_t len,
				 bool prefix)
{
	u8 buf[MIPI_DSI_CMD_MAX_LEN];
	ssize_t ret;

	ret = mipi_dsi_set_maximum_return_packet_size(dsi, len);
	if (!ret && generic)
		ret = mipi_dsi_generic_read(dsi, tx, tx_len, buf, len);
	else if (!ret)
		ret = mipi_dsi_dcs_read(dsi, tx[0], buf, len);

	if (prefix)
		printf("%02x: ", tx[0]);

	if (ret < 0) {
		printf("read failed: %pe\n", ERR_PTR(ret));
		return ret;
	}

	printf("%*phN\n", (int)ret, buf);

	return 0;
}

static int mipi_dsi_command_show_list(struct mipi_dsi_device *dsi,
				      const struct dcs_read *reads,
				      size_t nreads)
{
	int ret, err = 0;

	for (int i = 0; i < nreads; i++) {
		ret = mipi_dsi_command_show(dsi, false, &reads[i].cmd, 1,
					    reads[i].len, true);
		if (ret == -ENETDOWN)
			return ret;
		if (ret)
			err = ret;
		if (ctrlc())
			return -EINTR;
	}

	return err;
}

static const char * const mipi_dsi_formats[] = {
	[MIPI_DSI_FMT_RGB888] = "RGB888",
	[MIPI_DSI_FMT_RGB666] = "RGB666",
	[MIPI_DSI_FMT_RGB666_PACKED] = "RGB666_PACKED",
	[MIPI_DSI_FMT_RGB565] = "RGB565",
};

static const char * const mipi_dsi_mode_flags[] = {
	"video", "video-burst", "video-sync-pulse", "video-auto-vert",
	"video-hse", "video-no-hfp", "video-no-hbp", "video-no-hsa",
	"vsync-flush", "no-eot-packet", "clock-non-continuous", "lpm",
};

static void mipi_dsi_list(bool verbose)
{
	struct mipi_dsi_device *dsi;
	struct device *dev;

	bus_for_each_device(&mipi_dsi_bus_type, dev) {
		const char *format = "unknown";

		dsi = to_mipi_dsi_device(dev);
		printf("%s%s", dev_name(dev),
		       dsi->attached ? "" : " (not attached)");

		if (dsi->format < ARRAY_SIZE(mipi_dsi_formats))
			format = mipi_dsi_formats[dsi->format];

		if (verbose) {
			printf(": host %s, channel %u, %u lanes, %s, flags",
			       dev_name(dsi->host->dev), dsi->channel,
			       dsi->lanes, format);

			for (int i = 0; i < ARRAY_SIZE(mipi_dsi_mode_flags); i++) {
				if (dsi->mode_flags & BIT(i))
					printf(" %s", mipi_dsi_mode_flags[i]);
			}
		}

		printf("\n");
	}
}

struct mipi_dsi_opts {
	bool write, info, generic;
	int peripheral;	/* 1: turn on, 0: shut down, -1: none */
	unsigned int len;	/* 0: the known length of the register or 1 */
};

static int mipi_dsi_execute(struct mipi_dsi_device *dsi,
			    const struct mipi_dsi_opts *opts,
			    int argc, char *argv[])
{
	u8 buf[MIPI_DSI_CMD_MAX_LEN];
	unsigned int len;
	int ret, i;

	if (opts->peripheral >= 0) {
		if (opts->write || opts->info || opts->generic || argc)
			return COMMAND_ERROR_USAGE;
		if (opts->peripheral)
			return mipi_dsi_turn_on_peripheral(dsi);
		return mipi_dsi_shutdown_peripheral(dsi);
	}

	if (opts->info) {
		if (opts->write || opts->generic || argc)
			return COMMAND_ERROR_USAGE;
		return mipi_dsi_command_show_list(dsi, dcs_ids,
						  ARRAY_SIZE(dcs_ids));
	}

	if (!argc) {
		if (opts->write || opts->generic)
			return COMMAND_ERROR_USAGE;
		return mipi_dsi_command_show_list(dsi, dcs_reads,
						  ARRAY_SIZE(dcs_reads));
	}

	if (argc > sizeof(buf)) {
		printf("can only send up to %zu bytes at once\n", sizeof(buf));
		return -EOVERFLOW;
	}

	for (i = 0; i < argc; i++) {
		ret = kstrtou8(argv[i], 16, &buf[i]);
		if (ret < 0)
			return ret;
	}

	if (opts->write) {
		if (opts->generic)
			ret = mipi_dsi_generic_write(dsi, buf, argc);
		else
			ret = mipi_dsi_dcs_write_buffer(dsi, buf, argc);
		return ret < 0 ? ret : 0;
	}

	if (argc > (opts->generic ? 2 : 1))
		return COMMAND_ERROR_USAGE;

	len = opts->len;
	if (!len)
		len = opts->generic ? 1 : dcs_read_len(buf[0]);

	return mipi_dsi_command_show(dsi, opts->generic, buf, argc, len,
				     false);
}

static int do_mipi_dsi(int argc, char *argv[])
{
	struct mipi_dsi_opts opts = { .peripheral = -1 };
	struct mipi_dsi_device *dsi;
	struct device *dev;
	bool list = false, verbose = false, lpm = false, hs = false;
	unsigned long mode_flags;
	unsigned int channel, vc = UINT_MAX;
	int opt, ret;

	dev = list_first_entry_or_null(&mipi_dsi_bus_type.device_list,
				       struct device, bus_list);

	while ((opt = getopt(argc, argv, "c:d:gHilLn:p:vw")) > 0) {
		switch (opt) {
		case 'c':
			ret = kstrtouint(optarg, 0, &vc);
			if (ret)
				return ret;
			if (vc > 3)
				return -EINVAL;
			break;
		case 'd':
			dev = bus_find_device(&mipi_dsi_bus_type, NULL,
					      optarg, device_match_name);
			break;
		case 'g':
			opts.generic = true;
			break;
		case 'H':
			hs = true;
			break;
		case 'i':
			opts.info = true;
			break;
		case 'l':
			list = true;
			break;
		case 'L':
			lpm = true;
			break;
		case 'n':
			ret = kstrtouint(optarg, 0, &opts.len);
			if (ret)
				return ret;
			if (!opts.len || opts.len > MIPI_DSI_CMD_MAX_LEN)
				return -EINVAL;
			break;
		case 'p':
			if (!strcmp(optarg, "on"))
				opts.peripheral = 1;
			else if (!strcmp(optarg, "off"))
				opts.peripheral = 0;
			else
				return COMMAND_ERROR_USAGE;
			break;
		case 'v':
			verbose = true;
			break;
		case 'w':
			opts.write = true;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (list || verbose) {
		mipi_dsi_list(verbose);
		return 0;
	}

	if (lpm && hs)
		return COMMAND_ERROR_USAGE;

	if (!dev)
		return -ENODEV;

	dsi = to_mipi_dsi_device(dev);
	if (!dsi->attached) {
		printf("%s: not attached to its DSI host\n", dev_name(dev));
		return -ENODEV;
	}

	mode_flags = dsi->mode_flags;
	channel = dsi->channel;
	if (lpm)
		dsi->mode_flags |= MIPI_DSI_MODE_LPM;
	if (hs)
		dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;
	if (vc != UINT_MAX)
		dsi->channel = vc;

	ret = mipi_dsi_execute(dsi, &opts, argc - optind, argv + optind);

	dsi->mode_flags = mode_flags;
	dsi->channel = channel;

	if (ret == -ENETDOWN)
		printf("DSI host is powered down, enable the display first\n");

	return ret;
}

BAREBOX_CMD_HELP_START(mipi_dsi)
BAREBOX_CMD_HELP_TEXT("Without CMD, dump the standard DCS registers.")
BAREBOX_CMD_HELP_TEXT("CMD and DATA bytes are given in hex.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-l\t",  "list all MIPI DSI devices")
BAREBOX_CMD_HELP_OPT ("-v\t",  "verbose output")
BAREBOX_CMD_HELP_OPT ("-d DEVICE",  "select specific device (default is first registered)")
BAREBOX_CMD_HELP_OPT ("-i\t",  "read the panel identification registers")
BAREBOX_CMD_HELP_OPT ("-n LEN\t",  "number of bytes to read (default: known length, else 1)")
BAREBOX_CMD_HELP_OPT ("-w\t",  "issue write command")
BAREBOX_CMD_HELP_OPT ("-g\t",  "send generic packets: CMD and DATA are the payload,")
BAREBOX_CMD_HELP_OPT ("\t",  "reads take up to two bytes")
BAREBOX_CMD_HELP_OPT ("-p on|off",  "send Turn On or Shutdown Peripheral")
BAREBOX_CMD_HELP_OPT ("-c CH\t",  "use virtual channel CH (0-3)")
BAREBOX_CMD_HELP_OPT ("-L\t",  "send in low power mode")
BAREBOX_CMD_HELP_OPT ("-H\t",  "send in high speed mode")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(mipi_dsi)
	.cmd		= do_mipi_dsi,
	BAREBOX_CMD_DESC("interact with MIPI DSI device")
	BAREBOX_CMD_OPTS("[-lvdinwgLH] [-c CH] [-p on|off] [CMD] [DATA...]")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_mipi_dsi_help)
BAREBOX_CMD_END
