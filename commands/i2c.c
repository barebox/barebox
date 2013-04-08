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

static int do_i2c_probe(int argc, char *argv[])
{
	struct i2c_adapter *adapter;
	struct i2c_client client;
	int startaddr = -1, stopaddr = -1, addr, ret;
	u8 reg;

	if (argc < 4)
		return COMMAND_ERROR_USAGE;

	adapter = i2c_get_adapter(simple_strtoul(argv[1], NULL, 0));
	if (!adapter)
		return -ENODEV;
	client.adapter = adapter;

	startaddr = simple_strtol(argv[2], NULL, 0);
	stopaddr = simple_strtol(argv[3], NULL, 0);
	if ((startaddr == -1) || (stopaddr == -1) || (startaddr > stopaddr))
		return COMMAND_ERROR_USAGE;

	if (stopaddr > 0x7F)
		stopaddr = 0x7F;

	printf("probing i2c range 0x%02x - 0x%02x :\n", startaddr, stopaddr);
	for (addr = startaddr; addr <= stopaddr; addr++) {
		client.addr = addr;
		ret = i2c_write_reg(&client, 0x00, &reg, 0);
		if (ret == 0)
			printf("0x%02x ", addr);
	}
	printf("\n");
	return 0;
}

static const __maybe_unused char cmd_i2c_probe_help[] =
"Usage: i2c_probe bus 0xstartaddr 0xstopaddr\n"
"probe a range of i2c addresses.\n";

BAREBOX_CMD_START(i2c_probe)
	.cmd		= do_i2c_probe,
	.usage		= "probe for an i2c device",
	BAREBOX_CMD_HELP(cmd_i2c_probe_help)
BAREBOX_CMD_END

static int do_i2c_write(int argc, char *argv[])
{
	struct i2c_adapter *adapter = NULL;
	struct i2c_client client;
	int addr = -1, reg = -1, count = -1, verbose = 0, ret, opt, i, bus = 0, wide = 0;
	u8 *buf;

	while ((opt = getopt(argc, argv, "a:b:r:v:w")) > 0) {
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
			wide = 1;
			break;
		}
	}

	count = argc - optind;

	if ((addr < 0) || (reg < 0) || (count == 0) || (addr > 0x7F))
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
		*(buf + i) = (char) simple_strtol(argv[optind+i], NULL, 16);

	ret = i2c_write_reg(&client, reg | (wide ? I2C_ADDR_16_BIT : 0), buf, count);
	if (ret != count)
		goto out;
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

static const __maybe_unused char cmd_i2c_write_help[] =
"Usage: i2c_write [OPTION] ... hexdatas\n"
"write to i2c device.\n"
"  -a 0x<addr>   i2c device address\n"
"  -b <bus_num>  i2c bus number (default = 0)\n"
"  -r 0x<reg>    start register\n"
"  -w            use 16bit-wide address access\n";

BAREBOX_CMD_START(i2c_write)
	.cmd		= do_i2c_write,
	.usage		= "write to an i2c device",
	BAREBOX_CMD_HELP(cmd_i2c_write_help)
BAREBOX_CMD_END

static int do_i2c_read(int argc, char *argv[])
{
	struct i2c_adapter *adapter = NULL;
	struct i2c_client client;
	u8 *buf;
	int count = -1, addr = -1, reg = -1, verbose = 0, ret, opt, bus = 0, wide = 0;

	while ((opt = getopt(argc, argv, "a:b:c:r:v:w")) > 0) {
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
			wide = 1;
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
	ret = i2c_read_reg(&client, reg | (wide ? I2C_ADDR_16_BIT : 0), buf, count);
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

static const __maybe_unused char cmd_i2c_read_help[] =
"Usage: i2c_read [OPTION]\n"
"read i2c device.\n"
"  -a 0x<addr>   i2c device address\n"
"  -b <bus_num>  i2c bus number (default = 0)\n"
"  -r 0x<reg>    start register\n"
"  -w            use 16bit-wide address access\n"
"  -c <count>    byte count\n";

BAREBOX_CMD_START(i2c_read)
	.cmd		= do_i2c_read,
	.usage		= "read from an i2c device",
	BAREBOX_CMD_HELP(cmd_i2c_read_help)
BAREBOX_CMD_END
