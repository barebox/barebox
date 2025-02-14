// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Randomness driver for virtio
 *  Copyright (C) 2007, 2008 Rusty Russell IBM Corporation
 */

#include <common.h>
#include <linux/err.h>
#include <linux/hw_random.h>
#include <linux/spinlock.h>
#include <linux/virtio.h>
#include <linux/virtio_rng.h>
#include <linux/virtio_ring.h>
#include <module.h>
#include <linux/slab.h>

#define BUFFER_SIZE		16UL

struct virtrng_info {
	struct hwrng hwrng;
	char name[25];
	struct virtqueue *rng_vq;
	bool hwrng_register_done;
};

static inline struct virtrng_info *to_virtrng_info(struct hwrng *hwrng)
{
	return container_of(hwrng, struct virtrng_info, hwrng);
}

static int virtio_rng_read(struct hwrng *hwrng, void *data, size_t len, bool wait)
{
	int ret;
	unsigned int rsize;
	unsigned char buf[BUFFER_SIZE] __aligned(8);
	unsigned char *ptr = data;
	struct scatterlist sg;
	struct virtrng_info *vi = to_virtrng_info(hwrng);
	size_t remaining = len;

	while (remaining) {
		sg_init_one(&sg, buf, min(remaining, sizeof(buf)));

		ret = virtqueue_add_inbuf(vi->rng_vq, &sg, 1);
		if (ret)
			return ret;

		virtqueue_kick(vi->rng_vq);

		while (!virtqueue_get_buf(vi->rng_vq, &rsize))
			;

		memcpy(ptr, buf, rsize);
		remaining -= rsize;
		ptr += rsize;
	}

	return len;
}

static int virtrng_probe(struct virtio_device *vdev)
{
	struct virtrng_info *vi;

	vi = xzalloc(sizeof(*vi));

	vi->hwrng.name = vdev->dev.name;
	vi->hwrng.read = virtio_rng_read;

	vdev->priv = vi;

	/* We expect a single virtqueue. */
	return virtio_find_vqs(vdev, 1, &vi->rng_vq);
}

static void virtrng_remove(struct virtio_device *vdev)
{
	struct virtrng_info *vi = vdev->priv;

	vdev->config->reset(vdev);
	if (vi->hwrng_register_done)
		hwrng_unregister(&vi->hwrng);
	vdev->config->del_vqs(vdev);

	kfree(vi);
}

static void virtrng_scan(struct virtio_device *vdev)
{
	struct virtrng_info *vi = vdev->priv;
	int err;

	err = hwrng_register(&vdev->dev, &vi->hwrng);
	if (!err)
		vi->hwrng_register_done = true;
}

static const struct virtio_device_id id_table[] = {
	{ VIRTIO_ID_RNG, VIRTIO_DEV_ANY_ID },
	{ 0 },
};

static struct virtio_driver virtio_rng_driver = {
	.driver.name =	"virtio-rng",
	.id_table =	id_table,
	.probe =	virtrng_probe,
	.remove =	virtrng_remove,
	.scan =		virtrng_scan,
};

module_virtio_driver(virtio_rng_driver);
MODULE_DESCRIPTION("Virtio random number driver");
MODULE_LICENSE("GPL");
