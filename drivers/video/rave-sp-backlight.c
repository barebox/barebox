/*
 * LCD Backlight driver for RAVE SP
 *
 * Copyright (C) 2018 Zodiac Inflight Innovations
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <malloc.h>
#include <init.h>
#include <video/backlight.h>
#include <linux/mfd/rave-sp.h>

#define	RAVE_SP_BACKLIGHT_LCD_EN	BIT(7)

static int rave_sp_backlight_set(struct backlight_device *bd, int brightness)
{
	struct rave_sp *sp = bd->dev.priv;
	u8 cmd[] = {
		[0] = RAVE_SP_CMD_SET_BACKLIGHT,
		[1] = 0,
		[2] = brightness ? RAVE_SP_BACKLIGHT_LCD_EN | brightness : 0,
		[3] = 0,
		[4] = 0,
	};

	return rave_sp_exec(sp, cmd, sizeof(cmd), NULL, 0);
}

static int rave_sp_backlight_probe(struct device_d *dev)
{
	struct backlight_device *bd;
	int ret;

	bd = xzalloc(sizeof(*bd));
	bd->dev.priv = dev->parent->priv;
	bd->dev.parent = dev;
	bd->brightness = bd->brightness_cur = bd->brightness_default = 50;
	bd->brightness_max = 100;
	bd->brightness_set = rave_sp_backlight_set;

	ret = backlight_register(bd);
	if (ret) {
		free(bd);
		return ret;
	}

	return 0;
}

static const struct of_device_id rave_sp_backlight_of_match[] = {
	{ .compatible = "zii,rave-sp-backlight" },
	{}
};

static struct driver_d rave_sp_backlight_driver = {
	.name  = "rave-sp-backlight",
	.probe = rave_sp_backlight_probe,
	.of_compatible = DRV_OF_COMPAT(rave_sp_backlight_of_match),
};
device_platform_driver(rave_sp_backlight_driver);
