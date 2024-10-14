// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2015-2017, 2019-2021 Linaro Limited
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/device.h>
#include <linux/slab.h>
#include <linux/tee_drv.h>
#include <linux/uaccess.h>
#include <linux/sizes.h>
#include <fcntl.h>
#include <stdio.h>
#include "tee_private.h"

static void tee_shm_release(struct tee_device *teedev, struct tee_shm *shm)
{
	if (shm->flags & TEE_SHM_DYNAMIC)
		teedev->desc->ops->shm_unregister(shm->ctx, shm);

	if (!(shm->flags & TEE_SHM_PRIV)) {
		list_del(&shm->link);
		if (IS_ENABLED(CONFIG_OPTEE_DEVFS)) {
			devfs_remove(&shm->cdev);
			unregister_device(&shm->dev);
		}
	}

	if (shm->flags & TEE_SHM_POOL)
		free(shm->kaddr);

	teedev_ctx_put(shm->ctx);

	kfree(shm);

	tee_device_put(teedev);
}

static const struct cdev_operations tee_shm_ops = {
	.read = mem_read,
	.memmap = generic_memmap_ro,
};

static struct tee_shm *
register_shm_helper(struct tee_context *ctx, void *addr,
		    size_t size, u32 flags)
{
	struct tee_device *teedev = ctx->teedev;
	struct tee_shm *shm;
	int rc;

	if (!addr)
		return ERR_PTR(-ENOMEM);

	if (!tee_device_get(teedev))
		return ERR_PTR(-EINVAL);

	teedev_ctx_get(ctx);

	shm = calloc(1, sizeof(*shm));
	if (!shm) {
		rc = -ENOMEM;
		goto err;
	}

	shm->fd = -EBADF;
	shm->dev.id = -EACCES;
	shm->ctx = ctx;
	shm->kaddr = addr;
	shm->paddr = virt_to_phys(shm->kaddr);
	shm->size = size;
	shm->flags = flags;

	if (!(flags & TEE_SHM_PRIV)) {
		if (IS_ENABLED(CONFIG_OPTEE_DEVFS)) {
			shm->res.start = (resource_size_t)addr;
			shm->res.end = (resource_size_t)(addr + size - 1);
			shm->res.flags = IORESOURCE_MEM;

			shm->dev.id = DEVICE_ID_DYNAMIC;
			shm->dev.parent = &ctx->teedev->dev;
			shm->dev.resource = &shm->res;
			shm->dev.num_resources = 1;
			rc = dev_set_name(&shm->dev, "%s-shm", ctx->teedev->dev.unique_name);
			if (rc)
				goto err;

			rc = register_device(&shm->dev);
			if (rc)
				goto err;

			shm->res.name = shm->dev.unique_name;

			shm->cdev.dev = &shm->dev;
			shm->cdev.ops = &tee_shm_ops;
			shm->cdev.size = size;
			shm->cdev.name = shm->dev.unique_name;
			rc = devfs_create(&shm->cdev);
			if (rc)
				goto err;
		}

		list_add(&shm->link, &ctx->list_shm);
	}

	if (flags & TEE_SHM_DYNAMIC) {
		rc = ctx->teedev->desc->ops->shm_register(ctx, shm);
		if (rc)
			goto err;
	}

	refcount_set(&shm->refcount, 1);

	pr_debug("%s: shm=%p cdev=%s addr=%p size=%zu\n", __func__, shm,
		 shm->cdev.name ?: "(priv)", addr, size);

	return shm;
err:
	if (!(flags & TEE_SHM_PRIV)) {
		list_del(&shm->link);
		if (IS_ENABLED(CONFIG_OPTEE_DEVFS)) {
			devfs_remove(&shm->cdev);
			unregister_device(&shm->dev);
		}
	}

	free(shm);
	teedev_ctx_put(ctx);
	tee_device_put(teedev);

	return ERR_PTR(rc);
}

/**
 * tee_shm_register_user_buf() - Register a userspace shared memory buffer
 * @ctx:	Context that registers the shared memory
 * @addr:	The userspace address of the shared buffer
 * @length:	Length of the shared buffer
 *
 * @returns a pointer to 'struct tee_shm'
 */
struct tee_shm *tee_shm_register_user_buf(struct tee_context *ctx,
					  unsigned long addr, size_t length)
{
	u32 flags = TEE_SHM_USER_MAPPED | TEE_SHM_DYNAMIC;

	return register_shm_helper(ctx, (void *)addr, length, flags);
}

static struct tee_shm *shm_alloc_helper(struct tee_context *ctx, size_t size,
					size_t align, u32 flags)
{
	struct tee_shm *shm;
	void *addr;

	addr = align ? memalign(align, size) : malloc(size);
	if (!addr)
		return ERR_PTR(-ENOMEM);

	flags |= TEE_SHM_POOL;

	shm = register_shm_helper(ctx, addr, size, flags);
	if (IS_ERR(shm))
		free(addr);

	return shm;
}

/**
 * tee_shm_alloc_user_buf() - Allocate shared memory for user space
 * @ctx:	Context that allocates the shared memory
 * @size:	Requested size of shared memory
 *
 * Memory allocated as user space shared memory is automatically freed when
 * the TEE file pointer is closed. The primary usage of this function is
 * when the TEE driver doesn't support registering ordinary user space
 * memory.
 *
 * @returns a pointer to 'struct tee_shm'
 */
struct tee_shm *tee_shm_alloc_user_buf(struct tee_context *ctx, size_t size)
	__alias(tee_shm_alloc_kernel_buf);

/**
 * tee_shm_alloc_kernel_buf() - Allocate shared memory for kernel buffer
 * @ctx:	Context that allocates the shared memory
 * @size:	Requested size of shared memory
 *
 * The returned memory registered in secure world and is suitable to be
 * passed as a memory buffer in parameter argument to
 * tee_client_invoke_func(). The memory allocated is later freed with a
 * call to tee_shm_free().
 *
 * @returns a pointer to 'struct tee_shm'
 */
struct tee_shm *tee_shm_alloc_kernel_buf(struct tee_context *ctx, size_t size)
{
	u32 flags = TEE_SHM_DYNAMIC | TEE_SHM_POOL;

	return shm_alloc_helper(ctx, size, SZ_4K, flags);
}

/**
 * tee_shm_alloc_priv_buf() - Allocate shared memory for a privately shared
 *			      kernel buffer
 * @ctx:	Context that allocates the shared memory
 * @size:	Requested size of shared memory
 *
 * This function returns similar shared memory as
 * tee_shm_alloc_kernel_buf(), but with the difference that the memory
 * might not be registered in secure world in case the driver supports
 * passing memory not registered in advance.
 *
 * This function should normally only be used internally in the TEE
 * drivers.
 *
 * @returns a pointer to 'struct tee_shm'
 */
struct tee_shm *tee_shm_alloc_priv_buf(struct tee_context *ctx, size_t size)
{
	u32 flags = TEE_SHM_PRIV | TEE_SHM_POOL;

	return shm_alloc_helper(ctx, size, SZ_4K, flags);
}
EXPORT_SYMBOL_GPL(tee_shm_alloc_priv_buf);

/**
 * tee_shm_get_fd() - Increase reference count and return file descriptor
 * @shm:	Shared memory handle
 * @returns user space file descriptor to shared memory
 */
int tee_shm_get_fd(struct tee_shm *shm)
{
	int fd;

	if (!IS_ENABLED(CONFIG_OPTEE_DEVFS))
		return -ENOSYS;

	refcount_inc(&shm->refcount);

	if (shm->fd < 0) {
		char *tmp;

		tmp = basprintf("/dev/%s", shm->cdev.name);
		if (!tmp)
			return -ENOMEM;

		shm->fd = open(tmp, O_RDONLY);
		free(tmp);
	}

	fd = shm->fd;

	if (shm->fd < 0)
		tee_shm_put(shm);

	return fd;
}

/**
 * tee_shm_free() - Free shared memory
 * @shm:	Handle to shared memory to free
 */
void tee_shm_free(struct tee_shm *shm)
{
	tee_shm_put(shm);
}
EXPORT_SYMBOL_GPL(tee_shm_free);

/**
 * tee_shm_get_va() - Get virtual address of a shared memory plus an offset
 * @shm:	Shared memory handle
 * @offs:	Offset from start of this shared memory
 * @returns virtual address of the shared memory + offs if offs is within
 *	the bounds of this shared memory, else an ERR_PTR
 */
void *tee_shm_get_va(struct tee_shm *shm, size_t offs)
{
	if (!shm->kaddr)
		return ERR_PTR(-EINVAL);
	if (offs >= shm->size)
		return ERR_PTR(-EINVAL);
	return (char *)shm->kaddr + offs;
}
EXPORT_SYMBOL_GPL(tee_shm_get_va);

/**
 * tee_shm_get_pa() - Get physical address of a shared memory plus an offset
 * @shm:	Shared memory handle
 * @offs:	Offset from start of this shared memory
 * @pa:		Physical address to return
 * @returns 0 if offs is within the bounds of this shared memory, else an
 *	error code.
 */
int tee_shm_get_pa(struct tee_shm *shm, size_t offs, phys_addr_t *pa)
{
	if (offs >= shm->size)
		return -EINVAL;
	if (pa)
		*pa = shm->paddr + offs;
	return 0;
}
EXPORT_SYMBOL_GPL(tee_shm_get_pa);

/**
 * tee_shm_get_from_id() - Find shared memory object and increase reference
 * count
 * @ctx:	Context owning the shared memory
 * @id:		Id of shared memory object
 * @returns a pointer to 'struct tee_shm' on success or an ERR_PTR on failure
 */
struct tee_shm *tee_shm_get_from_id(struct tee_context *ctx, int id)
{
	struct tee_shm *shm;

	list_for_each_entry(shm, &ctx->list_shm, link) {
		if (shm->dev.id == id) {
			refcount_inc(&shm->refcount);
			return shm;
		}
	}

	return NULL;
}
EXPORT_SYMBOL_GPL(tee_shm_get_from_id);

/**
 * tee_shm_put() - Decrease reference count on a shared memory handle
 * @shm:	Shared memory handle
 */
void tee_shm_put(struct tee_shm *shm)
{
	struct tee_device *teedev = shm->ctx->teedev;

	if (refcount_dec_and_test(&shm->refcount))
		tee_shm_release(teedev, shm);
}
EXPORT_SYMBOL_GPL(tee_shm_put);
