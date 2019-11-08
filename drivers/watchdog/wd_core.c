/*
 * (c) 2012 Juergen Beisert <kernel@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#define pr_fmt(fmt) "watchdog: " fmt

#include <common.h>
#include <command.h>
#include <errno.h>
#include <linux/ctype.h>
#include <watchdog.h>

static LIST_HEAD(watchdog_list);

static const char *watchdog_name(struct watchdog *wd)
{
	if (wd->hwdev)
		return dev_name(wd->hwdev);
	if (wd->name)
		return wd->name;

	return "unknown";
}

/*
 * start, stop or retrigger the watchdog
 * timeout in [seconds]. timeout of '0' will disable the watchdog (if possible)
 */
int watchdog_set_timeout(struct watchdog *wd, unsigned timeout)
{
	int ret;

	if (!wd)
		return -ENODEV;

	if (timeout > wd->timeout_max)
		return -EINVAL;

	pr_debug("setting timeout on %s to %ds\n", watchdog_name(wd), timeout);

	ret = wd->set_timeout(wd, timeout);
	if (ret)
		return ret;

	wd->running = timeout ? WDOG_HW_RUNNING : WDOG_HW_NOT_RUNNING;

	return 0;
}
EXPORT_SYMBOL(watchdog_set_timeout);

static int watchdog_set_priority(struct param_d *param, void *priv)
{
	struct watchdog *wd = priv;

	if (wd->priority == 0)
		return watchdog_set_timeout(wd, 0);

	return 0;
}

static int watchdog_set_cur(struct param_d *param, void *priv)
{
	struct watchdog *wd = priv;

	if (wd->poller_timeout_cur > wd->timeout_max)
		return -EINVAL;

	return 0;
}

static void watchdog_poller_cb(void *priv);

static void watchdog_poller_start(struct watchdog *wd)
{
	watchdog_set_timeout(wd, wd->poller_timeout_cur);
	poller_call_async(&wd->poller, 500 * MSECOND,
			watchdog_poller_cb, wd);

}

static void watchdog_poller_cb(void *priv)
{
	struct watchdog *wd = priv;

	if (wd->poller_enable)
		watchdog_poller_start(wd);
}

static int watchdog_set_poller(struct param_d *param, void *priv)
{
	struct watchdog *wd = priv;


	if (wd->poller_enable) {
		dev_info(&wd->dev, "enable watchdog poller\n");
		watchdog_poller_start(wd);
	} else {
		dev_info(&wd->dev, "disable watchdog poller\n");
		poller_async_cancel(&wd->poller);
	}

	return 0;
}

static int watchdog_register_poller(struct watchdog *wd)
{
	struct param_d *p;
	int ret;

	ret = poller_async_register(&wd->poller);
	if (ret)
		return ret;

	p = dev_add_param_bool(&wd->dev, "autoping", watchdog_set_poller,
			NULL, &wd->poller_enable, wd);

	return PTR_ERR_OR_ZERO(p);
}

static int watchdog_register_dev(struct watchdog *wd, const char *name, int id)
{
	wd->dev.parent = wd->hwdev;
	wd->dev.id = id;
	dev_set_name(&wd->dev, name);

	return register_device(&wd->dev);
}

/**
 * dev_get_watchdog_priority() - get a device's desired watchdog priority
 * @dev:	The device, which device_node to read the property from
 *
 * return: The priority
 */
static unsigned int dev_get_watchdog_priority(struct device_d *dev)
{
	unsigned int priority = WATCHDOG_DEFAULT_PRIORITY;

	if (dev)
		of_property_read_u32(dev->device_node, "watchdog-priority",
				     &priority);

	return priority;
}

const char *running_names[] = {
	[WDOG_HW_RUNNING_UNSUPPORTED] = "unknown",
	[WDOG_HW_RUNNING] = "1",
	[WDOG_HW_NOT_RUNNING] = "0",
};

int watchdog_register(struct watchdog *wd)
{
	struct param_d *p;
	const char *alias = NULL;
	int ret = 0;

	if (wd->hwdev)
		alias = of_alias_get(wd->hwdev->device_node);

	if (alias)
		ret = watchdog_register_dev(wd, alias, DEVICE_ID_SINGLE);

	if (!alias || ret)
		ret = watchdog_register_dev(wd, "wdog", DEVICE_ID_DYNAMIC);

	if (ret)
		return ret;

	p = dev_add_param_enum_ro(&wd->dev, "running", &wd->running,
				  running_names, ARRAY_SIZE(running_names));
	if (IS_ERR(p))
		return PTR_ERR(p);

	if (!wd->priority)
		wd->priority = dev_get_watchdog_priority(wd->hwdev);

	p = dev_add_param_uint32(&wd->dev, "priority",
				 watchdog_set_priority, NULL,
				 &wd->priority, "%u", wd);
	if (IS_ERR(p))
		return PTR_ERR(p);

	/* set some default sane value */
	if (!wd->timeout_max)
		wd->timeout_max = 60 * 60 * 24;

	p = dev_add_param_uint32_ro(&wd->dev, "timeout_max",
			&wd->timeout_max, "%u");
	if (IS_ERR(p))
		return PTR_ERR(p);

	if (IS_ENABLED(CONFIG_WATCHDOG_POLLER)) {
		if (!wd->poller_timeout_cur ||
		    wd->poller_timeout_cur > wd->timeout_max)
			wd->poller_timeout_cur = wd->timeout_max;

		p = dev_add_param_uint32(&wd->dev, "timeout_cur", watchdog_set_cur,
				NULL, &wd->poller_timeout_cur, "%u", wd);
		if (IS_ERR(p))
			return PTR_ERR(p);

		ret = watchdog_register_poller(wd);
		if (ret)
			return ret;
	}

	list_add_tail(&wd->list, &watchdog_list);

	pr_debug("registering watchdog %s with priority %d\n", watchdog_name(wd),
			wd->priority);

	return 0;
}
EXPORT_SYMBOL(watchdog_register);

int watchdog_deregister(struct watchdog *wd)
{
	if (IS_ENABLED(CONFIG_WATCHDOG_POLLER)) {
		poller_async_cancel(&wd->poller);
		poller_async_unregister(&wd->poller);
	}

	unregister_device(&wd->dev);
	list_del(&wd->list);

	return 0;
}
EXPORT_SYMBOL(watchdog_deregister);

struct watchdog *watchdog_get_default(void)
{
	struct watchdog *tmp, *wd = NULL;
	int priority = 0;

	list_for_each_entry(tmp, &watchdog_list, list) {
		if (tmp->priority > priority) {
			priority = tmp->priority;
			wd = tmp;
		}
	}

	return wd;
}
EXPORT_SYMBOL(watchdog_get_default);

struct watchdog *watchdog_get_by_name(const char *name)
{
	struct watchdog *tmp;
	struct device_d *dev = get_device_by_name(name);
	if (!dev)
		return NULL;

	list_for_each_entry(tmp, &watchdog_list, list) {
		if (dev == tmp->hwdev || dev == &tmp->dev)
			return tmp;
	}

	return NULL;
}
EXPORT_SYMBOL(watchdog_get_by_name);
