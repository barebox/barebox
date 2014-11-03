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
#include <dt-bindings/gpio/tegra-gpio.h>
#include <gpio.h>
#include <i2c/i2c.h>
#include <init.h>

static int nvidia_beaver_fs_init(void)
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

	/* TPS659110: LDO1_REG = 1.05v, ACTIVE to PEX */
	data = 0x15;
	i2c_write_reg(&client, 0x30, &data, 1);

	/* enable SYS_3V3_PEXS */
	gpio_direction_output(TEGRA_GPIO(L, 7), 1);

	return 0;
}
fs_initcall(nvidia_beaver_fs_init);

static int nvidia_beaver_device_init(void)
{
	if (!of_machine_is_compatible("nvidia,beaver"))
		return 0;

	barebox_set_hostname("beaver");

	return 0;
}
device_initcall(nvidia_beaver_device_init);
