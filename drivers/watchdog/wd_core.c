// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * (c) 2012 Juergen Beisert <kernel@pengutronix.de>
 */
#define pr_fmt(fmt) "watchdog: " fmt

#include <common.h>
#include <command.h>
#include <errno.h>
#include <linux/ctype.h>
#include <watchdog.h>
#include <restart.h>

#define for_each_watchdog(wd) list_for_each_entry(wd, &watchdog_class.devices, dev.class_list)

DEFINE_DEV_CLASS(watchdog_class, "watchdog");

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

	if (!timeout && !watchdog_hw_running(wd))
		return 0;

	pr_debug("setting timeout on %s to %ds\n", watchdog_name(wd), timeout);

	ret = wd->set_timeout(wd, timeout);
	if (ret)
		return ret;

	wd->last_ping = get_time_ns();
	wd->timeout_cur = timeout;

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

static void watchdog_register_poller(struct watchdog *wd)
{
	poller_async_register(&wd->poller, dev_name(&wd->dev));

	dev_add_param_bool(&wd->dev, "autoping", watchdog_set_poller,
			   NULL, &wd->poller_enable, wd);
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
static unsigned int dev_get_watchdog_priority(struct device *dev)
{
	unsigned int priority = WATCHDOG_DEFAULT_PRIORITY;

	if (dev)
		of_property_read_u32(dev->of_node, "watchdog-priority",
				     &priority);

	return priority;
}

static int seconds_to_expire_get(struct param_d *p, void *priv)
{
	struct watchdog *wd = priv;
	uint64_t diff;

	if (!wd->timeout_cur) {
		wd->seconds_to_expire = -1;
		return 0;
	}

	diff = get_time_ns() - wd->last_ping;

	do_div(diff, 1000000000);

	wd->seconds_to_expire = wd->timeout_cur - diff;

	return 0;
}

static void __noreturn watchdog_restart_handle(struct restart_handler *this)
{
	struct watchdog *wd = watchdog_get_default();
	int ret = -ENODEV;

	if (wd)
		ret = watchdog_set_timeout(wd, 1);

	BUG_ON(ret);

	mdelay_non_interruptible(2000);

	pr_emerg("Watchdog failed to reset the machine\n");
	hang();
}

static struct restart_handler restart_handler = {
	.restart = watchdog_restart_handle,
	.name = "watchdog-restart",
};

int watchdog_register(struct watchdog *wd)
{
	const char *alias = NULL;
	int ret = 0;

	if (wd->hwdev)
		alias = of_alias_get(wd->hwdev->of_node);

	if (alias)
		ret = watchdog_register_dev(wd, alias, DEVICE_ID_SINGLE);

	if (!alias || ret)
		ret = watchdog_register_dev(wd, "wdog", DEVICE_ID_DYNAMIC);

	if (ret)
		return ret;

	dev_add_param_tristate_ro(&wd->dev, "running", &wd->running);

	if (!wd->priority)
		wd->priority = dev_get_watchdog_priority(wd->hwdev);

	dev_add_param_uint32(&wd->dev, "priority",
				 watchdog_set_priority, NULL,
				 &wd->priority, "%u", wd);

	/* set some default sane value */
	if (!wd->timeout_max)
		wd->timeout_max = 60 * 60 * 24;

	dev_add_param_uint32_ro(&wd->dev, "timeout_max",
			&wd->timeout_max, "%u");

	if (IS_ENABLED(CONFIG_WATCHDOG_POLLER)) {
		if (!wd->poller_timeout_cur ||
		    wd->poller_timeout_cur > wd->timeout_max)
			wd->poller_timeout_cur = wd->timeout_max;

		dev_add_param_uint32(&wd->dev, "timeout_cur", watchdog_set_cur,
				NULL, &wd->poller_timeout_cur, "%u", wd);

		watchdog_register_poller(wd);
	}

	dev_add_param_uint32(&wd->dev, "seconds_to_expire", param_set_readonly,
			seconds_to_expire_get, &wd->seconds_to_expire, "%d", wd);

	if (!restart_handler.priority) {
		restart_handler.priority = 10; /* don't override others */
		ret = restart_handler_register(&restart_handler);
		if (ret)
			dev_warn(&wd->dev, "failed to register restart handler\n");
	}

	class_add_device(&watchdog_class, &wd->dev);

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

	class_remove_device(&watchdog_class, &wd->dev);
	list_del(&wd->list);

	return 0;
}
EXPORT_SYMBOL(watchdog_deregister);

struct watchdog *watchdog_get_default(void)
{
	struct watchdog *tmp, *wd = NULL;
	int priority = 0;

	for_each_watchdog(tmp) {
		if (tmp->priority > priority) {
			priority = tmp->priority;
			wd = tmp;
		}
	}

	return wd;
}
EXPORT_SYMBOL(watchdog_get_default);

int watchdog_get_alias_id_from(struct watchdog *wd, struct device_node *root)
{
	struct device_node *dstnp, *srcnp = wd->hwdev ? wd->hwdev->of_node : NULL;
	char *name;

	if (!srcnp)
		return -ENODEV;

	name = of_get_reproducible_name(srcnp);
	dstnp = of_find_node_by_reproducible_name(root, name);
	free(name);

	if (!dstnp)
		return -ENODEV;

	return of_alias_get_id_from(root, wd->hwdev->of_node, "watchdog");
}
EXPORT_SYMBOL(watchdog_get_alias_id_from);

struct watchdog *watchdog_get_by_name(const char *name)
{
	struct watchdog *tmp;
	struct device *dev = get_device_by_name(name);
	if (!dev)
		return NULL;

	for_each_watchdog(tmp) {
		if (dev == tmp->hwdev || dev == &tmp->dev)
			return tmp;
	}

	return NULL;
}
EXPORT_SYMBOL(watchdog_get_by_name);

int watchdog_inhibit_all(void)
{
	struct watchdog *wd;
	int ret = 0;

	for_each_watchdog(wd) {
		int err;
		if (!wd->priority || watchdog_hw_running(wd) == false)
			continue;

		err = watchdog_set_timeout(wd, 0);
		if (!err)
			continue;

		if (err != -ENOSYS || !IS_ENABLED(CONFIG_WATCHDOG_POLLER)) {
			ret = err;
			continue;
		}

		wd->poller_enable = true;
		watchdog_poller_start(wd);
	}

	return ret;
}
EXPORT_SYMBOL(watchdog_inhibit_all);
