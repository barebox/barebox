/*
 * Copyright (C) 2014 Lucas Stach <l.stach@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <init.h>
#include <i2c/i2c.h>

static int nvidia_beaver_devices_init(void)
{
	struct i2c_client client;
	u8 data;

	if (!of_machine_is_compatible("nvidia,beaver"))
		return 0;

	client.adapter = i2c_get_adapter(4);
	client.addr = 0x2d;

	/* TPS659110: LDO5_REG = 3.3v, ACTIVE to SDMMC1 */
	data = 0x65;
	i2c_write_reg(&client, 0x32, &data, 1);

	return 0;
}
device_initcall(nvidia_beaver_devices_init);
