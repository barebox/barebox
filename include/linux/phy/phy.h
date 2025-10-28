/*
 * phy.h -- generic phy header file
 *
 * Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com
 *
 * Author: Kishon Vijay Abraham I <kishon@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __DRIVERS_PHY_H
#define __DRIVERS_PHY_H

#include <linux/err.h>
#include <of.h>
#include <regulator.h>
#include <linux/phy/phy-mipi-dphy.h>

struct phy;

enum phy_mode {
	PHY_MODE_INVALID,
	PHY_MODE_USB_HOST,
	PHY_MODE_USB_HOST_LS,
	PHY_MODE_USB_HOST_FS,
	PHY_MODE_USB_HOST_HS,
	PHY_MODE_USB_HOST_SS,
	PHY_MODE_USB_DEVICE,
	PHY_MODE_USB_DEVICE_LS,
	PHY_MODE_USB_DEVICE_FS,
	PHY_MODE_USB_DEVICE_HS,
	PHY_MODE_USB_DEVICE_SS,
	PHY_MODE_USB_OTG,
	PHY_MODE_UFS_HS_A,
	PHY_MODE_UFS_HS_B,
	PHY_MODE_PCIE,
	PHY_MODE_ETHERNET,
	PHY_MODE_MIPI_DPHY,
	PHY_MODE_SATA,
	PHY_MODE_LVDS,
	PHY_MODE_DP
};

/**
 * union phy_configure_opts - Opaque generic phy configuration
 *
 * @mipi_dphy:	Configuration set applicable for phys supporting
 *		the MIPI_DPHY phy mode.
 */
union phy_configure_opts {
	struct phy_configure_opts_mipi_dphy	mipi_dphy;
};

/**
 * struct phy_ops - set of function pointers for performing phy operations
 * @init: operation to be performed for initializing phy
 * @exit: operation to be performed while exiting
 * @power_on: powering on the phy
 * @power_off: powering off the phy
 * @set_mode: set the mode of the phy
 * @owner: the module owner containing the ops
 */
struct phy_ops {
	int	(*init)(struct phy *phy);
	int	(*exit)(struct phy *phy);
	int	(*power_on)(struct phy *phy);
	int	(*power_off)(struct phy *phy);
	int	(*set_mode)(struct phy *phy, enum phy_mode mode, int submode);
	struct usb_phy *(*to_usbphy)(struct phy *phy);

	/**
	 * @configure:
	 *
	 * Optional.
	 *
	 * Used to change the PHY parameters. phy_init() must have
	 * been called on the phy.
	 *
	 * Returns: 0 if successful, an negative error code otherwise
	 */
	int	(*configure)(struct phy *phy, union phy_configure_opts *opts);
};

/**
 * struct phy_attrs - represents phy attributes
 * @bus_width: Data path width implemented by PHY
 * @mode: PHY mode
 */
struct phy_attrs {
	u32			bus_width;
	enum phy_mode		mode;
};

/**
 * struct phy - represents the phy device
 * @dev: phy device
 * @id: id of the phy device
 * @ops: function pointers for performing phy operations
 * @mutex: mutex to protect phy_ops
 * @init_count: used to protect when the PHY is used by multiple consumers
 * @power_count: used to protect when the PHY is used by multiple consumers
 * @phy_attrs: used to specify PHY specific attributes
 */
struct phy {
	struct device		dev;
	int			id;
	const struct phy_ops	*ops;
	int			init_count;
	int			power_count;
	struct phy_attrs	attrs;
	struct regulator	*pwr;
};

/**
 * struct phy_provider - represents the phy provider
 * @dev: phy provider device
 * @owner: the module owner having of_xlate
 * @of_xlate: function pointer to obtain phy instance from phy pointer
 * @list: to maintain a linked list of PHY providers
 */
struct phy_provider {
	struct device		*dev;
	struct list_head	list;
	struct phy * (*of_xlate)(struct device *dev,
				 const struct of_phandle_args *args);
};

/**
 * struct phy_consumer - represents the phy consumer
 * @dev_name: the device name of the controller that will use this PHY device
 * @port: name given to the consumer port
 */
struct phy_consumer {
	const char *dev_name;
	const char *port;
};

/**
 * struct phy_init_data - contains the list of PHY consumers
 * @num_consumers: number of consumers for this PHY device
 * @consumers: list of PHY consumers
 */
struct phy_init_data {
	unsigned int num_consumers;
	struct phy_consumer *consumers;
};

#define PHY_CONSUMER(_dev_name, _port)				\
{								\
	.dev_name	= _dev_name,				\
	.port		= _port,				\
}

#define	to_phy(dev)	(container_of((dev), struct phy, dev))

#define	of_phy_provider_register(dev, xlate)	\
	__of_phy_provider_register((dev), (xlate))


static inline void phy_set_drvdata(struct phy *phy, void *data)
{
	phy->dev.priv = data;
}

static inline void *phy_get_drvdata(struct phy *phy)
{
	return phy->dev.priv;
}

struct phy *of_phy_simple_xlate(struct device *dev,
				const struct of_phandle_args *args);

#if IS_ENABLED(CONFIG_GENERIC_PHY)
int phy_init(struct phy *phy);
int phy_exit(struct phy *phy);
int phy_power_on(struct phy *phy);
int phy_power_off(struct phy *phy);
int phy_set_mode_ext(struct phy *phy, enum phy_mode mode, int submode);
#define phy_set_mode(phy, mode) \
        phy_set_mode_ext(phy, mode, 0)
int phy_configure(struct phy *phy, union phy_configure_opts *opts);
static inline int phy_get_bus_width(struct phy *phy)
{
	return phy->attrs.bus_width;
}
static inline void phy_set_bus_width(struct phy *phy, int bus_width)
{
	phy->attrs.bus_width = bus_width;
}
struct phy *phy_get(struct device *dev, const char *string);
struct phy *phy_optional_get(struct device *dev, const char *string);
struct phy *of_phy_get_by_phandle(struct device *dev, const char *phandle,
				  u8 index);
void phy_put(struct phy *phy);
struct phy *of_phy_get(struct device_node *np, const char *con_id);
struct phy *phy_create(struct device *dev, struct device_node *node,
		       const struct phy_ops *ops);
void phy_destroy(struct phy *phy);
struct phy_provider *__of_phy_provider_register(struct device *dev,
						struct phy * (*of_xlate)(struct device *dev,
								const struct of_phandle_args *args));
void of_phy_provider_unregister(struct phy_provider *phy_provider);
struct usb_phy *phy_to_usbphy(struct phy *phy);
struct phy *phy_get_by_index(struct device *dev, int index);
#else
static inline int phy_init(struct phy *phy)
{
	if (!phy)
		return 0;
	return -ENOSYS;
}

static inline int phy_exit(struct phy *phy)
{
	if (!phy)
		return 0;
	return -ENOSYS;
}

static inline int phy_power_on(struct phy *phy)
{
	if (!phy)
		return 0;
	return -ENOSYS;
}

static inline int phy_power_off(struct phy *phy)
{
	if (!phy)
		return 0;
	return -ENOSYS;
}

static inline int phy_set_mode_ext(struct phy *phy, enum phy_mode mode,
				   int submode)
{
	if (!phy)
		return 0;
	return -ENOSYS;
}

#define phy_set_mode(phy, mode) \
        phy_set_mode_ext(phy, mode, 0)

static inline int phy_configure(struct phy *phy,
				union phy_configure_opts *opts)
{
	if (!phy)
		return 0;

	return -ENOSYS;
}

static inline int phy_get_bus_width(struct phy *phy)
{
	return -ENOSYS;
}

static inline void phy_set_bus_width(struct phy *phy, int bus_width)
{
	return;
}

static inline struct phy *phy_get(struct device *dev, const char *string)
{
	return ERR_PTR(-ENOSYS);
}

static inline struct phy *phy_optional_get(struct device *dev,
					   const char *string)
{
	return NULL;
}

static inline struct phy *of_phy_get_by_phandle(struct device *dev,
						const char *phandle, u8 index)
{
	return ERR_PTR(-ENOSYS);
}

static inline void phy_put(struct phy *phy)
{
}

static inline struct phy *of_phy_get(struct device_node *np, const char *con_id)
{
	return ERR_PTR(-ENOSYS);
}

static inline struct phy *phy_create(struct device *dev,
				     struct device_node *node,
				     const struct phy_ops *ops)
{
	return ERR_PTR(-ENOSYS);
}

static inline void phy_destroy(struct phy *phy)
{
}

static inline struct phy_provider *__of_phy_provider_register(struct device *dev,
							      struct phy * (*of_xlate)(struct device *dev, struct of_phandle_args *args))
{
	return ERR_PTR(-ENOSYS);
}

static inline void of_phy_provider_unregister(struct phy_provider *phy_provider)
{
}

static inline struct usb_phy *phy_to_usbphy(struct phy *phy)
{
	return NULL;
}

static inline struct phy *phy_get_by_index(struct device *dev, int index)
{
	return ERR_PTR(-ENODEV);
}

#endif

#endif /* __DRIVERS_PHY_H */
