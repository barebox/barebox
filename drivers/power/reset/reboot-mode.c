// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2016, Fuzhou Rockchip Electronics Co., Ltd
 * Copyright (c) 2019, Ahmad Fatoum, Pengutronix
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <of.h>
#include <linux/reboot-mode.h>
#include <globalvar.h>
#include <magicvar.h>

static int __priority;
static struct reboot_mode_driver *__boot_mode;

static int reboot_mode_param_set(struct param_d *p, void *priv)
{
	struct reboot_mode_driver *reboot = priv;
	size_t i = reboot->reboot_mode_next * reboot->nelems;

	return reboot->write(reboot, &reboot->magics[i]);
}

static int reboot_mode_add_param(struct device *dev,
				 const char *prefix,
				 struct reboot_mode_driver *reboot)
{
	char name[sizeof "system.reboot_mode.when"];
	struct param_d *param;

	scnprintf(name, sizeof(name), "%sprev", prefix);

	param = dev_add_param_enum_ro(dev, name,
				      &reboot->reboot_mode_prev, reboot->modes,
				      reboot->nmodes);
	if (IS_ERR(param))
		return PTR_ERR(param);

	scnprintf(name, sizeof(name), "%snext", prefix);

	param = dev_add_param_enum(dev, name,
				   reboot_mode_param_set, NULL,
				   &reboot->reboot_mode_next, reboot->modes,
				   reboot->nmodes, reboot);

	return PTR_ERR_OR_ZERO(param);
}

static int of_reboot_mode_fixup(struct device_node *root, void *ctx)
{
	struct reboot_mode_driver *reboot = ctx;
	struct device_node *dstnp, *srcnp, *dstparent;

	srcnp = reboot->dev->of_node;
	dstnp = of_get_node_by_reproducible_name(root, srcnp);

	if (dstnp) {
		dstparent = dstnp->parent;
		of_delete_node(dstnp);
	} else {
		dstparent = of_get_node_by_reproducible_name(root, srcnp->parent);
	}

	if (!dstparent)
		return -EINVAL;

	of_copy_node(dstparent, srcnp);

	return 0;
}

static __maybe_unused int reboot_mode_add_globalvar(void)
{
	struct reboot_mode_driver *reboot = __boot_mode;

	if (!reboot)
		return 0;

	if (!reboot->no_fixup)
		of_register_fixup(of_reboot_mode_fixup, reboot);

	return reboot_mode_add_param(&global_device, "system.reboot_mode.", reboot);
}
#ifdef CONFIG_GLOBALVAR
late_initcall(reboot_mode_add_globalvar);
#endif

static void reboot_mode_print(struct reboot_mode_driver *reboot,
			      const char *prefix, const u32 *arr)
{
	size_t i;
	dev_dbg(reboot->dev, "%s: ", prefix);
	for (i = 0; i < reboot->nelems; i++)
		__pr_printk(7, "%08x ", arr[i]);
	__pr_printk(7, "\n");
}

static const char *get_mode_name(const struct property *prop)
{
	unsigned prefix_len;

	prefix_len = str_has_prefix(prop->name, "mode-");
	if (!prefix_len)
		prefix_len = str_has_prefix(prop->name, "barebox,mode-");
	if (!prefix_len)
		return NULL;

	return prop->name + prefix_len;
}

/**
 * reboot_mode_register - register a reboot mode driver
 * @reboot: reboot mode driver
 * @reboot_mode: reboot mode read from hardware
 *
 * Returns: 0 on success or a negative error code on failure.
 */
int reboot_mode_register(struct reboot_mode_driver *reboot,
			 const u32 *reboot_mode, size_t nelems)
{
	struct property *prop;
	struct device_node *np = reboot->dev->of_node;
	const char *alias;
	size_t nmodes = 0;
	int i = 0;
	int ret, normal = -1;

	for_each_property_of_node(np, prop) {
		u32 magic;

		if (!get_mode_name(prop))
			continue;
		if (of_property_read_u32(np, prop->name, &magic))
			continue;

		nmodes++;
	}

	reboot->nmodes = nmodes;
	reboot->nelems = nelems;

	/*
	 * Allocate one entry more than necessary, because in the loop below
	 * we use an entry before we realize that the property is not valid.
	 */
	reboot->magics = xzalloc((nmodes + 1) * nelems * sizeof(u32));
	reboot->modes = xzalloc((nmodes + 1) * sizeof(const char *));

	reboot_mode_print(reboot, "registering magic", reboot_mode);

	for_each_property_of_node(np, prop) {
		const char **mode;
		u32 *magic;

		magic = &reboot->magics[i * nelems];
		mode = &reboot->modes[i];

		*mode = get_mode_name(prop);
		if (!*mode)
			continue;
		if (*mode[0] == '\0') {
			ret = -EINVAL;
			dev_err(reboot->dev, "invalid mode name(%s): too short!\n",
				prop->name);
			goto error;
		}

		if (!strcmp(*mode, "normal"))
			normal = i;

		if (of_property_read_u32_array(np, prop->name, magic, nelems)) {
			dev_err(reboot->dev, "reboot mode %s without magic number\n",
				*mode);
			continue;
		}

		reboot_mode_print(reboot, *mode, magic);

		i++;
	}

	for (i = 0; i < reboot->nmodes; i++) {
		if (memcmp(&reboot->magics[i * nelems], reboot_mode, nelems * sizeof(u32)))
			continue;

		reboot->reboot_mode_prev = i;
		break;
	}

	reboot_mode_add_param(reboot->dev, "", reboot);

	/* clear mode for next reboot */
	if (normal >= 0)
		reboot->write(reboot, &reboot->magics[normal]);

	if (!reboot->priority)
		reboot->priority = REBOOT_MODE_DEFAULT_PRIORITY;

	if (reboot->priority >= __priority) {
		__priority = reboot->priority;
		__boot_mode = reboot;
	}


	alias = of_alias_get(np);
	if (alias)
		dev_set_name(reboot->dev, alias);

	return 0;

error:
	free(reboot->magics);
	free(reboot->modes);

	return ret;
}
EXPORT_SYMBOL_GPL(reboot_mode_register);

const char *reboot_mode_get(void)
{
	if (!__boot_mode)
		return NULL;

	return __boot_mode->modes[__boot_mode->reboot_mode_prev];
}
EXPORT_SYMBOL_GPL(reboot_mode_get);

BAREBOX_MAGICVAR(global.system.reboot_mode.prev,
		 "reboot-mode: Mode set previously, before barebox start");
BAREBOX_MAGICVAR(global.system.reboot_mode.next,
		 "reboot-mode: Mode to set next, to be evaluated after reset");
