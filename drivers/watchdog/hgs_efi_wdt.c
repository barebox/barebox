// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2025 Pengutronix

#include <common.h>
#include <init.h>
#include <of_device.h>
#include <watchdog.h>

#include <mfd/hgs-efi.h>

struct hgs_efi_wdt_data {
	unsigned int msg_id;
};

struct hgs_efi_wdt {
	struct watchdog wdd;
	struct hgs_efi *efi;
	const struct hgs_efi_wdt_data *data;
	bool pinged;
};

static struct hgs_efi_wdt *to_hgs_efi_wdt(struct watchdog *wdd)
{
	return container_of(wdd, struct hgs_efi_wdt, wdd);
}

static int hgs_efi_wdt_set_timeout(struct watchdog *wdd, unsigned int timeout)
{
	/*
	 * The set_timeout callback is required by the core, but we actually
	 * can't configure the watchdog, therefore return -ENOSYS.
	 */
	return -ENOSYS;
}

static int hgs_efi_wdt_ping(struct watchdog *wdd)
{
	struct hgs_efi_wdt *efi_wd = to_hgs_efi_wdt(wdd);
	struct device *dev = &wdd->dev;
	struct hgs_sep_cmd cmd = {
		.type = HGS_SEP_MSG_TYPE_EVENT,
		.msg_id = efi_wd->data->msg_id,
	};
	int error;

	/* The EFI watchdog doesn't have a timeout, once pinged */
	if (efi_wd->pinged)
		return 0;

	error = hgs_efi_exec(efi_wd->efi, &cmd);
	if (error) {
		dev_warn(dev, "Failed to send OsRunning/SystemReady\n");
		return error;
	}

	efi_wd->pinged = true;

	return 0;
}

static int hgs_efi_wdt_drv_probe(struct device *dev)
{
	struct hgs_efi_wdt *efi_wd;
	struct watchdog *wdd;
	int error;

	efi_wd = xzalloc(sizeof(*efi_wd));
	efi_wd->efi = dev_get_priv(dev->parent);
	efi_wd->data = of_device_get_match_data(dev);

	wdd = &efi_wd->wdd;
	wdd->hwdev = dev;
	wdd->ping = hgs_efi_wdt_ping;
	wdd->set_timeout = hgs_efi_wdt_set_timeout;
	/* The watchdog is always running */
	wdd->running = WDOG_HW_RUNNING;

	error = watchdog_register(wdd);
	if (error) {
		dev_err(dev, "Failed to register watchdog device\n");
		return error;
	}

	return 0;
}

const struct hgs_efi_wdt_data hgs_efi_wdt_gs05 = {
	.msg_id = 0,
};

static struct of_device_id hgs_efi_wdt_of_match[] = {
	{ .compatible = "hgs,efi-gs05-wdt", .data = &hgs_efi_wdt_gs05 },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, hgs_efi_wdt_of_match);

static struct driver hgs_efi_wdt_driver = {
	.name		= "hgs-efi-wdt",
	.probe		= hgs_efi_wdt_drv_probe,
	.of_compatible	= hgs_efi_wdt_of_match,
};
device_platform_driver(hgs_efi_wdt_driver);
