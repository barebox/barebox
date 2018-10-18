/*
 * Copyright (C) 2013 Jan Luebbe <jlu@pengutronix.de>
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
 *
 */

#include <common.h>
#include <driver.h>
#include <xfuncs.h>
#include <errno.h>
#include <watchdog.h>
#include <reset_source.h>

#include <i2c/i2c.h>
#include <restart.h>

#define DRIVERNAME		"da9053"

/* STATUS REGISTERS */
#define DA9053_STATUS_A_REG		1
#define DA9053_STATUS_B_REG		2
#define DA9053_STATUS_C_REG		3
#define DA9053_STATUS_D_REG		4

/* PARK REGISTER */
#define DA9053_PARK_REGISTER		DA9053_STATUS_D_REG

/* EVENT REGISTERS */
#define DA9053_EVENT_A_REG		5
#define DA9053_EVENT_B_REG		6
#define DA9053_EVENT_C_REG		7
#define DA9053_EVENT_D_REG		8
#define DA9053_FAULTLOG_REG		9

/* IRQ REGISTERS */
#define DA9053_IRQ_MASK_A_REG		10
#define DA9053_IRQ_MASK_B_REG		11
#define DA9053_IRQ_MASK_C_REG		12
#define DA9053_IRQ_MASK_D_REG		13

/* CONTROL REGISTERS */
#define DA9053_CONTROL_A_REG		14
#define DA9053_CONTROL_B_REG		15
#define DA9053_CONTROL_C_REG		16
#define DA9053_CONTROL_D_REG		17

#define DA9053_PDDIS_REG		18
#define DA9053_INTERFACE_REG		19
#define DA9053_RESET_REG		20

/* FAULT LOG REGISTER BITS */
#define DA9053_FAULTLOG_WAITSET		0X80
#define DA9053_FAULTLOG_NSDSET		0X40
#define DA9053_FAULTLOG_KEYSHUT		0X20
#define DA9053_FAULTLOG_TEMPOVER	0X08
#define DA9053_FAULTLOG_VDDSTART	0X04
#define DA9053_FAULTLOG_VDDFAULT	0X02
#define DA9053_FAULTLOG_TWDERROR	0X01

/* CONTROL REGISTER B BITS */
#define DA9053_CONTROLB_SHUTDOWN	0X80
#define DA9053_CONTROLB_DEEPSLEEP	0X40
#define DA9053_CONTROLB_WRITEMODE	0X20
#define DA9053_CONTROLB_BBATEN		0X10
#define DA9053_CONTROLB_OTPREADEN	0X08
#define DA9053_CONTROLB_AUTOBOOT	0X04
#define DA9053_CONTROLB_ACTDIODE	0X02
#define DA9053_CONTROLB_BUCKMERGE	0X01

/* CONTROL REGISTER D BITS */
#define DA9053_CONTROLD_WATCHDOG	0X80
#define DA9053_CONTROLD_ACCDETEN	0X40
#define DA9053_CONTROLD_GPI1415SD	0X20
#define DA9053_CONTROLD_NONKEYSD	0X10
#define DA9053_CONTROLD_KEEPACTEN	0X08
#define DA9053_CONTROLD_TWDSCALE	0X07

struct da9053_priv {
	struct watchdog		wd;
	struct i2c_client	*client;
	struct device_d		*dev;
	struct restart_handler	restart;
};

#define wd_to_da9053_priv(x)	container_of(x, struct da9053_priv, wd)

static int da9053_reg_read(struct da9053_priv *da9053, u32 reg, u8 *val)
{
	int ret;

	ret = i2c_read_reg(da9053->client, reg, val, 1);

	return ret == 1 ? 0 : ret;
}

static int da9053_reg_write(struct da9053_priv *da9053, u32 reg, u8 val)
{
	int ret;

	ret = i2c_write_reg(da9053->client, reg, &val, 1);

	return ret == 1 ? 0 : ret;
}

static int da9053_park(struct da9053_priv *da9053)
{
	int ret;
	u8 val;

	ret = i2c_read_reg(da9053->client, DA9053_PARK_REGISTER, &val, 1);

	return ret == 1 ? 0 : ret;
}

static int da9053_enable_multiwrite(struct da9053_priv *da9053)
{
	int ret;
	u8 val;

	ret = da9053_reg_read(da9053, DA9053_CONTROL_B_REG, &val);
	if (ret < 0)
		return ret;

	if (val & DA9053_CONTROLB_WRITEMODE) {
		val &= ~DA9053_CONTROLB_WRITEMODE;
		ret = da9053_reg_write(da9053, DA9053_CONTROL_B_REG, val);
		if (ret < 0)
			return ret;
	}

	ret = da9053_park(da9053);
	if (ret < 0)
		return ret;

	return 0;
}

static int da9053_set_timeout(struct watchdog *wd, unsigned timeout)
{
	struct da9053_priv *da9053 = wd_to_da9053_priv(wd);
	struct device_d *dev = da9053->dev;
	unsigned scale = 0;
	int ret;
	u8 val;

	if (timeout > 131)
		return -EINVAL;

	if (timeout) {
		timeout *= 1000; /* convert to ms */
		scale = 0;
		while (timeout > (2048 << scale) && scale <= 6)
			scale++;
		dev_dbg(dev, "calculated TWDSCALE=%u (req=%ims calc=%ims)\n",
		        scale, timeout, 2048 << scale);
		scale++; /* scale 0 disables the WD */
	}

	ret = da9053_reg_read(da9053, DA9053_CONTROL_D_REG, &val);
	if (ret < 0)
		return ret;

	dev_dbg(dev, "read watchdog (val=0x%02x)\n", val);

	if (scale && scale == (val & DA9053_CONTROLD_TWDSCALE)) {
		/* trigger WD */
		val |= DA9053_CONTROLD_WATCHDOG;
		ret = da9053_reg_write(da9053, DA9053_CONTROL_D_REG, val);
		if (ret < 0)
			return ret;
		dev_dbg(dev, "triggered watchdog (val=0x%02x)\n", val);
	} else {
		/* disable WD first */
		val &= ~DA9053_CONTROLD_TWDSCALE;
		ret = da9053_reg_write(da9053, DA9053_CONTROL_D_REG, val);
		if (ret < 0)
			return ret;

		dev_dbg(dev, "disabled watchdog (val=0x%02x)\n", val);
		if (scale) {
			/* park before waiting */
			ret = da9053_park(da9053);
			if (ret < 0)
				return ret;

			/* wait required time */
			udelay(150);

			/* enable WD with new timeout */
			val |= scale;
			ret = da9053_reg_write(da9053, DA9053_CONTROL_D_REG, val);
			if (ret < 0)
				return ret;
			dev_dbg(dev, "enabled watchdog (val=0x%02x)\n", val);
		}
	}

	ret = da9053_park(da9053);
	if (ret < 0)
		return ret;

	return 0;
}

static void da9053_detect_reset_source(struct da9053_priv *da9053)
{
	unsigned int priority;
	enum reset_src_type type;
	int ret;
	u8 val;

	ret = da9053_reg_read(da9053, DA9053_FAULTLOG_REG, &val);
	if (ret < 0)
		return;

	ret = da9053_park(da9053);
	if (ret < 0)
		return;

	if (val & DA9053_FAULTLOG_TWDERROR)
		type = RESET_WDG;
	else if (val & DA9053_FAULTLOG_VDDFAULT)
		type = RESET_POR;
	else if (val & DA9053_FAULTLOG_NSDSET)
		type = RESET_RST;
	else
		return;

	priority = of_get_reset_source_priority(da9053->dev->device_node);

	reset_source_set_priority(type, priority);

	ret = da9053_reg_write(da9053, DA9053_FAULTLOG_REG, val);
	if (ret < 0)
		return;
}

static void __noreturn da9053_force_system_reset(struct restart_handler *rst)
{
	struct da9053_priv *da9053 = container_of(rst, struct da9053_priv, restart);
	u8 val;
	int ret;

	ret = da9053_reg_read(da9053, DA9053_CONTROL_B_REG, &val);
	if (ret < 0)
		hang();

	val |= DA9053_CONTROLB_SHUTDOWN;
	ret = da9053_reg_write(da9053, DA9053_CONTROL_B_REG, val);
	if (ret < 0)
		hang();

	da9053_park(da9053);

	hang();
}

static int da9053_probe(struct device_d *dev)
{
	struct da9053_priv *da9053;
	int ret;

	da9053 = xzalloc(sizeof(*da9053));
	da9053->dev = dev;
	da9053->client = to_i2c_client(dev);
	da9053->wd.set_timeout = da9053_set_timeout;
	da9053->wd.priority = of_get_watchdog_priority(dev->device_node);
	da9053->wd.hwdev = dev;

	ret = da9053_enable_multiwrite(da9053);
	if (ret < 0)
		return ret;

	ret = watchdog_register(&da9053->wd);
	if (ret)
		return ret;

	da9053_detect_reset_source(da9053);

	da9053->restart.priority = of_get_restart_priority(dev->device_node);
	da9053->restart.name = "da9063";
	da9053->restart.restart = &da9053_force_system_reset;

	restart_handler_register(&da9053->restart);

	return 0;
}

static __maybe_unused struct of_device_id da9053_dt_ids[] = {
	{
		.compatible = "dlg,da9052",
	}, {
		/* sentinel */
	}
};

static struct driver_d da9053_driver = {
	.name  = DRIVERNAME,
	.probe = da9053_probe,
	.of_compatible = DRV_OF_COMPAT(da9053_dt_ids),
};
device_i2c_driver(da9053_driver);
