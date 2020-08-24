// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (C) 2019 Oleksij Rempel <o.rempel@pengutronix.de>, Pengutronix
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <efi.h>
#include <efi/efi.h>
#include <watchdog.h>

struct efi_wdt_priv {
	struct watchdog		wd;
	struct device_d		*dev;
};

#define to_efi_wdt(h) container_of(h, struct efi_wdt_priv, wd)

static int efi_wdt_set_timeout(struct watchdog *wd, unsigned timeout)
{
	struct efi_wdt_priv *priv = to_efi_wdt(wd);
	efi_status_t efiret;

	efiret = BS->set_watchdog_timer(timeout, 0, 0, NULL);
	if (EFI_ERROR(efiret)) {
		dev_err(priv->dev, "failed to set EFI watchdog: %lx\n", efiret);
		return -EINVAL;
	}

	return 0;
}

static int efi_wdt_probe(struct device_d *dev)
{
	struct efi_wdt_priv *priv;
	int ret;

	priv = xzalloc(sizeof(*priv));

	priv->wd.set_timeout = efi_wdt_set_timeout;
	priv->wd.hwdev = dev;
	priv->dev = dev;
	priv->wd.priority = WATCHDOG_DEFAULT_PRIORITY - 50;

	dev->priv = priv;

	priv->wd.timeout_max = U32_MAX;

	ret = watchdog_register(&priv->wd);
	if (ret)
		goto on_error;

	return 0;

on_error:
	free(priv);
	return ret;
}

static struct driver_d efi_wdt_driver = {
	.name = "efi-wdt",
	.probe = efi_wdt_probe,
};
device_platform_driver(efi_wdt_driver);
