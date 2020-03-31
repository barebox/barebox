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
#include <command.h>
#include <common.h>
#include <i2c/i2c.h>
#include <linux/mfd/rave-sp.h>

static int do_rdu2_switch_reset(void)
{
	struct i2c_client client;
	int ret;
	u8 reg;

	client.adapter = i2c_get_adapter(1);
	if (!client.adapter)
		return -ENODEV;

	/* address of the switch watchdog microcontroller */
	client.addr = 0x38;
	reg = 0x78;
	/* set switch reset time to 100ms */

	ret = i2c_write_reg(&client, 0x0a, &reg, 1);
	if (ret < 0) {
		pr_err("Failed to set switch reset time\n");
		return ret;
	}
	/* reset the switch */
	reg = 0x01;
	ret = i2c_write_reg(&client, 0x0d, &reg, 1);
	if (ret < 0) {
		pr_err("Failed to reset the switch\n");
		return ret;
	}
	/* issue dummy command to work around firmware bug */
	ret = i2c_read_reg(&client, 0x01, &reg, 1);
	if (ret < 0) {
		pr_err("Failed to issue a dummy command\n");
		return ret;
	}

	return 0;
}

static int do_rdu1_switch_reset(void)
{
	struct device_d *sp_dev = get_device_by_name("sp");
	struct rave_sp *sp = sp_dev->parent->priv;
	u8 cmd[] = {
		[0] = RAVE_SP_CMD_RESET_ETH_SWITCH,
		[1] = 0
	};

	if (IS_ENABLED(CONFIG_RAVE_SP_CORE))
		return rave_sp_exec(sp, cmd, sizeof(cmd), NULL, 0);
	else
		return -ENODEV;
}

static int do_rave_switch_reset(int argc, char *argv[])
{
	if (of_machine_is_compatible("zii,imx6q-zii-rdu2") ||
	    of_machine_is_compatible("zii,imx6qp-zii-rdu2"))
		return do_rdu2_switch_reset();

	if (of_machine_is_compatible("zii,imx51-rdu1"))
		return do_rdu1_switch_reset();

	return -ENODEV;
}

BAREBOX_CMD_START(rave_reset_switch)
	.cmd		= do_rave_switch_reset,
	BAREBOX_CMD_DESC("reset ethernet switch on RDU")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
BAREBOX_CMD_END
