// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Virtio memory mapped device driver
 *
 * Copyright 2011-2014, ARM Ltd.
 *
 * This module allows virtio devices to be used over a virtual, memory mapped
 * platform device.
 *
 * The guest device(s) may be instantiated via Device Tree node, eg.:
 *
 *		virtio_block@1e000 {
 *			compatible = "virtio,mmio";
 *			reg = <0x1e000 0x100>;
 *			interrupts = <42>;
 *		}
 *
 * Qemu will automatically fix up the nodes corresponding to its command line
 * arguments into the barebox device tree.
 *
 * Based on Virtio PCI driver by Anthony Liguori, copyright IBM Corp. 2007
 */

#define pr_fmt(fmt) "virtio-mmio: " fmt

#include <common.h>
#include <io.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <driver.h>
#include <linux/slab.h>
#include <linux/virtio.h>
#include <linux/virtio_config.h>
#include <uapi/linux/virtio_mmio.h>
#include <linux/virtio_ring.h>

#define to_virtio_mmio_device(_plat_dev) \
	container_of(_plat_dev, struct virtio_mmio_device, vdev)

#define VIRTIO_MMIO_VRING_ALIGN		PAGE_SIZE

struct virtio_mmio_device {
	struct virtio_device vdev;

	void __iomem *base;
	unsigned long version;
};

struct virtio_mmio_vq_info {
	/* the actual virtqueue */
	struct virtqueue *vq;
};

static int virtio_mmio_get_config(struct virtio_device *vdev, unsigned int offset,
				  void *buf, unsigned int len)
{
	struct virtio_mmio_device *priv = to_virtio_mmio_device(vdev);
	void __iomem *base = priv->base + VIRTIO_MMIO_CONFIG;
	u8 b;
	__le16 w;
	__le32 l;

	if (priv->version == 1) {
		u8 *ptr = buf;
		int i;

		for (i = 0; i < len; i++)
			ptr[i] = readb(base + offset + i);

		return 0;
	}

	switch (len) {
	case 1:
		b = readb(base + offset);
		memcpy(buf, &b, sizeof(b));
		break;
	case 2:
		w = cpu_to_le16(readw(base + offset));
		memcpy(buf, &w, sizeof(w));
		break;
	case 4:
		l = cpu_to_le32(readl(base + offset));
		memcpy(buf, &l, sizeof(l));
		break;
	case 8:
		l = cpu_to_le32(readl(base + offset));
		memcpy(buf, &l, sizeof(l));
		l = cpu_to_le32(readl(base + offset + sizeof(l)));
		memcpy(buf + sizeof(l), &l, sizeof(l));
		break;
	default:
		WARN_ON(true);
	}

	return 0;
}

static int virtio_mmio_set_config(struct virtio_device *vdev, unsigned int offset,
				  const void *buf, unsigned int len)
{
	struct virtio_mmio_device *priv = to_virtio_mmio_device(vdev);
	void __iomem *base = priv->base + VIRTIO_MMIO_CONFIG;
	u8 b;
	__le16 w;
	__le32 l;

	if (priv->version == 1) {
		const u8 *ptr = buf;
		int i;

		for (i = 0; i < len; i++)
			writeb(ptr[i], base + offset + i);

		return 0;
	}

	switch (len) {
	case 1:
		memcpy(&b, buf, sizeof(b));
		writeb(b, base + offset);
		break;
	case 2:
		memcpy(&w, buf, sizeof(w));
		writew(le16_to_cpu(w), base + offset);
		break;
	case 4:
		memcpy(&l, buf, sizeof(l));
		writel(le32_to_cpu(l), base + offset);
		break;
	case 8:
		memcpy(&l, buf, sizeof(l));
		writel(le32_to_cpu(l), base + offset);
		memcpy(&l, buf + sizeof(l), sizeof(l));
		writel(le32_to_cpu(l), base + offset + sizeof(l));
		break;
	default:
		WARN_ON(true);
	}

	return 0;
}

static u32 virtio_mmio_generation(struct virtio_device *vdev)
{
	struct virtio_mmio_device *priv = to_virtio_mmio_device(vdev);

	if (priv->version == 1)
		return 0;

	return readl(priv->base + VIRTIO_MMIO_CONFIG_GENERATION);
}

static int virtio_mmio_get_status(struct virtio_device *vdev)
{
	struct virtio_mmio_device *priv = to_virtio_mmio_device(vdev);

	return readl(priv->base + VIRTIO_MMIO_STATUS) & 0xff;
}

static int virtio_mmio_set_status(struct virtio_device *vdev, u8 status)
{
	struct virtio_mmio_device *priv = to_virtio_mmio_device(vdev);

	/* We should never be setting status to 0 */
	WARN_ON(status == 0);

	writel(status, priv->base + VIRTIO_MMIO_STATUS);

	return 0;
}

static int virtio_mmio_reset(struct virtio_device *vdev)
{
	struct virtio_mmio_device *priv = to_virtio_mmio_device(vdev);

	/* 0 status means a reset */
	writel(0, priv->base + VIRTIO_MMIO_STATUS);

	return 0;
}

static u64 virtio_mmio_get_features(struct virtio_device *vdev)
{
	struct virtio_mmio_device *priv = to_virtio_mmio_device(vdev);
	u64 features;

	writel(1, priv->base + VIRTIO_MMIO_DEVICE_FEATURES_SEL);
	features = readl(priv->base + VIRTIO_MMIO_DEVICE_FEATURES);
	features <<= 32;

	writel(0, priv->base + VIRTIO_MMIO_DEVICE_FEATURES_SEL);
	features |= readl(priv->base + VIRTIO_MMIO_DEVICE_FEATURES);

	return features;
}

 static int virtio_mmio_finalize_features(struct virtio_device *vdev)
{
	struct virtio_mmio_device *priv = to_virtio_mmio_device(vdev);

	/* Make sure there is are no mixed devices */
	if (priv->version == 2 && !__virtio_test_bit(vdev, VIRTIO_F_VERSION_1)) {
		dev_err(&vdev->dev, "New virtio-mmio devices (version 2) must provide VIRTIO_F_VERSION_1 feature!\n");
		return -EINVAL;
	}

	writel(1, priv->base + VIRTIO_MMIO_DRIVER_FEATURES_SEL);
	writel((u32)(vdev->features >> 32),
	       priv->base + VIRTIO_MMIO_DRIVER_FEATURES);

	writel(0, priv->base + VIRTIO_MMIO_DRIVER_FEATURES_SEL);
	writel((u32)vdev->features,
	       priv->base + VIRTIO_MMIO_DRIVER_FEATURES);

	return 0;
}

static struct virtqueue *virtio_mmio_setup_vq(struct virtio_device *vdev,
					      unsigned int index)
{
	struct virtio_mmio_device *priv = to_virtio_mmio_device(vdev);
	struct virtqueue *vq;
	unsigned int num;
	int err;

	/* Select the queue we're interested in */
	writel(index, priv->base + VIRTIO_MMIO_QUEUE_SEL);

	/* Queue shouldn't already be set up */
	if (readl(priv->base + (priv->version == 1 ?
	    VIRTIO_MMIO_QUEUE_PFN : VIRTIO_MMIO_QUEUE_READY))) {
		err = -ENOENT;
		goto error_available;
	}

	num = readl(priv->base + VIRTIO_MMIO_QUEUE_NUM_MAX);
	if (num == 0) {
		err = -ENOENT;
		goto error_new_virtqueue;
	}

	/* Create the vring */
	vq = vring_create_virtqueue(index, num, VIRTIO_MMIO_VRING_ALIGN, vdev);
	if (!vq) {
		err = -ENOMEM;
		goto error_new_virtqueue;
	}

	/* Activate the queue */
	writel(virtqueue_get_vring_size(vq),
	       priv->base + VIRTIO_MMIO_QUEUE_NUM);
	if (priv->version == 1) {
		u64 q_pfn = virtqueue_get_desc_addr(vq) >> PAGE_SHIFT;

		/*
		 * virtio-mmio v1 uses a 32bit QUEUE PFN. If we have something
		 * that doesn't fit in 32bit, fail the setup rather than
		 * pretending to be successful.
		 */
		if (q_pfn >> 32) {
			debug("platform bug: legacy virtio-mmio must not be used with RAM above 0x%llxGB\n",
			      0x1ULL << (32 + PAGE_SHIFT - 30));
			err = -E2BIG;
			goto error_bad_pfn;
		}

		writel(PAGE_SIZE, priv->base + VIRTIO_MMIO_QUEUE_ALIGN);
		writel(q_pfn, priv->base + VIRTIO_MMIO_QUEUE_PFN);
	} else {
		u64 addr;

		addr = virtqueue_get_desc_addr(vq);
		writel((u32)addr, priv->base + VIRTIO_MMIO_QUEUE_DESC_LOW);
		writel((u32)(addr >> 32),
		       priv->base + VIRTIO_MMIO_QUEUE_DESC_HIGH);

		addr = virtqueue_get_avail_addr(vq);
		writel((u32)addr, priv->base + VIRTIO_MMIO_QUEUE_AVAIL_LOW);
		writel((u32)(addr >> 32),
		       priv->base + VIRTIO_MMIO_QUEUE_AVAIL_HIGH);

		addr = virtqueue_get_used_addr(vq);
		writel((u32)addr, priv->base + VIRTIO_MMIO_QUEUE_USED_LOW);
		writel((u32)(addr >> 32),
		       priv->base + VIRTIO_MMIO_QUEUE_USED_HIGH);

		writel(1, priv->base + VIRTIO_MMIO_QUEUE_READY);
	}

	return vq;

error_bad_pfn:
	vring_del_virtqueue(vq);

error_new_virtqueue:
	if (priv->version == 1) {
		writel(0, priv->base + VIRTIO_MMIO_QUEUE_PFN);
	} else {
		writel(0, priv->base + VIRTIO_MMIO_QUEUE_READY);
		WARN_ON(readl(priv->base + VIRTIO_MMIO_QUEUE_READY));
	}

error_available:
	return ERR_PTR(err);
}

static void virtio_mmio_del_vq(struct virtqueue *vq)
{
	struct virtio_mmio_device *priv = to_virtio_mmio_device(vq->vdev);
	unsigned int index = vq->index;

	/* Select and deactivate the queue */
	writel(index, priv->base + VIRTIO_MMIO_QUEUE_SEL);
	if (priv->version == 1) {
		writel(0, priv->base + VIRTIO_MMIO_QUEUE_PFN);
	} else {
		writel(0, priv->base + VIRTIO_MMIO_QUEUE_READY);
		WARN_ON(readl(priv->base + VIRTIO_MMIO_QUEUE_READY));
	}

	vring_del_virtqueue(vq);
}

static int virtio_mmio_del_vqs(struct virtio_device *vdev)
{
	struct virtqueue *vq, *n;

	list_for_each_entry_safe(vq, n, &vdev->vqs, list)
		virtio_mmio_del_vq(vq);

	return 0;
}

static int virtio_mmio_find_vqs(struct virtio_device *vdev, unsigned int nvqs,
				struct virtqueue *vqs[])
{
	int i;

	for (i = 0; i < nvqs; ++i) {
		vqs[i] = virtio_mmio_setup_vq(vdev, i);
		if (IS_ERR(vqs[i])) {
			virtio_mmio_del_vqs(vdev);
			return PTR_ERR(vqs[i]);
		}
	}

	return 0;
}

static int virtio_mmio_notify(struct virtio_device *vdev, struct virtqueue *vq)
{
	struct virtio_mmio_device *priv = to_virtio_mmio_device(vdev);

	/*
	 * We write the queue's selector into the notification register
	 * to signal the other end
	 */
	writel(vq->index, priv->base + VIRTIO_MMIO_QUEUE_NOTIFY);

	return 0;
}

static const struct virtio_config_ops virtio_mmio_config_ops = {
	.get_config	= virtio_mmio_get_config,
	.set_config	= virtio_mmio_set_config,
	.generation	= virtio_mmio_generation,
	.get_status	= virtio_mmio_get_status,
	.set_status	= virtio_mmio_set_status,
	.reset		= virtio_mmio_reset,
	.get_features	= virtio_mmio_get_features,
	.finalize_features	= virtio_mmio_finalize_features,
	.find_vqs	= virtio_mmio_find_vqs,
	.del_vqs	= virtio_mmio_del_vqs,
	.notify		= virtio_mmio_notify,
};


/* Platform device */

static int virtio_mmio_probe(struct device *dev)
{
	struct virtio_mmio_device *vm_dev;
	struct resource *res;
	unsigned long magic;

	vm_dev = kzalloc(sizeof(*vm_dev), GFP_KERNEL);
	if (!vm_dev)
		return -ENOMEM;

	vm_dev->vdev.dev.parent = dev;
	vm_dev->vdev.config = &virtio_mmio_config_ops;

	res = dev_request_mem_resource(dev, 0);
	if (IS_ERR(res))
		return PTR_ERR(res);

	vm_dev->base = IOMEM(res->start);

	/* Check magic value */
	magic = readl(vm_dev->base + VIRTIO_MMIO_MAGIC_VALUE);
	if (magic != ('v' | 'i' << 8 | 'r' << 16 | 't' << 24)) {
		dev_warn(dev, "Wrong magic value 0x%08lx!\n", magic);
		return -ENODEV;
	}

	/* Check device version */
	vm_dev->version = readl(vm_dev->base + VIRTIO_MMIO_VERSION);
	if (vm_dev->version < 1 || vm_dev->version > 2) {
		dev_err(dev, "Version %ld not supported!\n",
				vm_dev->version);
		return -ENXIO;
	}

	vm_dev->vdev.id.device = readl(vm_dev->base + VIRTIO_MMIO_DEVICE_ID);
	if (vm_dev->vdev.id.device == 0) {
		/*
		 * virtio-mmio device with an ID 0 is a (dummy) placeholder
		 * with no function. End probing now with no error reported.
		 */
		return -ENODEV;
	}
	vm_dev->vdev.id.vendor = readl(vm_dev->base + VIRTIO_MMIO_VENDOR_ID);

	if (vm_dev->version == 1)
		writel(PAGE_SIZE, vm_dev->base + VIRTIO_MMIO_GUEST_PAGE_SIZE);

	dev->priv = vm_dev;

	return register_virtio_device(&vm_dev->vdev);
}

static void virtio_mmio_remove(struct device *dev)
{
	struct virtio_mmio_device *vm_dev = dev->priv;
	unregister_virtio_device(&vm_dev->vdev);
}


/* Platform driver */

static const struct of_device_id virtio_mmio_match[] = {
	{ .compatible = "virtio,mmio", },
	{},
};
MODULE_DEVICE_TABLE(of, virtio_mmio_match);

static struct driver virtio_mmio_driver = {
	.probe		= virtio_mmio_probe,
	.remove		= virtio_mmio_remove,
	.name		= "virtio-mmio",
	.of_compatible	= virtio_mmio_match,
};

static int __init virtio_mmio_init(void)
{
	return platform_driver_register(&virtio_mmio_driver);
}

module_init(virtio_mmio_init);

MODULE_AUTHOR("Pawel Moll <pawel.moll@arm.com>");
MODULE_DESCRIPTION("Platform bus driver for memory mapped virtio devices");
MODULE_LICENSE("GPL");
