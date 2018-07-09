// SPDX-License-Identifier: GPL-2.0+

/*
 * Driver for watchdog aspect of for Zodiac Inflight Innovations RAVE
 * Supervisory Processor(SP) MCU
 *
 * Copyright (C) 2018 Zodiac Inflight Innovation
 *
 */

#include <common.h>
#include <init.h>
#include <of_device.h>
#include <restart.h>
#include <watchdog.h>

#include <debug_ll.h>

#include <linux/mfd/rave-sp.h>
#include <linux/nvmem-consumer.h>

enum {
	RAVE_SP_RESET_BYTE = 1,
	RAVE_SP_RESET_REASON_NORMAL         = 0,
	RAVE_SP_RESET_REASON_HW_WATCHDOG    = 1,
	RAVE_SP_RESET_REASON_SW_WATCHDOG    = 2,
	RAVE_SP_RESET_REASON_VOLTAGE        = 3,
	RAVE_SP_RESET_REASON_HOST_REQUEST   = 4,
	RAVE_SP_RESET_REASON_TEMPERATURE    = 5,
	RAVE_SP_RESET_REASON_BUTTON_PRESS   = 6,
	RAVE_SP_RESET_REASON_PIC_CM         = 7,
	RAVE_SP_RESET_REASON_PIC_ILL_INST   = 8,
	RAVE_SP_RESET_REASON_PIC_TRAP       = 9,
	RAVE_SP_RESET_REASON_UKNOWN_REASON  = 10,
	RAVE_SP_RESET_REASON_THERMAL_SENSOR = 11,
	RAVE_SP_RESET_REASON_SW_VOLTAGE     = 12,
	RAVE_SP_RESET_REASON_CP_REQUEST     = 13,

	RAVE_SP_RESET_DELAY_MS = 500,
};

/**
 * struct rave_sp_wdt_variant - RAVE SP watchdog variant
 *
 * @max_timeout:	Largest possible watchdog timeout setting
 * @min_timeout:	Smallest possible watchdog timeout setting
 *
 * @configure:		Function to send configuration command
 * @restart:		Function to send "restart" command
 */
struct rave_sp_wdt_variant {
	unsigned int max_timeout;
	unsigned int min_timeout;

	int (*configure)(struct watchdog *, bool);
	int (*restart)(struct watchdog *);
	int (*reset_reason)(struct watchdog *);
};

/**
 * struct rave_sp_wdt - RAVE SP watchdog
 *
 * @wdd:		Underlying watchdog device
 * @sp:			Pointer to parent RAVE SP device
 * @variant:		Device specific variant information
 * @reboot_notifier:	Reboot notifier implementing machine reset
 */
struct rave_sp_wdt {
	struct watchdog wdd;
	struct rave_sp *sp;
	const struct rave_sp_wdt_variant *variant;
	struct restart_handler restart;
	unsigned int timeout;
	unsigned int boot_source;
	struct nvmem_cell *boot_source_cell;
};

static struct rave_sp_wdt *to_rave_sp_wdt(struct watchdog *wdd)
{
	return container_of(wdd, struct rave_sp_wdt, wdd);
}

static int rave_sp_wdt_exec(struct watchdog *wdd, void *data,
			    size_t data_size)
{
	return rave_sp_exec(to_rave_sp_wdt(wdd)->sp,
			    data, data_size, NULL, 0);
}

static int __rave_sp_wdt_rdu_reset_reason(struct watchdog *wdd,
					  uint8_t response[],
					  size_t response_len)
{
	u8 cmd[] = {
		[0] = RAVE_SP_CMD_RESET_REASON,
		[1] = 0,
	};
	int ret;

	ret = rave_sp_exec(to_rave_sp_wdt(wdd)->sp, cmd, sizeof(cmd),
			   response, response_len);
	if (ret)
		return ret;
	/*
	 * Non "legacy" watchdog variants return 2 bytes as response
	 * whereas "legacy" ones return only one. Both however send
	 * the data we need as a first byte of the response.
	 */
	return response[0];
}

static int rave_sp_wdt_rdu_reset_reason(struct watchdog *wdd)
{
	u8 response[2];
	return __rave_sp_wdt_rdu_reset_reason(wdd, response, sizeof(response));
}

static int rave_sp_wdt_legacy_reset_reason(struct watchdog *wdd)
{
	u8 response[1];
	return __rave_sp_wdt_rdu_reset_reason(wdd, response, sizeof(response));
}

static int rave_sp_wdt_legacy_configure(struct watchdog *wdd, bool on)
{
	struct rave_sp_wdt *sp_wd = to_rave_sp_wdt(wdd);

	u8 cmd[] = {
		[0] = RAVE_SP_CMD_SW_WDT,
		[1] = 0,
		[2] = 0,
		[3] = on,
		[4] = on ? sp_wd->timeout : 0,
	};

	return rave_sp_wdt_exec(wdd, cmd, sizeof(cmd));
}

static int rave_sp_wdt_rdu_configure(struct watchdog *wdd, bool on)
{
	struct rave_sp_wdt *sp_wd = to_rave_sp_wdt(wdd);

	u8 cmd[] = {
		[0] = RAVE_SP_CMD_SW_WDT,
		[1] = 0,
		[2] = on,
		[3] = sp_wd->timeout,
		[4] = (u8)(sp_wd->timeout >> 8),
	};

	return rave_sp_wdt_exec(wdd, cmd, sizeof(cmd));
}

static int rave_sp_wdt_configure(struct watchdog *wdd, bool on)
{
	return to_rave_sp_wdt(wdd)->variant->configure(wdd, on);
}

static int rave_sp_wdt_legacy_restart(struct watchdog *wdd)
{
	u8 cmd[] = {
		[0] = RAVE_SP_CMD_RESET,
		[1] = 0,
		[2] = RAVE_SP_RESET_BYTE
	};

	return rave_sp_wdt_exec(wdd, cmd, sizeof(cmd));
}

static int rave_sp_wdt_rdu_restart(struct watchdog *wdd)
{
	u8 cmd[] = {
		[0] = RAVE_SP_CMD_RESET,
		[1] = 0,
		[2] = RAVE_SP_RESET_BYTE,
		[3] = RAVE_SP_RESET_REASON_NORMAL
	};

	return rave_sp_wdt_exec(wdd, cmd, sizeof(cmd));
}

static void __noreturn rave_sp_wdt_restart_handler(struct restart_handler *rst)
{
	struct rave_sp_wdt *sp_wd =
			container_of(rst, struct rave_sp_wdt, restart);

	const int ret = sp_wd->variant->restart(&sp_wd->wdd);

	if (ret < 0)
		dev_err(&sp_wd->wdd.dev,
			"Failed to issue restart command (%d)", ret);
	/*
	 * The actual work was done by reboot notifier above. SP
	 * firmware waits 500 ms before issuing reset, so let's hang
	 * here for twice that delay and hopefuly we'd never reach
	 * the return statement.
	 */
	mdelay(2 * RAVE_SP_RESET_DELAY_MS);
	hang();
}

static int rave_sp_wdt_start(struct watchdog *wdd)
{
	return rave_sp_wdt_configure(wdd, true);
}

static int rave_sp_wdt_stop(struct watchdog *wdd)
{
	return rave_sp_wdt_configure(wdd, false);
}

static int rave_sp_wdt_set_timeout(struct watchdog *wdd,
				   unsigned int timeout)
{
	struct rave_sp_wdt *sp_wd = to_rave_sp_wdt(wdd);

	if (!timeout)
		return rave_sp_wdt_stop(wdd);

	if (timeout < sp_wd->variant->min_timeout ||
	    timeout > sp_wd->variant->max_timeout)
		return -EINVAL;

	sp_wd->timeout = timeout;
	return rave_sp_wdt_start(wdd);
}

static const struct rave_sp_wdt_variant rave_sp_wdt_legacy = {
	.max_timeout  = 255,
	.min_timeout  = 1,
	.configure    = rave_sp_wdt_legacy_configure,
	.restart      = rave_sp_wdt_legacy_restart,
	.reset_reason = rave_sp_wdt_legacy_reset_reason,
};

static const struct rave_sp_wdt_variant rave_sp_wdt_rdu = {
	.max_timeout  = 180,
	.min_timeout  = 60,
	.configure    = rave_sp_wdt_rdu_configure,
	.restart      = rave_sp_wdt_rdu_restart,
	.reset_reason = rave_sp_wdt_rdu_reset_reason,
};

static const struct of_device_id rave_sp_wdt_of_match[] = {
	{
		.compatible = "zii,rave-sp-watchdog-legacy",
		.data = &rave_sp_wdt_legacy,
	},
	{
		.compatible = "zii,rave-sp-watchdog",
		.data = &rave_sp_wdt_rdu,
	},
	{ /* sentinel */ }
};

static int rave_sp_wdt_set_boot_source(struct param_d *param, void *priv)
{
	struct rave_sp_wdt *sp_wd = priv;
	u8 boot_source = sp_wd->boot_source;
	int ret;

	ret = nvmem_cell_write(sp_wd->boot_source_cell, &boot_source,
			       sizeof(boot_source));
	if (ret < 0)
		return ret;

	return 0;
}

static int rave_sp_wdt_get_boot_source(struct param_d *param, void *priv)
{
	struct rave_sp_wdt *sp_wd = priv;
	u8 *boot_source;
	size_t len;

	boot_source = nvmem_cell_read(sp_wd->boot_source_cell, &len);
	if (IS_ERR(boot_source))
		return PTR_ERR(boot_source);

	sp_wd->boot_source = *boot_source;
	kfree(boot_source);

	return 0;
}

static int rave_sp_wdt_add_params(struct rave_sp_wdt *sp_wd)
{
	struct watchdog *wdd = &sp_wd->wdd;
	struct device_node *np = wdd->hwdev->device_node;
	struct param_d *p;

	sp_wd->boot_source_cell = of_nvmem_cell_get(np, "boot-source");
	if (IS_ERR(sp_wd->boot_source_cell)) {
		dev_warn(wdd->hwdev, "No bootsource info availible\n");
		return 0;
	}

	p = dev_add_param_int(&wdd->dev, "boot_source",
			      rave_sp_wdt_set_boot_source,
			      rave_sp_wdt_get_boot_source,
			      &sp_wd->boot_source, "%u", sp_wd);
	if (IS_ERR(p))
		return PTR_ERR(p);

	return 0;
}

static int rave_sp_wdt_probe(struct device_d *dev)
{
	struct rave_sp_wdt *sp_wd;
	const char *reset_reason;
	struct nvmem_cell *cell;
	struct watchdog *wdd;
	__le16 timeout = 60;
	int ret;

	sp_wd = xzalloc(sizeof(*sp_wd));
	sp_wd->variant = of_device_get_match_data(dev);
	sp_wd->sp      = dev->parent->priv;

	cell = nvmem_cell_get(dev, "wdt-timeout");
	if (!IS_ERR(cell)) {
		size_t len;
		void *value = nvmem_cell_read(cell, &len);

		if (!IS_ERR(value)) {
			memcpy(&timeout, value, min(len, sizeof(timeout)));
			kfree(value);
		}
		nvmem_cell_put(cell);
	}
	sp_wd->timeout = le16_to_cpu(timeout);

	wdd              = &sp_wd->wdd;
	wdd->hwdev       = dev;
	wdd->set_timeout = rave_sp_wdt_set_timeout;

	ret = rave_sp_wdt_stop(wdd);
	if (ret) {
		dev_err(dev, "Failed to stop watchdog device\n");
		return ret;
	}

	ret = watchdog_register(wdd);
	if (ret) {
		dev_err(dev, "Failed to register watchdog device\n");

		return ret;
	}

	ret = rave_sp_wdt_add_params(sp_wd);
	if (ret) {
		dev_err(dev, "Failed to register device parameters");
		return ret;
	}

	sp_wd->restart.name = "rave-sp-wdt";
	sp_wd->restart.restart = rave_sp_wdt_restart_handler;
	sp_wd->restart.priority = 200;

	ret = restart_handler_register(&sp_wd->restart);
	if (ret)
		dev_warn(dev, "Cannot register restart handler\n");

	ret = sp_wd->variant->reset_reason(wdd);
	if (ret < 0) {
		dev_warn(dev, "Failed to query reset reason\n");
		return 0;
	}

	switch (ret) {
	case RAVE_SP_RESET_REASON_NORMAL:
		reset_reason = "Normal poweroff";
		break;
	case RAVE_SP_RESET_REASON_HW_WATCHDOG:
		reset_reason = "PIC hardware watchdog";
		break;
	case RAVE_SP_RESET_REASON_SW_WATCHDOG:
		reset_reason = "PIC software watchdog";
		break;
	case RAVE_SP_RESET_REASON_VOLTAGE:
		reset_reason = "Input voltage out of range";
		break;
	case RAVE_SP_RESET_REASON_HOST_REQUEST:
		reset_reason = "Host requested";
		break;
	case RAVE_SP_RESET_REASON_TEMPERATURE:
		reset_reason = "Temperature out of range";
		break;
	case RAVE_SP_RESET_REASON_BUTTON_PRESS:
		reset_reason = "User requested";
		break;
	case RAVE_SP_RESET_REASON_PIC_CM:
		reset_reason = "Illegal configuration word";
		break;
	case RAVE_SP_RESET_REASON_PIC_ILL_INST:
		reset_reason = "Illegal instruction";
		break;
	case RAVE_SP_RESET_REASON_PIC_TRAP:
		reset_reason = "Illegal trap";
		break;
	default:
	case RAVE_SP_RESET_REASON_UKNOWN_REASON:
		reset_reason = "Unknown";
		break;
	case RAVE_SP_RESET_REASON_THERMAL_SENSOR:
		reset_reason = "Thermal sensor";
		break;
	case RAVE_SP_RESET_REASON_SW_VOLTAGE:
		reset_reason = "Software detected brownout";
		break;
	case RAVE_SP_RESET_REASON_CP_REQUEST:
		reset_reason = "Command request";
		break;
	}

	dev_info(dev, "Reset reason: %s\n", reset_reason);
	return 0;
}

static struct driver_d rave_sp_wdt_driver = {
	.name  = "rave-sp-wdt",
	.probe = rave_sp_wdt_probe,
	.of_compatible = DRV_OF_COMPAT(rave_sp_wdt_of_match),
};
console_platform_driver(rave_sp_wdt_driver);
