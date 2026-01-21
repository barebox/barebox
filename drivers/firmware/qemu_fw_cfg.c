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

/* fw_cfg device i/o register addresses */
struct fw_cfg {
	struct resource *iores;
	void __iomem *reg_ctrl;
	void __iomem *reg_data;
	void __iomem *reg_dma;
	struct cdev cdev;
	u32 sel;
	bool is_mmio;
	struct fw_cfg_dma_access __iomem *acc_virt;
	dma_addr_t acc_dma;
	u32 rev;
	int use_pio;
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

static inline bool fw_cfg_dma_enabled(struct fw_cfg *fw_cfg)
{
	return (fw_cfg->rev & FW_CFG_VERSION_DMA) && fw_cfg->reg_dma;
}

/* qemu fw_cfg device is sync today, but spec says it may become async */
static void fw_cfg_wait_for_control(struct fw_cfg_dma_access *d)
{
	for (;;) {
		u32 ctrl = be32_to_cpu(READ_ONCE(d->control));

		/* do not reorder the read to d->control */
		/* rmb(); */
		if ((ctrl & ~FW_CFG_DMA_CTL_ERROR) == 0)
			return;

		cpu_relax();
	}
}

static ssize_t fw_cfg_dma_transfer(struct fw_cfg *fw_cfg,
				   void *address, u32 length, u32 control)
{
	phys_addr_t dma;
	struct fw_cfg_dma_access *d = NULL;
	ssize_t ret = length;

	d = dma_alloc(sizeof(*d));
	if (!d) {
		ret = -ENOMEM;
		goto end;
	}

	/* fw_cfg device does not need IOMMU protection, so use physical addresses */
	*d = (struct fw_cfg_dma_access) {
		.address = cpu_to_be64(address ? virt_to_phys(address) : 0),
		.length = cpu_to_be32(length),
		.control = cpu_to_be32(control)
	};

	dma = virt_to_phys(d);

	iowrite32be((u64)dma >> 32, fw_cfg->reg_dma);
	/* force memory to sync before notifying device via MMIO */
	/* wmb(); */
	iowrite32be(dma, fw_cfg->reg_dma + 4);

	fw_cfg_wait_for_control(d);

	if (be32_to_cpu(READ_ONCE(d->control)) & FW_CFG_DMA_CTL_ERROR) {
		ret = -EIO;
	}

end:
	dma_free(d);

	return ret;
}

static ssize_t fw_cfg_dma_transfer_at(struct fw_cfg *fw_cfg,
				      void *buf, u32 count, loff_t pos,
				      u32 direction)
{
	int ret;

	if (pos == 0) {
		ret = fw_cfg_dma_transfer(fw_cfg, buf, count,
					  fw_cfg->sel << 16 |
					  FW_CFG_DMA_CTL_SELECT | direction);
	} else {
		fw_cfg_select(fw_cfg);
		ret = fw_cfg_dma_transfer(fw_cfg, NULL, pos, FW_CFG_DMA_CTL_SKIP);
		if (ret >= 0)
			ret = fw_cfg_dma_transfer(fw_cfg, buf, count, direction);
	}

	return ret;
}

static ssize_t fw_cfg_dma_read(struct cdev *cdev, void *buf,
			       size_t count, loff_t pos, unsigned long flags)
{
	return fw_cfg_dma_transfer_at(to_fw_cfg(cdev), buf, count, pos,
				      FW_CFG_DMA_CTL_READ);
}

static ssize_t fw_cfg_dma_write(struct cdev *cdev, const void *buf,
				size_t count, loff_t pos, unsigned long flags)
{
	return fw_cfg_dma_transfer_at(to_fw_cfg(cdev), (void *)buf, count, pos,
				      FW_CFG_DMA_CTL_WRITE);
}

static struct cdev_operations fw_cfg_dma_ops = {
	.read = fw_cfg_dma_read,
	.write = fw_cfg_dma_write,
	.ioctl = fw_cfg_ioctl,
};

static void reads_n(const void __iomem *src, void *dst,
		   resource_size_t count, int rwsize)
{
	switch (rwsize) {
	case 1: readsb(src, dst, count / 1); break;
	case 2: readsw(src, dst, count / 2); break;
	case 4: readsl(src, dst, count / 4); break;
#ifdef CONFIG_64BIT
	case 8: readsq(src, dst, count / 8); break;
#endif
	}
}

static void ins_n(unsigned long src, void *dst,
		  resource_size_t count, int rwsize)
{
	switch (rwsize) {
	case 1: insb(src, dst, count / 1); break;
	case 2: insw(src, dst, count / 2); break;
	case 4: insl(src, dst, count / 4); break;
	/* No insq, so just do 32-bit accesses */
	case 8: insl(src, dst, count / 4); break;
	}
}

static ssize_t fw_cfg_pio_read(struct cdev *cdev, void *buf, size_t count,
			       loff_t pos, unsigned long flags)
{
	struct fw_cfg *fw_cfg = to_fw_cfg(cdev);
	unsigned rdsize = FIELD_GET(O_RWSIZE_MASK, flags) ?: 8;

	if (!IS_ALIGNED(pos, rdsize) || !IS_ALIGNED(count, rdsize))
		rdsize = 1;

	fw_cfg_select(fw_cfg);
	while (pos-- > 0)
		ioread8(fw_cfg->reg_data);

	if (fw_cfg->is_mmio)
		reads_n(fw_cfg->reg_data, buf, count, rdsize);
	else
		ins_n((ulong)fw_cfg->reg_data, buf, count, rdsize);

	return count;
}

static void writes_n(void __iomem *dst, const void *src,
		   resource_size_t count, int rwsize)
{
	switch (rwsize) {
	case 1: writesb(dst, src, count / 1); break;
	case 2: writesw(dst, src, count / 2); break;
	case 4: writesl(dst, src, count / 4); break;
#ifdef CONFIG_64BIT
	case 8: writesq(dst, src, count / 8); break;
#endif
	}
}

static void outs_n(unsigned long dst, const void *src,
		  resource_size_t count, int rwsize)
{
	switch (rwsize) {
	case 1: outsb(dst, src, count / 1); break;
	case 2: outsw(dst, src, count / 2); break;
	case 4: outsl(dst, src, count / 4); break;
	/* No insq, so just do 32-bit accesses */
	case 8: outsl(dst, src, count / 4); break;
	}
}

static ssize_t fw_cfg_pio_write(struct cdev *cdev, const void *buf, size_t count,
			       loff_t pos, unsigned long flags)
{
	struct fw_cfg *fw_cfg = to_fw_cfg(cdev);
	unsigned wrsize = FIELD_GET(O_RWSIZE_MASK, flags) ?: 8;

	if (!IS_ALIGNED(pos, wrsize) || !IS_ALIGNED(count, wrsize))
		wrsize = 1;

	if (fw_cfg->is_mmio)
		writes_n(fw_cfg->reg_data, buf, count, wrsize);
	else
		outs_n((ulong)fw_cfg->reg_data, buf, count, wrsize);

	return count;
}

static struct cdev_operations fw_cfg_pio_ops = {
	.read = fw_cfg_pio_read,
	.write = fw_cfg_pio_write,
	.ioctl = fw_cfg_ioctl,
};

static int fw_cfg_param_select(struct param_d *p, void *priv)
{
	struct fw_cfg *fw_cfg = priv;

	return fw_cfg->sel <= U16_MAX ? 0 : -EINVAL;
}

static int fw_cfg_param_use_pio(struct param_d *p, void *priv)
{
	struct fw_cfg *fw_cfg = priv;
	fw_cfg->cdev.ops = fw_cfg->use_pio ? &fw_cfg_pio_ops : &fw_cfg_dma_ops;
	return 0;
}

static int fw_cfg_probe(struct device *dev)
{
	struct device_node *np = dev_of_node(dev);
	struct resource *parent_res, *iores;
	char sig[FW_CFG_SIG_SIZE];
	struct fw_cfg *fw_cfg;
	__le32 rev;
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
	fw_cfg_pio_read(&fw_cfg->cdev, sig, FW_CFG_SIG_SIZE, 0, 0);

	if (memcmp(sig, "QEMU", FW_CFG_SIG_SIZE) != 0) {
		ret = np ? -EILSEQ : -ENODEV;
		goto err;
	}

	fw_cfg->sel = FW_CFG_ID;
	ret = fw_cfg_pio_read(&fw_cfg->cdev, &rev, sizeof(rev), 0, 0);
	if (ret < 0)
		goto err;

	fw_cfg->rev = le32_to_cpu(rev);
	fw_cfg->use_pio = !fw_cfg_dma_enabled(fw_cfg);

	fw_cfg->acc_virt = dma_alloc_coherent(DMA_DEVICE_BROKEN,
					      sizeof(*fw_cfg->acc_virt), &fw_cfg->acc_dma);

	fw_cfg->cdev.name = "fw_cfg";
	fw_cfg->cdev.flags = DEVFS_IS_CHARACTER_DEV;
	fw_cfg->cdev.size = 0;
	fw_cfg->cdev.ops = fw_cfg->use_pio ? &fw_cfg_pio_ops : &fw_cfg_dma_ops;
	fw_cfg->cdev.dev = dev;
	fw_cfg->cdev.filetype = filetype_qemu_fw_cfg;

	dev_set_name(dev, "%s", fw_cfg->cdev.name);

	ret = devfs_create(&fw_cfg->cdev);
	if (ret) {
		dev_err(dev, "Failed to create corresponding cdev\n");
		goto err;
	}

	cdev_create_default_automount(&fw_cfg->cdev);

	dev_add_param_uint32(dev, "selector", fw_cfg_param_select,
			     NULL, &fw_cfg->sel, "%u", fw_cfg);

	dev_add_param_bool(dev, "pio", fw_cfg_param_use_pio, NULL,
			   &fw_cfg->use_pio, fw_cfg);

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
device_initcall(qemu_fw_cfg_init);
