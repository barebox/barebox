/*
 * Multiplexed I2C bus driver.
 *
 * Copyright (c) 2008-2009 Rodolfo Giometti <giometti@linux.it>
 * Copyright (c) 2008-2009 Eurotech S.p.A. <info@eurotech.it>
 * Copyright (c) 2009-2010 NSN GmbH & Co KG <michael.lawnick.ext@nsn.com>
 *
 * Ported to barebox from linux-v4.4-rc1
 * Copyright (C) 2015 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * Simplifies access to complex multiplexed I2C bus topologies, by presenting
 * each multiplexed bus segment as an additional I2C adapter.
 * Supports multi-level mux'ing (mux behind a mux).
 *
 * Based on:
 *	i2c-virt.c from Kumar Gala <galak@kernel.crashing.org>
 *	i2c-virtual.c from Ken Harrenstien, Copyright (c) 2004 Google, Inc.
 *	i2c-virtual.c from Brian Kuschak <bkuschak@yahoo.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <common.h>
#include <malloc.h>
#include <xfuncs.h>
#include <of.h>
#include <i2c/i2c.h>
#include <i2c/i2c-mux.h>

/* multiplexer per channel data */
struct i2c_mux_priv {
	struct i2c_adapter adap;

	struct i2c_adapter *parent;
	struct device_d *mux_dev;
	void *mux_priv;
	u32 chan_id;

	int (*select)(struct i2c_adapter *, void *mux_priv, u32 chan_id);
	int (*deselect)(struct i2c_adapter *, void *mux_priv, u32 chan_id);
};

static int i2c_mux_master_xfer(struct i2c_adapter *adap,
			       struct i2c_msg msgs[], int num)
{
	struct i2c_mux_priv *priv = adap->algo_data;
	struct i2c_adapter *parent = priv->parent;
	int ret;

	/* Switch to the right mux port and perform the transfer. */

	ret = priv->select(parent, priv->mux_priv, priv->chan_id);
	if (ret >= 0)
		ret = parent->master_xfer(parent, msgs, num);
	if (priv->deselect)
		priv->deselect(parent, priv->mux_priv, priv->chan_id);

	return ret;
}

struct i2c_adapter *i2c_add_mux_adapter(struct i2c_adapter *parent,
				struct device_d *mux_dev,
				void *mux_priv, u32 force_nr, u32 chan_id,
				int (*select) (struct i2c_adapter *,
					       void *, u32),
				int (*deselect) (struct i2c_adapter *,
						 void *, u32))
{
	struct i2c_mux_priv *priv;
	int ret;

	priv = kzalloc(sizeof(struct i2c_mux_priv), GFP_KERNEL);
	if (!priv)
		return NULL;

	/* Set up private adapter data */
	priv->parent = parent;
	priv->mux_dev = mux_dev;
	priv->mux_priv = mux_priv;
	priv->chan_id = chan_id;
	priv->select = select;
	priv->deselect = deselect;

	/* Need to do algo dynamically because we don't know ahead
	 * of time what sort of physical adapter we'll be dealing with.
	 */
	if (parent->master_xfer)
		priv->adap.master_xfer = i2c_mux_master_xfer;

	/* Now fill out new adapter structure */
	priv->adap.algo_data = priv;
	priv->adap.dev.parent = &parent->dev;
	priv->adap.retries = parent->retries;

	/*
	 * Try to populate the mux adapter's device_node, expands to
	 * nothing if !CONFIG_OF.
	 */
	if (mux_dev->device_node) {
		struct device_node *child;
		u32 reg;

		for_each_child_of_node(mux_dev->device_node, child) {
			ret = of_property_read_u32(child, "reg", &reg);
			if (ret)
				continue;
			if (chan_id == reg) {
				priv->adap.dev.device_node = child;
				break;
			}
		}
	}

	if (force_nr) {
		priv->adap.nr = force_nr;
	} else {
		priv->adap.nr = -1;
	}

	ret = i2c_add_numbered_adapter(&priv->adap);
	if (ret < 0) {
		dev_err(&parent->dev,
			"failed to add mux-adapter (error=%d)\n",
			ret);
		kfree(priv);
		return NULL;
	}

	dev_info(&parent->dev, "Added multiplexed i2c bus %d\n",
		 i2c_adapter_id(&priv->adap));

	return &priv->adap;
}
EXPORT_SYMBOL_GPL(i2c_add_mux_adapter);

void i2c_del_mux_adapter(struct i2c_adapter *adap)
{
	struct i2c_mux_priv *priv = adap->algo_data;

	free(priv);
}
EXPORT_SYMBOL_GPL(i2c_del_mux_adapter);

MODULE_AUTHOR("Rodolfo Giometti <giometti@linux.it>");
MODULE_DESCRIPTION("I2C driver for multiplexed I2C busses");
MODULE_LICENSE("GPL v2");
