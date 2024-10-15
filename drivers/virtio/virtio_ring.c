// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2018, Tuomas Tynkkynen <tuomas.tynkkynen@iki.fi>
 * Copyright (C) 2018, Bin Meng <bmeng.cn@gmail.com>
 *
 * virtio ring implementation
 */

#define pr_fmt(fmt) "virtio_ring: " fmt

#include <common.h>
#include <linux/virtio_config.h>
#include <linux/virtio_types.h>
#include <linux/virtio.h>
#include <linux/virtio_ring.h>
#include <linux/bug.h>
#include <dma.h>

#define vq_debug(vq, fmt, ...) \
	dev_dbg(&vq->vdev->dev, fmt, ##__VA_ARGS__)

#define vq_info(vq, fmt, ...) \
	dev_info(&vq->vdev->dev, fmt, ##__VA_ARGS__)

static inline struct device *vring_dma_dev(const struct virtqueue *vq)
{
	return vq->vdev->dev.parent;
}

/* Map one sg entry. */
static dma_addr_t vring_map_one_sg(struct virtqueue *vq,
				   struct virtio_sg *sg,
				   enum dma_data_direction direction)
{
	return dma_map_single(vring_dma_dev(vq), sg->addr, sg->length, direction);
}

static int vring_mapping_error(struct virtqueue *vq,
			       dma_addr_t addr)
{
	return dma_mapping_error(vring_dma_dev(vq), addr);
}

static void vring_unmap_one(struct virtqueue *vq,
			    struct vring_desc *desc)
{
	u16 flags;

	flags = virtio16_to_cpu(vq->vdev, desc->flags);

	dma_unmap_single(vring_dma_dev(vq),
		       virtio64_to_cpu(vq->vdev, desc->addr),
		       virtio32_to_cpu(vq->vdev, desc->len),
		       (flags & VRING_DESC_F_WRITE) ?
		       DMA_FROM_DEVICE : DMA_TO_DEVICE);
}

int virtqueue_add(struct virtqueue *vq, struct virtio_sg *sgs[],
		  unsigned int out_sgs, unsigned int in_sgs)
{
	struct vring_desc *desc;
	unsigned int total_sg = out_sgs + in_sgs;
	unsigned int i, err_idx, n, avail, descs_used, uninitialized_var(prev);
	int head;

	WARN_ON(total_sg == 0);

	head = vq->free_head;

	desc = vq->vring.desc;
	i = head;
	descs_used = total_sg;

	if (vq->num_free < descs_used) {
		vq_debug(vq, "Can't add buf len %i - avail = %i\n",
		      descs_used, vq->num_free);
		/*
		 * FIXME: for historical reasons, we force a notify here if
		 * there are outgoing parts to the buffer.  Presumably the
		 * host should service the ring ASAP.
		 */
		if (out_sgs)
			virtio_notify(vq->vdev, vq);
		return -ENOSPC;
	}

	for (n = 0; n < out_sgs; n++) {
		struct virtio_sg *sg = sgs[n];
		dma_addr_t addr = vring_map_one_sg(vq, sg, DMA_TO_DEVICE);
		if (vring_mapping_error(vq, addr))
			goto unmap_release;


		desc[i].flags = cpu_to_virtio16(vq->vdev, VRING_DESC_F_NEXT);
		desc[i].addr = cpu_to_virtio64(vq->vdev, addr);
		desc[i].len = cpu_to_virtio32(vq->vdev, sg->length);

		prev = i;
		i = virtio16_to_cpu(vq->vdev, desc[i].next);
	}
	for (; n < (out_sgs + in_sgs); n++) {
		struct virtio_sg *sg = sgs[n];
		dma_addr_t addr = vring_map_one_sg(vq, sg, DMA_FROM_DEVICE);
		if (vring_mapping_error(vq, addr))
			goto unmap_release;

		desc[i].flags = cpu_to_virtio16(vq->vdev, VRING_DESC_F_NEXT |
						VRING_DESC_F_WRITE);
		desc[i].addr = cpu_to_virtio64(vq->vdev, addr);
		desc[i].len = cpu_to_virtio32(vq->vdev, sg->length);

		prev = i;
		i = virtio16_to_cpu(vq->vdev, desc[i].next);
	}
	/* Last one doesn't continue */
	desc[prev].flags &= cpu_to_virtio16(vq->vdev, ~VRING_DESC_F_NEXT);

	/* We're using some buffers from the free list. */
	vq->num_free -= descs_used;

	/* Update free pointer */
	vq->free_head = i;

	/*
	 * Put entry in available array (but don't update avail->idx
	 * until they do sync).
	 */
	avail = vq->avail_idx_shadow & (vq->vring.num - 1);
	vq->vring.avail->ring[avail] = cpu_to_virtio16(vq->vdev, head);

	/*
	 * Descriptors and available array need to be set before we expose the
	 * new available array entries.
	 */
	virtio_wmb();
	vq->avail_idx_shadow++;
	vq->vring.avail->idx = cpu_to_virtio16(vq->vdev, vq->avail_idx_shadow);
	vq->num_added++;

	/*
	 * This is very unlikely, but theoretically possible.
	 * Kick just in case.
	 */
	if (unlikely(vq->num_added == (1 << 16) - 1))
		virtqueue_kick(vq);

	return 0;

unmap_release:
	err_idx = i;

	for (n = 0; n < total_sg; n++) {
		if (i == err_idx)
			break;
		vring_unmap_one(vq, &desc[i]);
		i = virtio16_to_cpu(vq->vdev, desc[i].next);
	}

	return -ENOMEM;

}

static bool virtqueue_kick_prepare(struct virtqueue *vq)
{
	u16 new, old;
	bool needs_kick;

	/*
	 * We need to expose available array entries before checking
	 * avail event.
	 */
	virtio_mb();

	old = vq->avail_idx_shadow - vq->num_added;
	new = vq->avail_idx_shadow;
	vq->num_added = 0;

	if (vq->event) {
		needs_kick = vring_need_event(virtio16_to_cpu(vq->vdev,
				vring_avail_event(&vq->vring)), new, old);
	} else {
		needs_kick = !(vq->vring.used->flags & cpu_to_virtio16(vq->vdev,
				VRING_USED_F_NO_NOTIFY));
	}

	return needs_kick;
}

void virtqueue_kick(struct virtqueue *vq)
{
	if (virtqueue_kick_prepare(vq))
		virtio_notify(vq->vdev, vq);
}

static void detach_buf(struct virtqueue *vq, unsigned int head)
{
	unsigned int i;
	__virtio16 nextflag = cpu_to_virtio16(vq->vdev, VRING_DESC_F_NEXT);

	/* Put back on free list: unmap first-level descriptors and find end */
	i = head;

	while (vq->vring.desc[i].flags & nextflag) {
		vring_unmap_one(vq, &vq->vring.desc[i]);
		i = virtio16_to_cpu(vq->vdev, vq->vring.desc[i].next);
		vq->num_free++;
	}

	vring_unmap_one(vq, &vq->vring.desc[i]);
	vq->vring.desc[i].next = cpu_to_virtio16(vq->vdev, vq->free_head);
	vq->free_head = head;

	/* Plus final descriptor */
	vq->num_free++;
}

static inline bool more_used(const struct virtqueue *vq)
{
	return virtqueue_poll(vq, vq->last_used_idx);
}

void *virtqueue_get_buf(struct virtqueue *vq, unsigned int *len)
{
	unsigned int i;
	u16 last_used;

	if (!more_used(vq)) {
		vq_debug(vq, "No more buffers in queue\n");
		return NULL;
	}

	/* Only get used array entries after they have been exposed by host */
	virtio_rmb();

	last_used = (vq->last_used_idx & (vq->vring.num - 1));
	i = virtio32_to_cpu(vq->vdev, vq->vring.used->ring[last_used].id);
	if (len) {
		*len = virtio32_to_cpu(vq->vdev,
				       vq->vring.used->ring[last_used].len);
		vq_debug(vq, "last used idx %u with len %u\n", i, *len);
	}

	if (unlikely(i >= vq->vring.num)) {
		vq_info(vq, "id %u out of range\n", i);
		return NULL;
	}

	detach_buf(vq, i);
	vq->last_used_idx++;
	/*
	 * If we expect an interrupt for the next entry, tell host
	 * by writing event index and flush out the write before
	 * the read in the next get_buf call.
	 */
	if (!(vq->avail_flags_shadow & VRING_AVAIL_F_NO_INTERRUPT))
		virtio_store_mb(&vring_used_event(&vq->vring),
				cpu_to_virtio16(vq->vdev, vq->last_used_idx));

	return IOMEM((uintptr_t)virtio64_to_cpu(vq->vdev,
						  vq->vring.desc[i].addr));
}

static struct virtqueue *__vring_new_virtqueue(unsigned int index,
					       struct vring vring,
					       struct virtio_device *vdev)
{
	unsigned int i;
	struct virtqueue *vq;

	vq = malloc(sizeof(*vq));
	if (!vq)
		return NULL;

	vq->vdev = vdev;
	vq->index = index;
	vq->num_free = vring.num;
	vq->vring = vring;
	vq->last_used_idx = 0;
	vq->avail_flags_shadow = 0;
	vq->avail_idx_shadow = 0;
	vq->num_added = 0;
	vq->queue_dma_addr = 0;
	vq->queue_size_in_bytes = 0;
	list_add_tail(&vq->list, &vdev->vqs);

	vq->event = virtio_has_feature(vdev, VIRTIO_RING_F_EVENT_IDX);

	/* Tell other side not to bother us */
	vq->avail_flags_shadow |= VRING_AVAIL_F_NO_INTERRUPT;
	if (!vq->event)
		vq->vring.avail->flags = cpu_to_virtio16(vdev,
				vq->avail_flags_shadow);

	/* Put everything in free lists */
	vq->free_head = 0;
	for (i = 0; i < vring.num - 1; i++)
		vq->vring.desc[i].next = cpu_to_virtio16(vdev, i + 1);

	return vq;
}

/*
 * Modern virtio devices have feature bits to specify whether they need a
 * quirk and bypass the IOMMU. If not there, just use the DMA API.
 *
 * If there, the interaction between virtio and DMA API is messy.
 *
 * On most systems with virtio, physical addresses match bus addresses,
 * and it _shouldn't_ particularly matter whether we use the DMA API.
 *
 * However, barebox' dma_alloc_coherent doesn't yet take a device pointer
 * as argument, so even for dma-coherent devices, the virtqueue is mapped
 * uncached on ARM. This has considerable impact on the Virt I/O performance,
 * so we really want to avoid using the DMA API if possible for the time being.
 *
 * On some systems, including Xen and any system with a physical device
 * that speaks virtio behind a physical IOMMU, we must use the DMA API
 * for virtio DMA to work at all.
 *
 * On other systems, including SPARC and PPC64, virtio-pci devices are
 * enumerated as though they are behind an IOMMU, but the virtio host
 * ignores the IOMMU, so we must either pretend that the IOMMU isn't
 * there or somehow map everything as the identity.
 *
 * For the time being, we preserve historic behavior and bypass the DMA
 * API.
 *
 * TODO: install a per-device DMA ops structure that does the right thing
 * taking into account all the above quirks, and use the DMA API
 * unconditionally on data path.
 */

static bool vring_use_dma_api(const struct virtio_device *vdev)
{
	return !virtio_has_dma_quirk(vdev);
}

static void *vring_alloc_queue(struct virtio_device *vdev,
			       size_t size, dma_addr_t *dma_handle)
{
	if (vring_use_dma_api(vdev)) {
		return dma_alloc_coherent(size, dma_handle);
	} else {
		void *queue = memalign(PAGE_SIZE, PAGE_ALIGN(size));

		if (queue) {
			phys_addr_t phys_addr = virt_to_phys(queue);
			*dma_handle = (dma_addr_t)phys_addr;

			/*
			 * Sanity check: make sure we dind't truncate
			 * the address.  The only arches I can find that
			 * have 64-bit phys_addr_t but 32-bit dma_addr_t
			 * are certain non-highmem MIPS and x86
			 * configurations, but these configurations
			 * should never allocate physical pages above 32
			 * bits, so this is fine.  Just in case, throw a
			 * warning and abort if we end up with an
			 * unrepresentable address.
			 */
			if (WARN_ON_ONCE(*dma_handle != phys_addr)) {
				free(queue);
				return NULL;
			}
		}
		return queue;
	}
}

static void vring_free_queue(struct virtio_device *vdev,
			     size_t size, void *queue, dma_addr_t dma_handle)
{
	if (vring_use_dma_api(vdev))
		dma_free_coherent(queue, dma_handle, size);
	else
		free(queue);
}

struct virtqueue *vring_create_virtqueue(unsigned int index, unsigned int num,
					 unsigned int vring_align,
					 struct virtio_device *vdev)
{
	struct virtqueue *vq;
	void *queue = NULL;
	dma_addr_t dma_addr;
	size_t queue_size_in_bytes;
	struct vring vring;

	/* We assume num is a power of 2 */
	if (num & (num - 1)) {
		pr_err("Bad virtqueue length %u\n", num);
		return NULL;
	}

	/* TODO: allocate each queue chunk individually */
	for (; num && vring_size(num, vring_align) > PAGE_SIZE; num /= 2) {
		queue = vring_alloc_queue(vdev, vring_size(num, vring_align), &dma_addr);
		if (queue)
			break;
	}

	if (!num)
		return NULL;

	if (!queue) {
		/* Try to get a single page. You are my only hope! */
		queue = vring_alloc_queue(vdev, vring_size(num, vring_align), &dma_addr);
	}
	if (!queue)
		return NULL;

	queue_size_in_bytes = vring_size(num, vring_align);
	vring_init(&vring, num, queue, vring_align);

	vq = __vring_new_virtqueue(index, vring, vdev);
	if (!vq) {
		vring_free_queue(vdev, queue_size_in_bytes, queue, dma_addr);
		return NULL;
	}
	vq_debug(vq, "created vring @ (virt=%p, phys=%pad) for vq with num %u\n",
		 queue, &dma_addr, num);

	vq->queue_dma_addr = dma_addr;
	vq->queue_size_in_bytes = queue_size_in_bytes;

	return vq;
}

void vring_del_virtqueue(struct virtqueue *vq)
{
	vring_free_queue(vq->vdev, vq->queue_size_in_bytes,
			 vq->vring.desc, vq->queue_dma_addr);
	list_del(&vq->list);
	free(vq);
}

unsigned int virtqueue_get_vring_size(struct virtqueue *vq)
{
	return vq->vring.num;
}

dma_addr_t virtqueue_get_desc_addr(struct virtqueue *vq)
{
	return vq->queue_dma_addr;
}

dma_addr_t virtqueue_get_avail_addr(struct virtqueue *vq)
{
	return vq->queue_dma_addr +
	       ((char *)vq->vring.avail - (char *)vq->vring.desc);
}

dma_addr_t virtqueue_get_used_addr(struct virtqueue *vq)
{
	return vq->queue_dma_addr +
	       ((char *)vq->vring.used - (char *)vq->vring.desc);
}

bool virtqueue_poll(const struct virtqueue *vq, u16 last_used_idx)
{
	virtio_mb();

	return last_used_idx != virtio16_to_cpu(vq->vdev, vq->vring.used->idx);
}

void virtqueue_dump(struct virtqueue *vq)
{
	unsigned int i;

	printf("virtqueue %p for dev %s:\n", vq, vq->vdev->dev.name);
	printf("\tindex %u, phys addr %p num %u\n",
	       vq->index, vq->vring.desc, vq->vring.num);
	printf("\tfree_head %u, num_added %u, num_free %u\n",
	       vq->free_head, vq->num_added, vq->num_free);
	printf("\tlast_used_idx %u, avail_flags_shadow %u, avail_idx_shadow %u\n",
	       vq->last_used_idx, vq->avail_flags_shadow, vq->avail_idx_shadow);

	printf("Descriptor dump:\n");
	for (i = 0; i < vq->vring.num; i++) {
		printf("\tdesc[%u] = { 0x%llx, len %u, flags %u, next %u }\n",
		       i, vq->vring.desc[i].addr, vq->vring.desc[i].len,
		       vq->vring.desc[i].flags, vq->vring.desc[i].next);
	}

	printf("Avail ring dump:\n");
	printf("\tflags %u, idx %u\n",
	       vq->vring.avail->flags, vq->vring.avail->idx);
	for (i = 0; i < vq->vring.num; i++) {
		printf("\tavail[%u] = %u\n",
		       i, vq->vring.avail->ring[i]);
	}

	printf("Used ring dump:\n");
	printf("\tflags %u, idx %u\n",
	       vq->vring.used->flags, vq->vring.used->idx);
	for (i = 0; i < vq->vring.num; i++) {
		printf("\tused[%u] = { %u, %u }\n", i,
		       vq->vring.used->ring[i].id, vq->vring.used->ring[i].len);
	}
}

int virtio_notify(struct virtio_device *vdev, struct virtqueue *vq)
{
	return vdev->config->notify(vdev, vq);
}
