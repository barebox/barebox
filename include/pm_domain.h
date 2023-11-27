/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _PM_DOMAIN_H
#define _PM_DOMAIN_H

#include <linux/list.h>
#include <of.h>

enum gpd_status {
	GPD_STATE_ACTIVE = 0,	/* PM domain is active */
	GPD_STATE_POWER_OFF,	/* PM domain is off */
};

struct generic_pm_domain {
	const char *name;
	struct list_head gpd_list_node;	/* Node in the global PM domains list */

	enum gpd_status status;	/* Current state of the domain */

	int (*power_off)(struct generic_pm_domain *domain);
	int (*power_on)(struct generic_pm_domain *domain);
};

typedef struct generic_pm_domain *(*genpd_xlate_t)(struct of_phandle_args *args,
						   void *data);

#ifdef CONFIG_PM_GENERIC_DOMAINS

void genpd_activate(void);

int genpd_dev_pm_attach(struct device *dev);

/**
 * dev_pm_domain_attach - Attach a device to its PM domain.
 * @dev: Device to attach.
 * @power_on: Used to indicate whether we should power on the device.
 *
 * The @dev may only be attached to a single PM domain. By iterating through
 * the available alternatives we try to find a valid PM domain for the device.
 * As attachment succeeds, the ->detach() callback in the struct dev_pm_domain
 * should be assigned by the corresponding attach function.
 *
 * This function should typically be invoked from subsystem level code during
 * the probe phase. Especially for those that holds devices which requires
 * power management through PM domains.
 *
 * Callers must ensure proper synchronization of this function with power
 * management callbacks.
 *
 * Returns 0 on successfully attached PM domain or negative error code.
 */
static inline int dev_pm_domain_attach(struct device *dev, bool power_on)
{
	return genpd_dev_pm_attach(dev);
}

int pm_genpd_init(struct generic_pm_domain *genpd, void *gov, bool is_off);

int of_genpd_add_provider_simple(struct device_node *np,
				 struct generic_pm_domain *genpd);

struct genpd_onecell_data {
	struct generic_pm_domain **domains;
	unsigned int num_domains;
	genpd_xlate_t xlate;
};

int of_genpd_add_provider_onecell(struct device_node *np,
				  struct genpd_onecell_data *data);

void pm_genpd_print(void);

#else

static inline void genpd_activate(void)
{
}

static inline int pm_genpd_init(struct generic_pm_domain *genpd,
				void *gov, bool is_off)
{
	return -ENOSYS;
}

static inline int genpd_dev_pm_attach(struct device *dev)
{
	return 0;
}

static inline int dev_pm_domain_attach(struct device *dev, bool power_on)
{
	return 0;
}

static inline int
of_genpd_add_provider_simple(struct device_node *np,
			     struct generic_pm_domain *genpd)
{
	return -ENOTSUPP;
}

#endif

#endif
