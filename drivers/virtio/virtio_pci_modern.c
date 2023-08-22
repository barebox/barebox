// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2018, Bin Meng <bmeng.cn@gmail.com>
 *
 * VirtIO PCI bus transport driver
 * Ported from Linux drivers/virtio/virtio_pci*.c
 */

#include <common.h>
#include <linux/virtio_types.h>
#include <linux/virtio.h>
#include <linux/virtio_ring.h>
#include <linux/bug.h>
#include <linux/err.h>
#include <linux/pci.h>
#include <io.h>
#include "virtio_pci_common.h"

#define VIRTIO_PCI_DRV_NAME	"virtio-pci.m"

static int virtio_pci_get_config(struct virtio_device *vdev, unsigned int offset,
				 void *buf, unsigned int len)
{
	struct virtio_pci_device *vp_dev = to_vp_device(vdev);
	u8 b;
	__le16 w;
	__le32 l;

	BUG_ON(offset + len > vp_dev->device_len);

	switch (len) {
	case 1:
		b = ioread8(vp_dev->device + offset);
		memcpy(buf, &b, sizeof(b));
		break;
	case 2:
		w = cpu_to_le16(ioread16(vp_dev->device + offset));
		memcpy(buf, &w, sizeof(w));
		break;
	case 4:
		l = cpu_to_le32(ioread32(vp_dev->device + offset));
		memcpy(buf, &l, sizeof(l));
		break;
	case 8:
		l = cpu_to_le32(ioread32(vp_dev->device + offset));
		memcpy(buf, &l, sizeof(l));
		l = cpu_to_le32(ioread32(vp_dev->device + offset + sizeof(l)));
		memcpy(buf + sizeof(l), &l, sizeof(l));
		break;
	default:
		WARN_ON(true);
	}

	return 0;
}

static int virtio_pci_set_config(struct virtio_device *vdev, unsigned int offset,
				 const void *buf, unsigned int len)
{
	struct virtio_pci_device *vp_dev = to_vp_device(vdev);
	u8 b;
	__le16 w;
	__le32 l;

	WARN_ON(offset + len > vp_dev->device_len);

	switch (len) {
	case 1:
		memcpy(&b, buf, sizeof(b));
		iowrite8(b, vp_dev->device + offset);
		break;
	case 2:
		memcpy(&w, buf, sizeof(w));
		iowrite16(le16_to_cpu(w), vp_dev->device + offset);
		break;
	case 4:
		memcpy(&l, buf, sizeof(l));
		iowrite32(le32_to_cpu(l), vp_dev->device + offset);
		break;
	case 8:
		memcpy(&l, buf, sizeof(l));
		iowrite32(le32_to_cpu(l), vp_dev->device + offset);
		memcpy(&l, buf + sizeof(l), sizeof(l));
		iowrite32(le32_to_cpu(l), vp_dev->device + offset + sizeof(l));
		break;
	default:
		WARN_ON(true);
	}

	return 0;
}

static u32 virtio_pci_generation(struct virtio_device *vdev)
{
	struct virtio_pci_device *vp_dev = to_vp_device(vdev);

	return ioread8(&vp_dev->common->config_generation);
}

static int virtio_pci_get_status(struct virtio_device *vdev)
{
	struct virtio_pci_device *vp_dev = to_vp_device(vdev);

	return ioread8(&vp_dev->common->device_status);
}

static int virtio_pci_set_status(struct virtio_device *vdev, u8 status)
{
	struct virtio_pci_device *vp_dev = to_vp_device(vdev);

	/* We should never be setting status to 0 */
	WARN_ON(status == 0);

	iowrite8(status, &vp_dev->common->device_status);

	return 0;
}

static int virtio_pci_reset(struct virtio_device *vdev)
{
	struct virtio_pci_device *vp_dev = to_vp_device(vdev);

	/* 0 status means a reset */
	iowrite8(0, &vp_dev->common->device_status);

	/*
	 * After writing 0 to device_status, the driver MUST wait for a read
	 * of device_status to return 0 before reinitializing the device.
	 * This will flush out the status write, and flush in device writes,
	 * including MSI-X interrupts, if any.
	 */
	while (ioread8(&vp_dev->common->device_status))
		udelay(1000);

	return 0;
}

static u64 virtio_pci_get_features(struct virtio_device *vdev)
{
	struct virtio_pci_device *vp_dev = to_vp_device(vdev);
	u64 features;

	iowrite32(0, &vp_dev->common->device_feature_select);
	features = ioread32(&vp_dev->common->device_feature);
	iowrite32(1, &vp_dev->common->device_feature_select);
	features |= ((u64)ioread32(&vp_dev->common->device_feature) << 32);

	return features;
}

static int virtio_pci_set_features(struct virtio_device *vdev)
{
	struct virtio_pci_device *vp_dev = to_vp_device(vdev);

	if (!__virtio_test_bit(vdev, VIRTIO_F_VERSION_1)) {
		dev_dbg(&vdev->dev, "device uses modern interface but does not have VIRTIO_F_VERSION_1\n");
		return -EINVAL;
	}

	iowrite32(0, &vp_dev->common->guest_feature_select);
	iowrite32((u32)vdev->features, &vp_dev->common->guest_feature);
	iowrite32(1, &vp_dev->common->guest_feature_select);
	iowrite32(vdev->features >> 32, &vp_dev->common->guest_feature);

	return 0;
}

static struct virtqueue *virtio_pci_setup_vq(struct virtio_device *vdev,
					     unsigned int index)
{
	struct virtio_pci_device *vp_dev = to_vp_device(vdev);
	struct virtio_pci_common_cfg __iomem *cfg = vp_dev->common;
	struct virtqueue *vq;
	u16 num;
	u64 addr;
	int err;

	if (index >= ioread16(&cfg->num_queues))
		return ERR_PTR(-ENOENT);

	/* Select the queue we're interested in */
	iowrite16(index, &cfg->queue_select);

	/* Check if queue is either not available or already active */
	num = ioread16(&cfg->queue_size);
	if (!num || ioread16(&cfg->queue_enable))
		return ERR_PTR(-ENOENT);

	if (num & (num - 1)) {
		dev_warn(&vdev->dev, "bad queue size %u", num);
		return ERR_PTR(-EINVAL);
	}

	/* Create the vring */
	vq = vring_create_virtqueue(index, num, VIRTIO_PCI_VRING_ALIGN, vdev);
	if (!vq) {
		err = -ENOMEM;
		goto error_available;
	}

	/* Activate the queue */
	iowrite16(virtqueue_get_vring_size(vq), &cfg->queue_size);

	addr = virtqueue_get_desc_addr(vq);
	iowrite32((u32)addr, &cfg->queue_desc_lo);
	iowrite32(addr >> 32, &cfg->queue_desc_hi);

	addr = virtqueue_get_avail_addr(vq);
	iowrite32((u32)addr, &cfg->queue_avail_lo);
	iowrite32(addr >> 32, &cfg->queue_avail_hi);

	addr = virtqueue_get_used_addr(vq);
	iowrite32((u32)addr, &cfg->queue_used_lo);
	iowrite32(addr >> 32, &cfg->queue_used_hi);

	iowrite16(1, &cfg->queue_enable);

	return vq;

error_available:
	return ERR_PTR(err);
}

static void virtio_pci_del_vq(struct virtqueue *vq)
{
	struct virtio_pci_device *vp_dev = to_vp_device(vq->vdev);
	unsigned int index = vq->index;

	iowrite16(index, &vp_dev->common->queue_select);

	/* Select and deactivate the queue */
	iowrite16(0, &vp_dev->common->queue_enable);

	vring_del_virtqueue(vq);
}

static int virtio_pci_del_vqs(struct virtio_device *vdev)
{
	struct virtqueue *vq, *n;

	list_for_each_entry_safe(vq, n, &vdev->vqs, list)
		virtio_pci_del_vq(vq);

	return 0;
}

static int virtio_pci_find_vqs(struct virtio_device *vdev, unsigned int nvqs,
			       struct virtqueue *vqs[])
{
	int i;

	for (i = 0; i < nvqs; ++i) {
		vqs[i] = virtio_pci_setup_vq(vdev, i);
		if (IS_ERR(vqs[i])) {
			virtio_pci_del_vqs(vdev);
			return PTR_ERR(vqs[i]);
		}
	}

	return 0;
}

static int virtio_pci_notify(struct virtio_device *vdev, struct virtqueue *vq)
{
	struct virtio_pci_device *vp_dev = to_vp_device(vdev);
	u16 off;

	/* Select the queue we're interested in */
	iowrite16(vq->index, &vp_dev->common->queue_select);

	/* get offset of notification word for this vq */
	off = ioread16(&vp_dev->common->queue_notify_off);

	/*
	 * We write the queue's selector into the notification register
	 * to signal the other end
	 */
	iowrite16(vq->index,
		  vp_dev->notify_base + off * vp_dev->notify_offset_multiplier);

	return 0;
}

/**
 * virtio_pci_find_capability - walk capabilities to find device info
 *
 * @dev:	the PCI device
 * @cfg_type:	the VIRTIO_PCI_CAP_* value we seek
 *
 * @return offset of the configuration structure
 */
static int virtio_pci_find_capability(struct pci_dev *dev, u8 cfg_type)
{
	int pos;
	int offset;
	u8 type, bar;

	for (pos = pci_find_capability(dev, PCI_CAP_ID_VNDR);
	     pos > 0;
	     pos = pci_find_next_capability(dev, pos, PCI_CAP_ID_VNDR)) {
		offset = pos + offsetof(struct virtio_pci_cap, cfg_type);
		pci_read_config_byte(dev, offset, &type);
		offset = pos + offsetof(struct virtio_pci_cap, bar);
		pci_read_config_byte(dev, offset, &bar);

		/* Ignore structures with reserved BAR values */
		if (bar > 0x5)
			continue;

		if (type == cfg_type)
			return pos;
	}

	return 0;
}

/**
 * virtio_pci_map_capability - map base address of the capability
 *
 * @dev:	the PCI device
 * @off:	offset of the configuration structure
 *
 * @return base address of the capability
 */
static void __iomem *virtio_pci_map_capability(struct pci_dev *dev, int off)
{
	u32 offset;
	u8 bar;

	if (!off)
		return NULL;

	offset = off + offsetof(struct virtio_pci_cap, bar);
	pci_read_config_byte(dev, offset, &bar);
	offset = off + offsetof(struct virtio_pci_cap, offset);
	pci_read_config_dword(dev, offset, &offset);

        return pci_iomap(dev, bar) + offset;
}

static const struct virtio_config_ops virtio_pci_config_ops = {
	.get_config	= virtio_pci_get_config,
	.set_config	= virtio_pci_set_config,
	.generation	= virtio_pci_generation,
	.get_status	= virtio_pci_get_status,
	.set_status	= virtio_pci_set_status,
	.reset		= virtio_pci_reset,
	.get_features	= virtio_pci_get_features,
	.finalize_features	= virtio_pci_set_features,
	.find_vqs	= virtio_pci_find_vqs,
	.del_vqs	= virtio_pci_del_vqs,
	.notify		= virtio_pci_notify,
};

int virtio_pci_modern_probe(struct virtio_pci_device *vp_dev)
{
	struct pci_dev *pci_dev = vp_dev->pci_dev;
	struct device *dev = &pci_dev->dev;
	int common, notify, device;
	int offset;

	/* We only own devices >= 0x1000 and <= 0x107f: leave the rest. */
	if (pci_dev->device < 0x1000 || pci_dev->device > 0x107f)
		return -ENODEV;

	if (pci_dev->device < 0x1040) {
		/* Transitional devices: use the PCI subsystem device id as
		 * virtio device id, same as legacy driver always did.
		 */
		vp_dev->vdev.id.device = pci_dev->subsystem_device;
	} else {
		/* Modern devices: simply use PCI device id, but start from 0x1040. */
		vp_dev->vdev.id.device = pci_dev->device - 0x1040;
	}
	vp_dev->vdev.id.vendor = pci_dev->subsystem_vendor;

	/* Check for a common config: if not, driver could fall back to legacy mode (bar 0) */
	common = virtio_pci_find_capability(pci_dev, VIRTIO_PCI_CAP_COMMON_CFG);
	if (!common)
		return -ENODEV;

	/* If common is there, notify should be too */
	notify = virtio_pci_find_capability(pci_dev, VIRTIO_PCI_CAP_NOTIFY_CFG);
	if (!notify) {
		dev_warn(dev, "missing capabilities %i/%i\n", common, notify);
		return -EINVAL;
	}

	/*
	 * Device capability is only mandatory for devices that have
	 * device-specific configuration.
	 */
	device = virtio_pci_find_capability(pci_dev, VIRTIO_PCI_CAP_DEVICE_CFG);
	if (device) {
		offset = notify + offsetof(struct virtio_pci_cap, length);
		pci_read_config_dword(pci_dev, offset, &vp_dev->device_len);
	}

	/* Map configuration structures */
	vp_dev->common = virtio_pci_map_capability(pci_dev, common);
	vp_dev->notify_base = virtio_pci_map_capability(pci_dev, notify);
	vp_dev->device = virtio_pci_map_capability(pci_dev, device);
	dev_dbg(dev, "common @ %p, notify base @ %p, device @ %p\n",
		vp_dev->common, vp_dev->notify_base, vp_dev->device);

	/* Read notify_off_multiplier from config space */
	offset = notify + offsetof(struct virtio_pci_notify_cap,
				   notify_off_multiplier);
	pci_read_config_dword(pci_dev, offset, &vp_dev->notify_offset_multiplier);

	vp_dev->vdev.config = &virtio_pci_config_ops;

	return 0;
}
