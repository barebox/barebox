/*
 * Copyright (c) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2 only
 *
 */

#include <common.h>
#include <command.h>
#include <errno.h>
#include <malloc.h>
#include <getopt.h>
#include <spi/spi.h>

static int do_spi(int argc, char *argv[])
{
	struct spi_device spi;
	int bus = 0;
	int read = 0;
	int verbose = 0;
	int opt, count, i, ret;
	int byte_per_word;

	u8 *tx_buf, *rx_buf;

	memset(&spi, 0, sizeof(struct spi_device));

	spi.max_speed_hz = 1 * 1000 * 1000;
	spi.bits_per_word = 8;

	while ((opt = getopt(argc, argv, "b:c:r:m:f:w:v")) > 0) {
		switch (opt) {
		case 'b':
			bus = simple_strtol(optarg, NULL, 0);
			break;
		case 'r':
			read = simple_strtol(optarg, NULL, 0);
			break;
		case 'c':
			spi.chip_select = simple_strtoul(optarg, NULL, 0);
			break;
		case 'm':
			spi.mode = simple_strtoul(optarg, NULL, 0);
			break;
		case 'f':
			spi.max_speed_hz = simple_strtoul(optarg, NULL, 0);
			break;
		case 'w':
			spi.bits_per_word = simple_strtoul(optarg, NULL, 0);
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	count = argc - optind;

	if ((!read && !count) || !spi.max_speed_hz || !spi.bits_per_word)
		return COMMAND_ERROR_USAGE;


	spi.master = spi_get_master(bus);
	if (!spi.master) {
		printf("spi bus %d not found\n", bus);
		return -ENODEV;
	}

	if (spi.chip_select >= spi.master->num_chipselect) {
		printf("spi chip select (%d) >= master num chipselect (%d)\n",
			spi.chip_select, spi.master->num_chipselect);
		return -EINVAL;
	}

	ret = spi.master->setup(&spi);
	if (ret) {
		printf("can not setup the master (%d)\n", ret);
		return ret;
	}

	tx_buf = xmalloc(count);
	rx_buf = xmalloc(read);

	for (i = 0; i < count; i++)
		tx_buf[i] = (u8) simple_strtol(argv[optind + i], NULL, 0);

	ret = spi_write_then_read(&spi, tx_buf, count, rx_buf, read);
	if (ret)
		goto out;

	byte_per_word = max(spi.bits_per_word / 8, 1);
	if (verbose) {
		printf("device config\n");
		printf("    bus_num       = %d\n", spi.master->bus_num);
		printf("    max_speed_hz  = %d\n", spi.max_speed_hz);
		printf("    chip_select   = %d\n", spi.chip_select);
		printf("    mode          = 0x%x\n", spi.mode);
		printf("    bits_per_word = %d\n", spi.bits_per_word);
		printf("\n");

		printf("wrote %i bytes\n", count);
		memory_display(tx_buf, 0, count, byte_per_word, 0);

		printf("read %i bytes\n", read);
	}

	memory_display(rx_buf, 0, read, byte_per_word, 0);

out:
	free(rx_buf);
	free(tx_buf);
	return ret;
}


BAREBOX_CMD_HELP_START(spi)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-b BUS\t",  "SPI bus number (default 0)")
BAREBOX_CMD_HELP_OPT ("-r COUNT",  "bytes to read")
BAREBOX_CMD_HELP_OPT ("-c\t",      "chip select (default 0)")
BAREBOX_CMD_HELP_OPT ("-m MODE\t", "SPI mode (default 0)")
BAREBOX_CMD_HELP_OPT ("-f HZ\t",   "max speed frequency, in Hz (default 1 MHz)")
BAREBOX_CMD_HELP_OPT ("-w BIT\t",  "bits per word (default 8)")
BAREBOX_CMD_HELP_OPT ("-v\t",      "verbose")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(spi)
	.cmd		= do_spi,
	BAREBOX_CMD_DESC("write/read from SPI device")
	BAREBOX_CMD_OPTS("[-brcmfwv] DATA...")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_spi_help)
BAREBOX_CMD_END
