// SPDX-License-Identifier: GPL-2.0-only
/*
 * devres.c - device-managed resources on top of talloc
 *
 * Managed resources are children of a per-device talloc context.
 */

#include <barebox.h>
#include <driver.h>
#include <dma.h>
#include <talloc.h>
#include <xfuncs.h>
#include <stdio.h>
#include <printf.h>
#include <linux/device.h>
#include <linux/overflow.h>
#include <asm/sections.h>

static void *devres_ctx(struct device *dev)
{
	if (!dev->devres)
		dev->devres = talloc_new(NULL);

	return dev->devres;
}

/**
 * devres_release_all() - release all managed resources of a device
 * @dev: device to release resources of
 *
 * Runs all actions and frees all memory registered with devm_ functions
 * in reverse order of registration.
 */
void devres_release_all(struct device *dev)
{
	talloc_free(dev->devres);
	dev->devres = NULL;
}

/**
 * devres_run_actions() - run the managed actions of a device
 * @dev: device that is shut down
 *
 * Like devres_release_all(), but keeps the managed memory, which may
 * still be referenced after shutdown.
 */
void devres_run_actions(struct device *dev)
{
	talloc_run_destructors(dev->devres);
}

/*
 * Memory must be DMA-safe like kmalloc()'s, so align it within the chunk
 * and store the chunk pointer in front of it for devm_kfree().
 */
static void *devres_alloc(struct device *dev, size_t size)
{
	void *chunk, *mem;

	if (!size)
		return ZERO_SIZE_PTR;

	/* Saturated size_mul()/struct_size() results would wrap in ALIGN() */
	if (size > SIZE_MAX - 2 * DMA_ALIGNMENT)
		panic("%s: devm allocation of %zu bytes\n", dev_name(dev), size);

	/* chunk is pointer-aligned, so mem is at most DMA_ALIGNMENT bytes in */
	chunk = xtalloc_size(devres_ctx(dev),
			     DMA_ALIGNMENT + ALIGN(size, DMA_ALIGNMENT));
	mem = PTR_ALIGN(chunk + sizeof(void *), DMA_ALIGNMENT);
	((void **)mem)[-1] = chunk;

	return mem;
}

void *devm_kmalloc(struct device *dev, size_t size, gfp_t gfp)
{
	return devres_alloc(dev, size);
}
EXPORT_SYMBOL(devm_kmalloc);

void *devm_kzalloc(struct device *dev, size_t size, gfp_t gfp)
{
	void *mem = devres_alloc(dev, size);

	memset(mem, 0, size);

	return mem;
}
EXPORT_SYMBOL(devm_kzalloc);

/**
 * devm_kfree() - free memory allocated with devm_kmalloc() early
 * @dev: device the memory was allocated for
 * @mem: pointer to memory allocated by a devm_ function
 */
void devm_kfree(struct device *dev, const void *mem)
{
	/* NULL/ZERO_SIZE_PTR or .rodata string from devm_kstrdup_const() */
	if (ZERO_OR_NULL_PTR(mem) || is_barebox_rodata((ulong)mem))
		return;

	talloc_free(((void **)mem)[-1]);
}
EXPORT_SYMBOL(devm_kfree);

void *devm_kmemdup(struct device *dev, const void *src, size_t len, gfp_t gfp)
{
	return memcpy(devres_alloc(dev, len), src, len);
}
EXPORT_SYMBOL(devm_kmemdup);

char *devm_kstrdup(struct device *dev, const char *s, gfp_t gfp)
{
	if (!s)
		return NULL;

	return devm_kmemdup(dev, s, strlen(s) + 1, gfp);
}
EXPORT_SYMBOL(devm_kstrdup);

const char *devm_kstrdup_const(struct device *dev, const char *s, gfp_t gfp)
{
	if (is_barebox_rodata((ulong)s))
		return s;

	return devm_kstrdup(dev, s, gfp);
}
EXPORT_SYMBOL(devm_kstrdup_const);

char *devm_kvasprintf(struct device *dev, gfp_t gfp, const char *fmt,
		      va_list ap)
{
	unsigned int len;
	va_list aq;
	char *p;

	va_copy(aq, ap);
	len = vsnprintf(NULL, 0, fmt, aq) + 1;
	va_end(aq);

	p = devres_alloc(dev, len);
	vsnprintf(p, len, fmt, ap);

	return p;
}
EXPORT_SYMBOL(devm_kvasprintf);

char *devm_kasprintf(struct device *dev, gfp_t gfp, const char *fmt, ...)
{
	va_list ap;
	char *p;

	va_start(ap, fmt);
	p = devm_kvasprintf(dev, gfp, fmt, ap);
	va_end(ap);

	return p;
}
EXPORT_SYMBOL(devm_kasprintf);

int devm_add_action(struct device *dev, void (*action)(void *), void *data)
{
	void *ctx = devres_ctx(dev);

	if (!ctx)
		return -ENOMEM;

	return talloc_add_destructor(ctx, action, data);
}
EXPORT_SYMBOL(devm_add_action);

static void devm_region_release(void *res)
{
	/* No need to release regions when shutting down */
	if (barebox_system_state == BAREBOX_EXITING)
		return;

	release_region(res);
}

/**
 * devm_request_mem_resource() - exclusively request resource 'num' for a device
 * @dev: device to request the memory resource for
 * @num: index of the memory resource
 *
 * Like dev_request_mem_resource(), but device-managed.
 *
 * Return: the requested resource or an ERR_PTR() on failure.
 */
struct resource *devm_request_mem_resource(struct device *dev, int num)
{
	struct resource *res;
	int ret;

	res = dev_request_mem_resource(dev, num);
	if (IS_ERR(res))
		return res;

	ret = devm_add_action_or_reset(dev, devm_region_release, res);
	if (ret)
		return ERR_PTR(ret);

	return res;
}
EXPORT_SYMBOL(devm_request_mem_resource);
