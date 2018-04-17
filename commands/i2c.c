/*
 * i2c.c - i2c commands
 *
 * Copyright (c) 2010 Eric Bénard <eric@eukrea.Com>, Eukréa Electromatique
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <command.h>
#include <errno.h>
#include <malloc.h>
#include <getopt.h>
#include <i2c/i2c.h>

static void i2c_probe_range(struct i2c_adapter *adapter, int startaddr, int stopaddr)
{
	struct i2c_client client = {};
	int addr;
	int ret;
	u8 reg;

	client.adapter = adapter;

	printf("probing i2c%d range 0x%02x-0x%02x: ", adapter->nr, startaddr, stopaddr);
	for (addr = startaddr; addr <= stopaddr && !ctrlc(); addr++) {
		client.addr = addr;
		ret = i2c_write_reg(&client, 0x00, &reg, 0);
		if (ret == 0)
			printf("0x%02x ", addr);
	}
	printf("\n");
}

static int do_i2c_probe(int argc, char *argv[])
{
	struct i2c_adapter *adapter = NULL;
	int startaddr = 0, stopaddr = 0x7f;

	if (argc > 1) {
		adapter = i2c_get_adapter(simple_strtoul(argv[1], NULL, 0));
		if (!adapter)
			return -ENODEV;
	}

	if (argc > 2)
		startaddr = simple_strtol(argv[2], NULL, 0);
	if (argc > 3)
		stopaddr = simple_strtol(argv[3], NULL, 0);


	if (startaddr > stopaddr)
		return COMMAND_ERROR_USAGE;

	if (stopaddr > 0x7F)
		stopaddr = 0x7F;

	if (adapter) {
		i2c_probe_range(adapter, startaddr, stopaddr);
	} else {
		for_each_i2c_adapter(adapter)
			i2c_probe_range(adapter, startaddr, stopaddr);
	}

	return 0;
}

BAREBOX_CMD_HELP_START(i2c_probe)
BAREBOX_CMD_HELP_TEXT("Probe the i2c bus BUS, address range from START to END for devices.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(i2c_probe)
	.cmd		= do_i2c_probe,
	BAREBOX_CMD_DESC("probe for an i2c device")
	BAREBOX_CMD_OPTS("BUS START END")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_i2c_probe_help)
BAREBOX_CMD_END

static int do_i2c_write(int argc, char *argv[])
{
	struct i2c_adapter *adapter = NULL;
	struct i2c_client client;
	int addr = -1, reg = -1, count = -1, verbose = 0, ret, opt, i, bus = 0, wide = 0;
	u8 *buf;

	while ((opt = getopt(argc, argv, "a:b:r:vw")) > 0) {
		switch (opt) {
		case 'a':
			addr = simple_strtol(optarg, NULL, 0);
			break;
		case 'r':
			reg = simple_strtol(optarg, NULL, 0);
			break;
		case 'b':
			bus = simple_strtoul(optarg, NULL, 0);
			break;
		case 'v':
			verbose = 1;
			break;
		case 'w':
			wide = I2C_ADDR_16_BIT;
			break;
		}
	}

	count = argc - optind;

	if ((addr < 0) || (count == 0) || (addr > 0x7F))
		return COMMAND_ERROR_USAGE;

	adapter = i2c_get_adapter(bus);
	if (!adapter) {
		printf("i2c bus %d not found\n", bus);
		return -ENODEV;
	}

	client.adapter = adapter;
	client.addr = addr;

	buf = xmalloc(count);
	for (i = 0; i < count; i++)
		*(buf + i) = (char) simple_strtol(argv[optind+i], NULL, 0);

	if (reg >= 0) {
		ret = i2c_write_reg(&client, reg | wide, buf, count);
	} else {
		ret = i2c_master_send(&client, buf, count);
	}
	if (ret != count) {
		if (verbose)
			printf("write aborted, count(%i) != writestatus(%i)\n",
				count, ret);
		goto out;
	}
	ret = 0;

	if (verbose) {
		printf("wrote %i bytes starting at reg 0x%04x to i2cdev 0x%02x on bus %i\n",
			count, reg, addr, adapter->nr);
		for (i = 0; i < count; i++)
			printf("0x%02x ", *(buf + i));
		printf("\n");
	}

out:
	free(buf);
	return ret;
}

BAREBOX_CMD_HELP_START(i2c_write)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-b BUS\t", "i2c bus number (default 0)")
BAREBOX_CMD_HELP_OPT ("-a ADDR\t", "i2c device address")
BAREBOX_CMD_HELP_OPT ("-r START", "start register")
BAREBOX_CMD_HELP_OPT ("-w\t",       "use word (16 bit) wide access")
BAREBOX_CMD_HELP_OPT ("-v\t",       "verbose")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(i2c_write)
	.cmd		= do_i2c_write,
	BAREBOX_CMD_DESC("write to an i2c device")
	BAREBOX_CMD_OPTS("[-barwv] DATA...")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_i2c_write_help)
BAREBOX_CMD_END

static int do_i2c_read(int argc, char *argv[])
{
	struct i2c_adapter *adapter = NULL;
	struct i2c_client client;
	u8 *buf;
	int count = -1, addr = -1, reg = -1, verbose = 0, ret, opt, bus = 0, wide = 0;

	while ((opt = getopt(argc, argv, "a:b:c:r:vw")) > 0) {
		switch (opt) {
		case 'a':
			addr = simple_strtol(optarg, NULL, 0);
			break;
		case 'c':
			count = simple_strtoul(optarg, NULL, 0);
			break;
		case 'b':
			bus = simple_strtoul(optarg, NULL, 0);
			break;
		case 'r':
			reg = simple_strtol(optarg, NULL, 0);
			break;
		case 'v':
			verbose = 1;
			break;
		case 'w':
			wide = I2C_ADDR_16_BIT;
			break;
		}
	}

	if ((addr < 0) || (reg < 0) || (count < 1) || (addr > 0x7F))
		return COMMAND_ERROR_USAGE;

	adapter = i2c_get_adapter(bus);
	if (!adapter) {
		printf("i2c bus %d not found\n", bus);
		return -ENODEV;
	}

	client.adapter = adapter;
	client.addr = addr;

	buf = xmalloc(count);
	ret = i2c_read_reg(&client, reg | wide, buf, count);
	if (ret == count) {
		int i;
		if (verbose)
			printf("read %i bytes starting at reg 0x%04x from i2cdev 0x%02x on bus %i\n",
				count, reg, addr, adapter->nr);
		for (i = 0; i < count; i++)
			printf("0x%02x ", *(buf + i));
		printf("\n");
		ret = 0;
	}

	free(buf);
	return ret;
}

BAREBOX_CMD_HELP_START(i2c_read)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-b BUS\t", "i2c bus number (default 0)")
BAREBOX_CMD_HELP_OPT("-a ADDR\t", "i2c device address")
BAREBOX_CMD_HELP_OPT("-r START", "start register")
BAREBOX_CMD_HELP_OPT("-w\t",       "use word (16 bit) wide access")
BAREBOX_CMD_HELP_OPT("-c COUNT", "byte count")
BAREBOX_CMD_HELP_OPT("-v\t",       "verbose")
BAREBOX_CMD_HELP_END


BAREBOX_CMD_START(i2c_read)
	.cmd		= do_i2c_read,
	BAREBOX_CMD_DESC("read from an i2c device")
	BAREBOX_CMD_OPTS("[-bacrwv]")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_i2c_read_help)
BAREBOX_CMD_END
