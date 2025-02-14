// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2018, Tuomas Tynkkynen <tuomas.tynkkynen@iki.fi>
 * Copyright (C) 2018, Bin Meng <bmeng.cn@gmail.com>
 */

#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <net.h>
#include <init.h>
#include <linux/virtio.h>
#include <linux/virtio_ring.h>
#include <uapi/linux/virtio_net.h>

/* Amount of buffers to keep in the RX virtqueue */
#define VIRTIO_NET_NUM_RX_BUFS	32

/*
 * This value comes from the VirtIO spec: 1500 for maximum packet size,
 * 14 for the Ethernet header, 12 for virtio_net_hdr. In total 1526 bytes.
 */
#define VIRTIO_NET_RX_BUF_SIZE	1526

struct virtio_net_priv {
	union {
		struct virtqueue *vqs[2];
		struct {
			struct virtqueue *rx_vq;
			struct virtqueue *tx_vq;
		};
	};

	char rx_buff[VIRTIO_NET_NUM_RX_BUFS][VIRTIO_NET_RX_BUF_SIZE];
	bool rx_running;
	int net_hdr_len;
	struct eth_device edev;
	struct virtio_device *vdev;
};

static inline struct virtio_net_priv *to_priv(struct eth_device *edev)
{
	return container_of(edev, struct virtio_net_priv, edev);
}

static int virtio_net_start(struct eth_device *edev)
{
	struct virtio_net_priv *priv = to_priv(edev);
	struct scatterlist sg;
	int i;

	if (!priv->rx_running) {

		/* setup the receive buffer address */
		for (i = 0; i < VIRTIO_NET_NUM_RX_BUFS; i++) {
			/* receive buffer length is always 1526 */
			sg.length = VIRTIO_NET_RX_BUF_SIZE;

			sg_init_one(&sg, priv->rx_buff[i], VIRTIO_NET_RX_BUF_SIZE);
			virtqueue_add_inbuf(priv->rx_vq, &sg, 1);
		}

		virtqueue_kick(priv->rx_vq);

		/* setup the receive queue only once */
		priv->rx_running = true;
	}

	return 0;
}

static int virtio_net_send(struct eth_device *edev, void *packet, int length)
{
	struct virtio_net_priv *priv = to_priv(edev);
	struct virtio_net_hdr_v1 hdr_v1 = {};
	struct virtio_net_hdr hdr = {};
	struct scatterlist sgs[2];
	int ret;

	sg_init_table(sgs, ARRAY_SIZE(sgs));

	if (priv->net_hdr_len == sizeof(struct virtio_net_hdr))
		sg_set_buf(&sgs[0], &hdr, priv->net_hdr_len);
	else
		sg_set_buf(&sgs[0], &hdr_v1, priv->net_hdr_len);

	sg_set_buf(&sgs[1], packet, length);

	ret = virtqueue_add_outbuf(priv->tx_vq, sgs, ARRAY_SIZE(sgs));
	if (ret)
		return ret;

	virtqueue_kick(priv->tx_vq);

	while (1) {
		if (virtqueue_get_buf(priv->tx_vq, NULL))
			break;
	}

	return 0;
}

static void virtio_net_recv(struct eth_device *edev)
{
	struct virtio_net_priv *priv = to_priv(edev);
	struct scatterlist sg;
	unsigned int len;
	void *buf, *addr;

	addr = virtqueue_get_buf(priv->rx_vq, &len);
	if (!addr)
		return;

	sg_init_one(&sg, addr, VIRTIO_NET_RX_BUF_SIZE);

	buf = sg.address + priv->net_hdr_len;
	len -= priv->net_hdr_len;

	net_receive(edev, buf, len);

	/* Put the buffer back to the rx ring */
	virtqueue_add_inbuf(priv->rx_vq, &sg, 1);
}

static void virtio_net_stop(struct eth_device *dev)
{
	/*
	 * There is no way to stop the queue from running, unless we issue
	 * a reset to the virtio device, and re-do the queue initialization
	 * from the beginning.
	 */
}

static int virtio_net_read_rom_hwaddr(struct eth_device *edev, unsigned char *adr)
{
	struct virtio_net_priv *priv = to_priv(edev);

	virtio_cread_bytes(priv->vdev, offsetof(struct virtio_net_config, mac), adr, 6);

	return 0;
}

static int virtio_net_write_hwaddr(struct eth_device *edev, const unsigned char *adr)
{
	struct virtio_net_priv *priv = to_priv(edev);
	int i, ret;

	/*
	 * v1.0 compliant device's MAC address is set through control channel,
	 * which we don't support for now.
	 */
	if (virtio_has_feature(priv->vdev, VIRTIO_F_VERSION_1)) {
		char mac[6];

		ret = virtio_net_read_rom_hwaddr(edev, mac);
		if (ret)
			return ret;

		if (!memcmp(mac, adr, 5))
			return 0;
		else
			return -ENOSYS;
	}

	for (i = 0; i < 6; i++)
		virtio_cwrite8(priv->vdev, offsetof(struct virtio_net_config, mac) + i, adr[i]);

	return 0;
}

static int virtio_net_probe(struct virtio_device *vdev)
{
	struct virtio_net_priv *priv;
	struct eth_device *edev;
	int ret;

	priv = xzalloc(sizeof(*priv));

	vdev->priv = priv;

	/*
	 * For v1.0 compliant device, it always assumes the member
	 * 'num_buffers' exists in the struct virtio_net_hdr while
	 * the legacy driver only presented 'num_buffers' when
	 * VIRTIO_NET_F_MRG_RXBUF was negotiated. Without that feature
	 * the structure was 2 bytes shorter.
	 */
	if (virtio_has_feature(vdev, VIRTIO_F_VERSION_1))
		priv->net_hdr_len = sizeof(struct virtio_net_hdr_v1);
	else
		priv->net_hdr_len = sizeof(struct virtio_net_hdr);

	ret = virtio_find_vqs(vdev, 2, priv->vqs);
	if (ret < 0)
		return ret;

	priv->vdev = vdev;

	edev = &priv->edev;
	edev->priv = priv;
	edev->parent = &vdev->dev;

	edev->open = virtio_net_start;
	edev->send = virtio_net_send;
	edev->recv = virtio_net_recv;
	edev->halt = virtio_net_stop;
	edev->get_ethaddr = virtio_net_read_rom_hwaddr;
	edev->set_ethaddr = virtio_net_write_hwaddr;

	return eth_register(edev);
}

static void virtio_net_remove(struct virtio_device *vdev)
{
	struct virtio_net_priv *priv = vdev->priv;

	vdev->config->reset(vdev);
	eth_unregister(&priv->edev);
	vdev->config->del_vqs(vdev);

	free(priv);
}

/*
 * For simplicity, the driver only negotiates the VIRTIO_NET_F_MAC feature.
 * For the VIRTIO_NET_F_STATUS feature, we don't negotiate it, hence per spec
 * we should assume the link is always active.
 */
static const u32 features[] = {
	VIRTIO_NET_F_MAC
};

static const struct virtio_device_id id_table[] = {
        { VIRTIO_ID_NET, VIRTIO_DEV_ANY_ID },
        { 0 },
};

static struct virtio_driver virtio_net = {
        .driver.name	= "virtio_net",
        .id_table	= id_table,
        .probe		= virtio_net_probe,
	.remove		= virtio_net_remove,
	.feature_table			= features,
	.feature_table_size		= ARRAY_SIZE(features),
	.feature_table_legacy		= features,
	.feature_table_size_legacy	= ARRAY_SIZE(features),
};
device_virtio_driver(virtio_net);
