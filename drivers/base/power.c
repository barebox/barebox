// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <driver.h>
#include <errno.h>
#include <linux/device.h>
#include <of.h>

#include <pm_domain.h>

#define genpd_status_on(genpd)		(genpd->status == GPD_STATE_ACTIVE)

static LIST_HEAD(gpd_list);

static inline struct generic_pm_domain *dev_to_genpd(struct device *dev)
{
	if (IS_ERR_OR_NULL(dev->pm_domain))
		return ERR_PTR(-EINVAL);

	return dev->pm_domain;
}

/**
 * pm_genpd_init - Initialize a generic I/O PM domain object.
 * @genpd: PM domain object to initialize.
 * @gov: PM domain governor to associate with the domain (may be NULL).
 * @is_off: Initial value of the domain's power_is_off field.
 *
 * Returns 0 on successful initialization, else a negative error code.
 */
int pm_genpd_init(struct generic_pm_domain *genpd, void *gov, bool is_off)
{
	if (IS_ERR_OR_NULL(genpd))
		return -EINVAL;

	genpd->status = is_off ? GPD_STATE_POWER_OFF : GPD_STATE_ACTIVE;

	list_add(&genpd->gpd_list_node, &gpd_list);

	return 0;
}
EXPORT_SYMBOL_GPL(pm_genpd_init);

int pm_genpd_remove(struct generic_pm_domain *genpd)
{
	if (IS_ERR_OR_NULL(genpd))
		return -EINVAL;

	list_del(&genpd->gpd_list_node);

	pr_debug("%s: removed %s\n", __func__, genpd->name);

	return 0;
}
EXPORT_SYMBOL_GPL(pm_genpd_remove);

/**
 * struct of_genpd_provider - PM domain provider registration structure
 * @link: Entry in global list of PM domain providers
 * @node: Pointer to device tree node of PM domain provider
 * @xlate: Provider-specific xlate callback mapping a set of specifier cells
 *         into a PM domain.
 * @data: context pointer to be passed into @xlate callback
 */
struct of_genpd_provider {
	struct list_head link;
	struct device_node *node;
	genpd_xlate_t xlate;
	void *data;
};

/* List of registered PM domain providers. */
static LIST_HEAD(of_genpd_providers);

static bool genpd_present(const struct generic_pm_domain *genpd)
{
	const struct generic_pm_domain *gpd;

	if (IS_ERR_OR_NULL(genpd))
		return false;

	list_for_each_entry(gpd, &gpd_list, gpd_list_node)
		if (gpd == genpd)
			return true;

	return false;
}

/**
 * genpd_xlate_simple() - Xlate function for direct node-domain mapping
 * @genpdspec: OF phandle args to map into a PM domain
 * @data: xlate function private data - pointer to struct generic_pm_domain
 *
 * This is a generic xlate function that can be used to model PM domains that
 * have their own device tree nodes. The private data of xlate function needs
 * to be a valid pointer to struct generic_pm_domain.
 */
static struct generic_pm_domain *genpd_xlate_simple(
					struct of_phandle_args *genpdspec,
					void *data)
{
	return data;
}

/**
 * genpd_xlate_onecell() - Xlate function using a single index.
 * @genpdspec: OF phandle args to map into a PM domain
 * @data: xlate function private data - pointer to struct genpd_onecell_data
 *
 * This is a generic xlate function that can be used to model simple PM domain
 * controllers that have one device tree node and provide multiple PM domains.
 * A single cell is used as an index into an array of PM domains specified in
 * the genpd_onecell_data struct when registering the provider.
 */
static struct generic_pm_domain *genpd_xlate_onecell(
					struct of_phandle_args *genpdspec,
					void *data)
{
	struct genpd_onecell_data *genpd_data = data;
	unsigned int idx = genpdspec->args[0];

	if (genpdspec->args_count != 1)
		return ERR_PTR(-EINVAL);

	if (idx >= genpd_data->num_domains) {
		pr_err("%s: invalid domain index %u\n", __func__, idx);
		return ERR_PTR(-EINVAL);
	}

	if (!genpd_data->domains[idx])
		return ERR_PTR(-ENOENT);

	return genpd_data->domains[idx];
}

/**
 * genpd_add_provider() - Register a PM domain provider for a node
 * @np: Device node pointer associated with the PM domain provider.
 * @xlate: Callback for decoding PM domain from phandle arguments.
 * @data: Context pointer for @xlate callback.
 */
static int genpd_add_provider(struct device_node *np, genpd_xlate_t xlate,
			      void *data)
{
	struct of_genpd_provider *cp;

	cp = kzalloc(sizeof(*cp), GFP_KERNEL);
	if (!cp)
		return -ENOMEM;

	cp->node  = np;
	cp->data  = data;
	cp->xlate = xlate;

	list_add(&cp->link, &of_genpd_providers);
	pr_debug("Added domain provider from %pOF\n", np);

	return 0;
}

/**
 * of_genpd_add_provider_simple() - Register a simple PM domain provider
 * @np: Device node pointer associated with the PM domain provider.
 * @genpd: Pointer to PM domain associated with the PM domain provider.
 */
int of_genpd_add_provider_simple(struct device_node *np,
				 struct generic_pm_domain *genpd)
{
	int ret = -EINVAL;

	if (!np || !genpd)
		return -EINVAL;

	if (genpd_present(genpd))
		ret = genpd_add_provider(np, genpd_xlate_simple, genpd);

	return ret;
}
EXPORT_SYMBOL_GPL(of_genpd_add_provider_simple);

/**
 * of_genpd_add_provider_onecell() - Register a onecell PM domain provider
 * @np: Device node pointer associated with the PM domain provider.
 * @data: Pointer to the data associated with the PM domain provider.
 */
int of_genpd_add_provider_onecell(struct device_node *np,
				  struct genpd_onecell_data *data)
{
	struct generic_pm_domain *genpd;
	unsigned int i;
	int ret = -EINVAL;

	if (!np || !data)
		return -EINVAL;

	if (!data->xlate)
		data->xlate = genpd_xlate_onecell;

	for (i = 0; i < data->num_domains; i++) {
		genpd = data->domains[i];

		if (!genpd)
			continue;
		if (!genpd_present(genpd))
			goto error;
	}

	ret = genpd_add_provider(np, data->xlate, data);
	if (ret < 0)
		goto error;

	return 0;

error:
	while (i--) {
		genpd = data->domains[i];

		if (!genpd)
			continue;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(of_genpd_add_provider_onecell);

/**
 * of_genpd_del_provider() - Remove a previously registered PM domain provider
 * @np: Device node pointer associated with the PM domain provider
 */
void of_genpd_del_provider(struct device_node *np)
{
	struct of_genpd_provider *cp, *tmp;

	list_for_each_entry_safe(cp, tmp, &of_genpd_providers, link) {
		if (cp->node == np) {
			list_del(&cp->link);
			kfree(cp);
			break;
		}
	}
}
EXPORT_SYMBOL_GPL(of_genpd_del_provider);

/**
 * genpd_get_from_provider() - Look-up PM domain
 * @genpdspec: OF phandle args to use for look-up
 *
 * Looks for a PM domain provider under the node specified by @genpdspec and if
 * found, uses xlate function of the provider to map phandle args to a PM
 * domain.
 *
 * Returns a valid pointer to struct generic_pm_domain on success or ERR_PTR()
 * on failure.
 */
static struct generic_pm_domain *genpd_get_from_provider(
					struct of_phandle_args *genpdspec)
{
	struct generic_pm_domain *genpd = ERR_PTR(-ENOENT);
	struct device_node *node = genpdspec->np;
	struct of_genpd_provider *provider;
	int ret;

	if (!genpdspec)
		return ERR_PTR(-EINVAL);

	ret = of_device_ensure_probed(node);
	if (ret) {
		struct device_node *parent;

		/*
		 * If "barebox,allow-dummy" property is set for power domain
		 * provider, assume it's turned on.
		 */
		parent = of_get_parent(node);
		if (of_get_property(node, "barebox,allow-dummy", NULL) ||
		    of_get_property(parent, "barebox,allow-dummy", NULL))
			return NULL;

		return ERR_PTR(ret);
	}

	/* Check if we have such a provider in our array */
	list_for_each_entry(provider, &of_genpd_providers, link) {
		if (provider->node == node)
			genpd = provider->xlate(genpdspec, provider->data);
		if (!IS_ERR(genpd))
			break;
	}

	return genpd;
}

static int _genpd_power_on(struct generic_pm_domain *genpd)
{
	if (!genpd->power_on)
		return 0;

	return genpd->power_on(genpd);
}

/**
 * genpd_power_on - Restore power to a given PM domain and its masters.
 * @genpd: PM domain to power up.
 *
 * Restore power to @genpd and all of its masters so that it is possible to
 * resume a device belonging to it.
 */
static int genpd_power_on(struct generic_pm_domain *genpd)
{
	int ret;

	if (!genpd || genpd_status_on(genpd))
		return 0;

	ret = _genpd_power_on(genpd);
	if (ret)
		return ret;

	genpd->status = GPD_STATE_ACTIVE;

	return 0;
}

int pm_runtime_resume_and_get_genpd(struct device *dev)
{
	struct generic_pm_domain *genpd;

	genpd = dev_to_genpd(dev);
	if (IS_ERR(genpd))
		return PTR_ERR(genpd);

	return genpd_power_on(genpd);
}

static void genpd_add_device(struct generic_pm_domain *genpd, struct device *dev)
{
	dev->pm_domain = genpd;
}

static void genpd_remove_device(struct generic_pm_domain *genpd,
			       struct device *dev)
{
	dev->pm_domain = NULL;
}

static bool have_genpd_providers;

void genpd_activate(void)
{
	have_genpd_providers = true;
}

static struct bus_type genpd_bus_type = {
	.name		= "genpd",
};

static int __init genpd_bus_init(void)
{
	return bus_register(&genpd_bus_type);
}
core_initcall(genpd_bus_init);

static int __genpd_dev_pm_attach(struct device *dev,
				 unsigned int index, bool power_on)
{
	struct of_phandle_args pd_args;
	struct generic_pm_domain *pd;
	int ret;

	if (!have_genpd_providers)
		return 0;

	ret = of_parse_phandle_with_args(dev->of_node, "power-domains",
				"#power-domain-cells", index, &pd_args);
	if (ret < 0)
		return ret;

	pd = genpd_get_from_provider(&pd_args);
	if (IS_ERR(pd)) {
		ret = PTR_ERR(pd);
		dev_dbg(dev, "%s() failed to find PM domain: %d\n",
			__func__, ret);
		/*
		 * Assume that missing genpds are unresolved
		 * dependency are report them as deferred
		 */
		return (ret == -ENOENT) ? -EPROBE_DEFER : ret;
	}

	dev_dbg(dev, "adding to PM domain %s\n", pd ? pd->name : "dummy");

	genpd_add_device(pd, dev);

	if (power_on) {
		ret = genpd_power_on(pd);
		if (ret < 0)
			genpd_remove_device(pd, dev);
	}

	return ret ?: 1;
}

/**
 * genpd_dev_pm_attach - Attach a device to its PM domain using DT.
 * @dev: Device to attach.
 *
 * Parse device's OF node to find a PM domain specifier. If such is found,
 * attaches the device to retrieved pm_domain ops.
 *
 * Returns 1 on successfully attached PM domain, 0 when the device don't need a
 * PM domain or when multiple power-domains exists for it, else a negative error
 * code. Note that if a power-domain exists for the device, but it cannot be
 * found or turned on, then return -EPROBE_DEFER to ensure that the device is
 * not probed and to re-try again later.
 */
int genpd_dev_pm_attach(struct device *dev)
{
	if (!dev->of_node)
		return 0;

	/*
	 * Devices with multiple PM domains must be attached separately, as we
	 * can only attach one PM domain per device.
	 */
	if (of_count_phandle_with_args(dev->of_node, "power-domains",
				       "#power-domain-cells") != 1)
		return 0;

	return __genpd_dev_pm_attach(dev, 0, true);
}
EXPORT_SYMBOL_GPL(genpd_dev_pm_attach);

/**
 * dev_pm_domain_attach_by_id - Associate a device with one of its PM domains.
 * @dev: The device used to lookup the PM domain.
 * @index: The index of the PM domain.
 *
 * As @dev may only be attached to a single PM domain, the backend PM domain
 * provider creates a virtual device to attach instead. If attachment succeeds,
 * the ->detach() callback in the struct dev_pm_domain are assigned by the
 * corresponding backend attach function, as to deal with detaching of the
 * created virtual device.
 *
 * This function should typically be invoked by a driver during the probe phase,
 * in case its device requires power management through multiple PM domains. The
 * driver may benefit from using the received device, to configure device-links
 * towards its original device. Depending on the use-case and if needed, the
 * links may be dynamically changed by the driver, which allows it to control
 * the power to the PM domains independently from each other.
 *
 * Callers must ensure proper synchronization of this function with power
 * management callbacks.
 *
 * Returns the virtual created device when successfully attached to its PM
 * domain, NULL in case @dev don't need a PM domain, else an ERR_PTR().
 * Note that, to detach the returned virtual device, the driver shall call
 * dev_pm_domain_detach() on it, typically during the remove phase.
 */
struct device *genpd_dev_pm_attach_by_id(struct device *dev,
					 unsigned int index)
{
	struct device *virt_dev;
	int num_domains;
	int ret;

	if (!dev->of_node)
		return NULL;

	/* Verify that the index is within a valid range. */
	num_domains = of_count_phandle_with_args(dev->of_node, "power-domains",
						 "#power-domain-cells");
	if (index >= num_domains)
		return NULL;

	/* Allocate and register device on the genpd bus. */
	virt_dev = kzalloc(sizeof(*virt_dev), GFP_KERNEL);
	if (!virt_dev)
		return ERR_PTR(-ENOMEM);

	dev_set_name(virt_dev, "genpd");
	virt_dev->bus = &genpd_bus_type;
	virt_dev->parent = dev;
	virt_dev->of_node = dev->of_node;
	virt_dev->id = index;

	ret = device_register(virt_dev);
	if (ret) {
		kfree(dev);
		return ERR_PTR(ret);
	}

	/* Try to attach the device to the PM domain at the specified index. */
	ret = __genpd_dev_pm_attach(virt_dev, index, false);
	if (ret < 1) {
		device_unregister(virt_dev);
		return ret ? ERR_PTR(ret) : NULL;
	}

	return virt_dev;
}
EXPORT_SYMBOL_GPL(genpd_dev_pm_attach_by_id);

/**
 * genpd_dev_pm_attach_by_name - Associate a device with one of its PM domains.
 * @dev: The device used to lookup the PM domain.
 * @name: The name of the PM domain.
 *
 * Parse device's OF node to find a PM domain specifier using the
 * power-domain-names DT property. For further description see
 * genpd_dev_pm_attach_by_id().
 */
struct device *genpd_dev_pm_attach_by_name(struct device *dev, const char *name)
{
	int index;

	if (!dev->of_node)
		return NULL;

	index = of_property_match_string(dev->of_node, "power-domain-names",
					 name);
	if (index < 0)
		return NULL;

	return genpd_dev_pm_attach_by_id(dev, index);
}
EXPORT_SYMBOL_GPL(genpd_dev_pm_attach_by_name);

void pm_genpd_print(void)
{
	struct generic_pm_domain *genpd;

	printf("%-20s %6s\n", "name", "active");
	list_for_each_entry(genpd, &gpd_list, gpd_list_node)
		printf("%-20s %6s\n", genpd->name,
		       genpd->status == GPD_STATE_ACTIVE ? "on" : "off");
}
