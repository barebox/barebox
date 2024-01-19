/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _PM_DOMAIN_H
#define _PM_DOMAIN_H

#include <linux/list.h>
#include <driver.h>
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
struct device *genpd_dev_pm_attach_by_id(struct device *dev,
					 unsigned int index);
struct device *genpd_dev_pm_attach_by_name(struct device *dev,
					   const char *name);

int pm_runtime_resume_and_get_genpd(struct device *dev);

int pm_genpd_init(struct generic_pm_domain *genpd, void *gov, bool is_off);

int pm_genpd_remove(struct generic_pm_domain *genpd);

int of_genpd_add_provider_simple(struct device_node *np,
				 struct generic_pm_domain *genpd);

struct genpd_onecell_data {
	struct generic_pm_domain **domains;
	unsigned int num_domains;
	genpd_xlate_t xlate;
};

int of_genpd_add_provider_onecell(struct device_node *np,
				  struct genpd_onecell_data *data);

void of_genpd_del_provider(struct device_node *np);

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

static inline int pm_genpd_remove(struct generic_pm_domain *genpd)
{
	return -EOPNOTSUPP;
}

static inline int genpd_dev_pm_attach(struct device *dev)
{
	return 0;
}

static inline struct device *genpd_dev_pm_attach_by_id(struct device *dev,
						       unsigned int index)
{
	return NULL;
}

static inline struct device *genpd_dev_pm_attach_by_name(struct device *dev,
							 const char *name)
{
	return NULL;
}

static inline int pm_runtime_resume_and_get_genpd(struct device *dev)
{
	return 0;
}

static inline int
of_genpd_add_provider_simple(struct device_node *np,
			     struct generic_pm_domain *genpd)
{
	return -ENOTSUPP;
}

static inline void of_genpd_del_provider(struct device_node *np)
{
}

#endif

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
	if (dev->pm_domain)
		return 0;

	return genpd_dev_pm_attach(dev);
}

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
static inline struct device *dev_pm_domain_attach_by_id(struct device *dev,
							unsigned int index)
{
	if (dev->pm_domain)
		return ERR_PTR(-EEXIST);

	return genpd_dev_pm_attach_by_id(dev, index);
}

/**
 * dev_pm_domain_attach_by_name - Associate a device with one of its PM domains.
 * @dev: The device used to lookup the PM domain.
 * @name: The name of the PM domain.
 *
 * For a detailed function description, see dev_pm_domain_attach_by_id().
 */
static inline struct device *dev_pm_domain_attach_by_name(struct device *dev,
							  const char *name)
{
	if (dev->pm_domain)
		return ERR_PTR(-EEXIST);

	return genpd_dev_pm_attach_by_name(dev, name);
}

static inline void dev_pm_domain_detach(struct device *dev, bool power_off)
{
	/* Just keep power domain enabled until dev_pm_domain_attach*
	 * start doing reference counting
	 */
}

static inline void pm_runtime_put_genpd(struct device *dev)
{
	/* Just keep power domain enabled until pm_runtime_resume_and_get_genpd
	 * starts doing reference counting
	 */
}
#endif
