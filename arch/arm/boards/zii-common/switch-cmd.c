/*
 * Copyright (C) 2018 Zodiac Inflight Innovation
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
 */
#include <common.h>
#include <command.h>
#include <i2c/i2c.h>

static int do_rave_switch_reset(int argc, char *argv[])
{
	struct i2c_client client;
	u8 reg;

	if (!of_machine_is_compatible("zii,imx6q-zii-rdu2") &&
	    !of_machine_is_compatible("zii,imx6qp-zii-rdu2"))
		return -ENODEV;

	client.adapter = i2c_get_adapter(1);
	if (!client.adapter)
		return -ENODEV;

	/* address of the switch watchdog microcontroller */
	client.addr = 0x38;
	reg = 0x78;
	/* set switch reset time to 100ms */
	i2c_write_reg(&client, 0x0a, &reg, 1);
	/* reset the switch */
	reg = 0x01;
	i2c_write_reg(&client, 0x0d, &reg, 1);
	/* issue dummy command to work around firmware bug */
	i2c_read_reg(&client, 0x01, &reg, 1);

	return 0;
}

BAREBOX_CMD_START(rave_reset_switch)
	.cmd		= do_rave_switch_reset,
	BAREBOX_CMD_DESC("reset ethernet switch on RDU2")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
BAREBOX_CMD_END
