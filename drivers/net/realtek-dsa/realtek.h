/* SPDX-License-Identifier: GPL-2.0+ */
/* Realtek SMI interface driver defines
 *
 * Copyright (C) 2017 Linus Walleij <linus.walleij@linaro.org>
 * Copyright (C) 2009-2010 Gabor Juhos <juhosg@openwrt.org>
 */

#ifndef _REALTEK_H
#define _REALTEK_H

#include <linux/spinlock.h>
#include <linux/phy.h>
#include <driver.h>
#include <gpio.h>
#include <dsa.h>

#define REALTEK_HW_STOP_DELAY		25	/* msecs */
#define REALTEK_HW_START_DELAY		100	/* msecs */

struct realtek_ops;

struct realtek_priv {
	struct device		*dev;
	struct gpio_desc	*reset;
	struct gpio_desc	*mdc;
	struct gpio_desc	*mdio;
	union {
		struct regmap		*map;
		struct regmap		*map_nolock;
	};
	struct mii_bus		slave_mii_bus[1];
	struct mii_bus		*bus;
	int			mdio_addr;

	unsigned int		clk_delay;
	u8			cmd_read;
	u8			cmd_write;
	spinlock_t		lock; /* Locks around command writes */
	struct dsa_switch	*ds;
	bool			leds_disabled;

	unsigned int		cpu_port;
	unsigned int		num_ports;

	const struct realtek_ops *ops;
	int			(*setup_interface)(struct dsa_switch *ds);
	int			(*write_reg_noack)(void *ctx, u32 addr, u32 data);

	char			buf[4096];
	void			*chip_data; /* Per-chip extra variant data */
};

/*
 * struct realtek_ops - vtable for the per-SMI-chiptype operations
 * @detect: detects the chiptype
 */
struct realtek_ops {
	int	(*detect)(struct realtek_priv *priv);
	int	(*reset_chip)(struct realtek_priv *priv);
	int	(*setup)(struct realtek_priv *priv);
	void	(*cleanup)(struct realtek_priv *priv);
	int	(*enable_port)(struct realtek_priv *priv, int port, bool enable);
	int	(*phy_read)(struct realtek_priv *priv, int phy, int regnum);
	int	(*phy_write)(struct realtek_priv *priv, int phy, int regnum,
			     u16 val);
	enum dsa_tag_protocol (*get_tag_protocol)(struct realtek_priv *priv);
	int (*change_tag_protocol)(struct realtek_priv *priv,
				   enum dsa_tag_protocol proto);
};

struct realtek_variant {
	const struct dsa_switch_ops *ds_ops;
	const struct realtek_ops *ops;
	unsigned int clk_delay;
	u8 cmd_read;
	u8 cmd_write;
	size_t chip_data_sz;
};

enum dsa_tag_protocol {
	DSA_TAG_PROTO_RTL4_A		= 17,
	DSA_TAG_PROTO_RTL8_4		= 24,
	DSA_TAG_PROTO_RTL8_4T		= 25,
};

struct dsa_device_ops {
	int (*xmit)(struct dsa_port *dp, int port, void *packet, int length);
	int (*rcv)(struct dsa_switch *ds, int *portp, void *packet, int length);
	unsigned int needed_headroom;
	unsigned int needed_tailroom;
	const char *name;
	enum dsa_tag_protocol proto;
};

extern const struct realtek_variant rtl8366rb_variant;
extern const struct realtek_variant rtl8365mb_variant;

int realtek_dsa_init_tagger(struct realtek_priv *priv);

extern const struct dsa_device_ops rtl4a_netdev_ops;
extern const struct dsa_device_ops rtl8_4_netdev_ops;
extern const struct dsa_device_ops rtl8_4t_netdev_ops;

#endif /*  _REALTEK_H */
