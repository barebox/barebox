// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 Nicolas Saenz Julienne <nsaenzjulienne@suse.de>
 */

#include <io.h>
#include <driver.h>
#include <linux/bitmap.h>
#include <linux/nvmem-provider.h>
#include <init.h>

struct rmem {
	struct device *dev;
	const struct resource *mem;
	unsigned long *protection_bitmap;
	size_t size;
};

static int rmem_read(void *context, unsigned int offset,
		     void *val, size_t bytes)
{
	struct rmem *rmem = context;
	return mem_copy(rmem->dev, val, (void *)rmem->mem->start + offset,
			bytes, offset, 0);
}

/**
 * rmem_write() - Write data to the NVMEM device.
 * @context:	Pointer to the rmem private data (struct rmem).
 * @offset:	Offset within the NVMEM device to write to.
 * @val:	Buffer containing the data to write.
 * @bytes:	Number of bytes to write.
 *
 * This function is called by the NVMEM core to write data to the
 * reserved memory region. It first checks the protection bitmap to ensure
 * the target range is not write-protected. If writable, it uses the
 * custom 'mem_copy' function.
 *
 * Return: 0 on success, or a negative error code on failure.
 */
static int rmem_write(void *context, unsigned int offset, const void *val,
		      size_t bytes)
{
	unsigned long protected_offset;
	struct rmem *rmem = context;

	protected_offset = find_next_bit(rmem->protection_bitmap,
					 offset + bytes, offset);
	if (protected_offset < offset + bytes)
		return -EROFS;

	/*
	 * The last two arguments to mem_copy (0, 0) are specific to
	 * the custom mem_copy implementation.
	 */
	return mem_copy(rmem->dev, (void *)rmem->mem->start + offset, val,
			bytes, 0, 0);
}

/**
 * rmem_protect() - NVMEM callback to change protection status of a range.
 * @context:	Pointer to the rmem private data (struct rmem).
 * @offset:	Starting offset of the range.
 * @bytes:	Length of the range in bytes.
 * @prot_mode:	Protection mode to apply (PROTECT_DISABLE_WRITE or
 * PROTECT_ENABLE_WRITE).
 *
 * This function is called by the NVMEM core to enable or disable
 * write protection for a specified memory range.
 *
 * Return: 0 on success, or a negative error code on failure.
 */
static int rmem_protect(void *context, unsigned int offset, size_t bytes,
			int prot_mode)
{
	struct rmem *rmem = context;
	int ret;

	if (offset + bytes > rmem->size)
		return -EINVAL;

	switch (prot_mode) {
	case PROTECT_DISABLE_WRITE: /* Make read-only */
		bitmap_set(rmem->protection_bitmap, offset, bytes);
		break;
	case PROTECT_ENABLE_WRITE: /* Make writable */
		bitmap_clear(rmem->protection_bitmap, offset, bytes);
		break;
	default:
		dev_warn(rmem->dev, "%s: Invalid protection mode %d\n",
			 __func__, prot_mode);
		ret = -EINVAL;
		break;
	}

	dev_dbg(rmem->dev,
		"Protection op complete [0x%x, len %zu], mode %d\n", offset,
		bytes, prot_mode);

	return 0;
}

static int rmem_probe(struct device *dev)
{
	struct nvmem_config config = { };
	struct resource *mem;
	struct rmem *priv;

	mem = dev_get_resource(dev, IORESOURCE_MEM, 0);
	if (IS_ERR(mem))
		return PTR_ERR(mem);

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->mem = mem;

	config.dev = priv->dev = dev;
	config.priv = priv;
	config.name = "rmem";
	config.size = resource_size(mem);
	priv->size = config.size;

	priv->protection_bitmap = bitmap_xzalloc(priv->size);

	config.reg_read = rmem_read;
	config.reg_write = rmem_write;
	config.reg_protect = rmem_protect;

	return PTR_ERR_OR_ZERO(nvmem_register(&config));
}

static const struct of_device_id rmem_match[] = {
	{ .compatible = "nvmem-rmem", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, rmem_match);

static struct driver rmem_driver = {
	.name = "rmem",
	.of_compatible = rmem_match,
	.probe = rmem_probe,
};
device_platform_driver(rmem_driver);

MODULE_AUTHOR("Nicolas Saenz Julienne <nsaenzjulienne@suse.de>");
MODULE_DESCRIPTION("Reserved Memory Based nvmem Driver");
MODULE_LICENSE("GPL");
