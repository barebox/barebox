// SPDX-License-Identifier: GPL-2.0-or-later

#include "realtek.h"

int realtek_dsa_init_tagger(struct realtek_priv *priv)
{
	const struct dsa_device_ops *tagger_ops;
	struct dsa_switch_ops *ops;

	/* TODO: Tagging can be configured per port in Linux. barebox DSA core
	 * will need some refactoring to do that. For now we just use the
	 * Linux default and leave ->change_tag_protocol unused and
	 * dsa-tag-protocol OF properties unheeded.
	 */
	switch (priv->ops->get_tag_protocol(priv)) {
	case DSA_TAG_PROTO_RTL4_A:
		tagger_ops = &rtl4a_netdev_ops;
		break;
	case DSA_TAG_PROTO_RTL8_4:
		tagger_ops = &rtl8_4_netdev_ops;
		break;
	case DSA_TAG_PROTO_RTL8_4T:
		tagger_ops = &rtl8_4t_netdev_ops;
		break;
	default:
		return -EINVAL;
	}

	ops = memdup(priv->ds->ops, sizeof(*priv->ds->ops));
	ops->xmit = tagger_ops->xmit;
	ops->rcv = tagger_ops->rcv;
	priv->ds->ops = ops;
	priv->ds->needed_headroom = tagger_ops->needed_headroom;
	priv->ds->needed_rx_tailroom = tagger_ops->needed_tailroom;
	priv->ds->needed_tx_tailroom = tagger_ops->needed_tailroom;

	return 0;
}
