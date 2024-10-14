// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * qemu_fw_cfg.c - QEMU FW CFG character device
 *
 * Copyright (C) 2022 Adrian Negreanu
 * Copyright (C) 2022 Ahmad Fatoum
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <fcntl.h>
#include <dma.h>
#include <linux/err.h>
#include <linux/bitfield.h>
#include <linux/qemu_fw_cfg.h>
#include <asm/unaligned.h>
#include <io-64-nonatomic-lo-hi.h>

/* arch-specific ctrl & data register offsets are not available in ACPI, DT */
#ifdef CONFIG_X86
# define FW_CFG_CTRL_OFF 0x00
# define FW_CFG_DATA_OFF 0x01
# define FW_CFG_DMA_OFF 0x04
#else
# define FW_CFG_CTRL_OFF 0x08
# define FW_CFG_DATA_OFF 0x00
# define FW_CFG_DMA_OFF 0x10
#endif

/* fw_cfg DMA commands */
#define FW_CFG_DMA_CTL_ERROR   0x01
#define FW_CFG_DMA_CTL_READ    0x02
#define FW_CFG_DMA_CTL_SKIP    0x04
#define FW_CFG_DMA_CTL_SELECT  0x08
#define FW_CFG_DMA_CTL_WRITE   0x10

struct fw_cfg_dma {
	__be32 control;
	__be32 length;
	__be64 address;
} __packed;

/* fw_cfg device i/o register addresses */
struct fw_cfg {
	struct resource *iores;
	void __iomem *reg_ctrl;
	void __iomem *reg_data;
	void __iomem *reg_dma;
	struct cdev cdev;
	loff_t next_read_offset;
	u32 sel;
	bool is_mmio;
	struct fw_cfg_dma __iomem *acc_virt;
	dma_addr_t acc_dma;
};

static struct fw_cfg *to_fw_cfg(struct cdev *cdev)
{
	return container_of(cdev, struct fw_cfg, cdev);
}

/* pick appropriate endianness for selector key */
static void fw_cfg_select(struct fw_cfg *fw_cfg)
{
	if (fw_cfg->is_mmio)
		iowrite16be(fw_cfg->sel, fw_cfg->reg_ctrl);
	else
		iowrite16(fw_cfg->sel, fw_cfg->reg_ctrl);
}

/* clean up fw_cfg device i/o */
static void fw_cfg_io_cleanup(struct fw_cfg *fw_cfg)
{
	release_region(fw_cfg->iores);
}

static int fw_cfg_ioctl(struct cdev *cdev, unsigned int request, void *buf)
{
	struct fw_cfg *fw_cfg = to_fw_cfg(cdev);
	int ret = 0;

	switch (request) {
	case FW_CFG_SELECT:
		fw_cfg->sel = *(u16 *)buf;
		break;
	default:
		ret = -ENOTTY;
	}

	return 0;
}

#define __raw_readu64 __raw_readq
#define __raw_readu32 __raw_readl
#define __raw_readu16 __raw_readw
#define __raw_readu8 __raw_readb

#define fw_cfg_data_read_sized(fw_cfg, remaining, address, type) do {	\
	while (*remaining >= sizeof(type)) {				\
		val = __raw_read##type((fw_cfg)->reg_data);		\
		*remaining -= sizeof(type);				\
		if (*address) {						\
			put_unaligned(val, (type *)*address);		\
			*address += sizeof(type);			\
		}							\
	}								\
} while(0)

static void fw_cfg_data_read(struct fw_cfg *fw_cfg, void *address, size_t remaining,
			     unsigned rdsize)
{

	u64 val;

	if (fw_cfg->is_mmio) {
		/*
		 * This is just a preference. If we can't honour it, we
		 * fall back to byte-sized copy
		 */
		switch(rdsize) {
		case 8:
#ifdef CONFIG_64BIT
			fw_cfg_data_read_sized(fw_cfg, &remaining, &address, u64);
			break;
#endif
		case 4:
			fw_cfg_data_read_sized(fw_cfg, &remaining, &address, u32);
			break;
		case 2:
			fw_cfg_data_read_sized(fw_cfg, &remaining, &address, u16);
			break;
		}
	}

	fw_cfg_data_read_sized(fw_cfg, &remaining, &address, u8);
}

static ssize_t fw_cfg_read(struct cdev *cdev, void *buf, size_t count,
			   loff_t pos, unsigned long flags)
{
	struct fw_cfg *fw_cfg = to_fw_cfg(cdev);
	unsigned rdsize = FIELD_GET(O_RWSIZE_MASK, flags);

	if (!pos || pos != fw_cfg->next_read_offset) {
		fw_cfg_select(fw_cfg);
		fw_cfg->next_read_offset = 0;
	}

	if (!rdsize) {
		if (pos % 8 == 0)
			rdsize = 8;
		else if (pos % 4 == 0)
			rdsize = 4;
		else if (pos % 2 == 0)
			rdsize = 2;
		else
			rdsize = 1;
	}

	while (pos-- > fw_cfg->next_read_offset)
		fw_cfg_data_read(fw_cfg, NULL, count, rdsize);

	fw_cfg_data_read(fw_cfg, buf, count, rdsize);

	fw_cfg->next_read_offset += count;
	return count;
}

static ssize_t fw_cfg_write(struct cdev *cdev, const void *buf, size_t count,
			    loff_t pos, unsigned long flags)
{
	struct fw_cfg *fw_cfg = to_fw_cfg(cdev);
	struct device *dev = cdev->dev;
	struct fw_cfg_dma __iomem *acc = fw_cfg->acc_virt;
	void *dma_buf;
	dma_addr_t mapping;
	int ret = 0;

	if (pos != 0)
		return -EINVAL;

	dma_buf = dma_alloc(count);
	if (!dma_buf)
		return -ENOMEM;

	memcpy(dma_buf, buf, count);

	mapping = dma_map_single(dev, dma_buf, count, DMA_TO_DEVICE);
	if (dma_mapping_error(dev, mapping)) {
		ret = -EFAULT;
		goto free_buf;
	}

	fw_cfg->next_read_offset = 0;

	acc->address = cpu_to_be64(mapping);
	acc->length = cpu_to_be32(count);
	acc->control = cpu_to_be32(FW_CFG_DMA_CTL_WRITE |
				   FW_CFG_DMA_CTL_SELECT | fw_cfg->sel << 16);

	iowrite64be(fw_cfg->acc_dma, fw_cfg->reg_dma);

	while (ioread32be(&acc->control) & ~FW_CFG_DMA_CTL_ERROR)
		;

	dma_unmap_single(dev, mapping, count, DMA_FROM_DEVICE);
free_buf:
	dma_free(dma_buf);

	return ret ?: count;
}

static struct cdev_operations fw_cfg_ops = {
	.read = fw_cfg_read,
	.write = fw_cfg_write,
	.ioctl = fw_cfg_ioctl,
};

static int fw_cfg_param_select(struct param_d *p, void *priv)
{
	struct fw_cfg *fw_cfg = priv;

	return fw_cfg->sel <= U16_MAX ? 0 : -EINVAL;
}

static int fw_cfg_probe(struct device *dev)
{
	struct device_node *np = dev_of_node(dev);
	struct resource *parent_res, *iores;
	char sig[FW_CFG_SIG_SIZE];
	struct fw_cfg *fw_cfg;
	int ret;

	fw_cfg = xzalloc(sizeof(*fw_cfg));

	/* acquire i/o range details */
	fw_cfg->is_mmio = false;
	iores = dev_get_resource(dev, IORESOURCE_IO, 0);
	if (IS_ERR(iores)) {
		fw_cfg->is_mmio = true;
		iores = dev_get_resource(dev, IORESOURCE_MEM, 0);
		if (IS_ERR(iores))
			return -EINVAL;
	}

	parent_res = fw_cfg->is_mmio ? &iomem_resource : &ioport_resource;
	iores = __request_region(parent_res, iores->start, iores->end, dev_name(dev), 0);
	if (IS_ERR(iores))
		return -EBUSY;

	/* use architecture-specific offsets */
	fw_cfg->reg_ctrl = IOMEM(iores->start + FW_CFG_CTRL_OFF);
	fw_cfg->reg_data = IOMEM(iores->start + FW_CFG_DATA_OFF);
	fw_cfg->reg_dma  = IOMEM(iores->start + FW_CFG_DMA_OFF);

	fw_cfg->iores = iores;

	/* verify fw_cfg device signature */
	fw_cfg->sel = FW_CFG_SIGNATURE;
	fw_cfg_read(&fw_cfg->cdev, sig, FW_CFG_SIG_SIZE, 0, 0);

	if (memcmp(sig, "QEMU", FW_CFG_SIG_SIZE) != 0) {
		ret = np ? -EILSEQ : -ENODEV;
		goto err;
	}

	fw_cfg->acc_virt = dma_alloc_coherent(DMA_DEVICE_BROKEN,
					      sizeof(*fw_cfg->acc_virt), &fw_cfg->acc_dma);

	fw_cfg->cdev.name = basprintf("fw_cfg%d", cdev_find_free_index("fw_cfg"));
	fw_cfg->cdev.flags = DEVFS_IS_CHARACTER_DEV;
	fw_cfg->cdev.size = 0;
	fw_cfg->cdev.ops = &fw_cfg_ops;
	fw_cfg->cdev.dev = dev;
	fw_cfg->cdev.filetype = filetype_qemu_fw_cfg;

	dev_set_name(dev, fw_cfg->cdev.name);

	ret = devfs_create(&fw_cfg->cdev);
	if (ret) {
		dev_err(dev, "Failed to create corresponding cdev\n");
		goto err;
	}

	cdev_create_default_automount(&fw_cfg->cdev);

	dev_add_param_uint32(dev, "selector", fw_cfg_param_select,
			     NULL, &fw_cfg->sel, "%u", fw_cfg);

	dev->priv = fw_cfg;

	return 0;
err:
	fw_cfg_io_cleanup(fw_cfg);
	return ret;
}

static const struct of_device_id qemu_fw_cfg_of_match[] = {
	{ .compatible = "qemu,fw-cfg-mmio", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, qemu_fw_cfg_of_match);

static struct driver qemu_fw_cfg_drv = {
	.name = "fw_cfg",
	.probe  = fw_cfg_probe,
	.of_compatible = of_match_ptr(qemu_fw_cfg_of_match),
};

static int qemu_fw_cfg_init(void)
{
	int ret;

	ret = platform_driver_register(&qemu_fw_cfg_drv);
	if (ret)
		return ret;

	return of_devices_ensure_probed_by_dev_id(qemu_fw_cfg_of_match);
}
postmmu_initcall(qemu_fw_cfg_init);
