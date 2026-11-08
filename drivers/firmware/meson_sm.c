// SPDX-License-Identifier: GPL-2.0-only
// SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/firmware/meson/meson_sm.c?id=1c5cb7391ef5f869ef333c04d6422c0e0859a870
/*
 * Amlogic Secure Monitor driver
 *
 * Copyright (C) 2016 Endless Mobile, Inc.
 * Author: Carlo Caione <carlo@endlessm.com>
 */

#define pr_fmt(fmt) "meson-sm: " fmt

#include <linux/arm-smccc.h>
#include <linux/bug.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/module.h>
#include <barebox-info.h>
#include <of.h>
#include <of_address.h>
#include <driver.h>
#include <mmu.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/sizes.h>
 #include <linux/slab.h>

#include <linux/firmware/meson/meson_sm.h>

struct meson_sm_cmd {
	unsigned int index;
	u32 smc_id;
};
#define CMD(d, s) { .index = (d), .smc_id = (s), }

struct meson_sm_chip {
	unsigned int shmem_size;
	u32 cmd_shmem_in_base;
	u32 cmd_shmem_out_base;
	struct meson_sm_cmd cmd[];
};

static const struct meson_sm_chip gxbb_chip = {
	.shmem_size		= SZ_4K,
	.cmd_shmem_in_base	= 0x82000020,
	.cmd_shmem_out_base	= 0x82000021,
	.cmd = {
		CMD(SM_EFUSE_READ,		0x82000030),
		CMD(SM_EFUSE_WRITE,		0x82000031),
		CMD(SM_EFUSE_USER_MAX,	0x82000033),
		CMD(SM_GET_CHIP_ID,		0x82000044),
		CMD(SM_THERMAL_CALIB_READ,	0x82000047),
		CMD(SM_A1_PWRC_SET,		0x82000093),
		CMD(SM_A1_PWRC_GET,		0x82000095),
		{ /* sentinel */ },
	},
};

struct meson_sm_firmware {
	const struct meson_sm_chip *chip;
	void __iomem *sm_shmem_in_base;
	void __iomem *sm_shmem_out_base;
};

static u32 meson_sm_get_cmd(const struct meson_sm_chip *chip,
			    unsigned int cmd_index)
{
	const struct meson_sm_cmd *cmd = chip->cmd;

	while (cmd->smc_id && cmd->index != cmd_index)
		cmd++;

	return cmd->smc_id;
}

static s32 __meson_sm_call(u32 cmd, u32 arg0, u32 arg1, u32 arg2,
			   u32 arg3, u32 arg4)
{
	struct arm_smccc_res res;

	arm_smccc_smc(cmd, arg0, arg1, arg2, arg3, arg4, 0, 0, &res);
	return res.a0;
}

static void __iomem *meson_sm_map_shmem(u32 cmd_shmem, unsigned int size)
{
	u32 sm_phy_base;
	unsigned long base;

	sm_phy_base = __meson_sm_call(cmd_shmem, 0, 0, 0, 0, 0);
	if (!sm_phy_base)
		return NULL;

	/* barebox maps reserved memory uncached, ioremap_cache() maps cached */
	base = PAGE_ALIGN_DOWN(sm_phy_base);
	if (remap_range((void *)base, PAGE_ALIGN(sm_phy_base - base + size), MAP_CACHED))
		return NULL;

	return IOMEM((unsigned long)sm_phy_base);
}

/**
 * meson_sm_call - generic SMC32 call to the secure-monitor
 *
 * @fw:		Pointer to secure-monitor firmware
 * @cmd_index:	Index of the SMC32 function ID
 * @ret:	Returned value
 * @arg0:	SMC32 Argument 0
 * @arg1:	SMC32 Argument 1
 * @arg2:	SMC32 Argument 2
 * @arg3:	SMC32 Argument 3
 * @arg4:	SMC32 Argument 4
 *
 * Return:	0 on success, a negative value on error
 */
int meson_sm_call(struct meson_sm_firmware *fw, unsigned int cmd_index,
		  s32 *ret, u32 arg0, u32 arg1, u32 arg2, u32 arg3, u32 arg4)
{
	u32 cmd;
	s32 lret;

	if (!fw->chip)
		return -ENOENT;

	cmd = meson_sm_get_cmd(fw->chip, cmd_index);
	if (!cmd)
		return -EINVAL;

	lret = __meson_sm_call(cmd, arg0, arg1, arg2, arg3, arg4);

	if (ret)
		*ret = lret;

	return 0;
}
EXPORT_SYMBOL(meson_sm_call);

/**
 * meson_sm_call_read - retrieve data from secure-monitor
 *
 * @fw:		Pointer to secure-monitor firmware
 * @buffer:	Buffer to store the retrieved data
 * @bsize:	Size of the buffer
 * @cmd_index:	Index of the SMC32 function ID
 * @arg0:	SMC32 Argument 0
 * @arg1:	SMC32 Argument 1
 * @arg2:	SMC32 Argument 2
 * @arg3:	SMC32 Argument 3
 * @arg4:	SMC32 Argument 4
 *
 * Return:	size of read data on success, a negative value on error
 *		When 0 is returned there is no guarantee about the amount of
 *		data read and bsize bytes are copied in buffer.
 */
int meson_sm_call_read(struct meson_sm_firmware *fw, void *buffer,
		       unsigned int bsize, unsigned int cmd_index, u32 arg0,
		       u32 arg1, u32 arg2, u32 arg3, u32 arg4)
{
	s32 size;
	int ret;

	if (!fw->chip)
		return -ENOENT;

	if (!fw->chip->cmd_shmem_out_base)
		return -EINVAL;

	if (bsize > fw->chip->shmem_size)
		return -EINVAL;

	if (meson_sm_call(fw, cmd_index, &size, arg0, arg1, arg2, arg3, arg4) < 0)
		return -EINVAL;

	if (size < 0 || size > bsize)
		return -EINVAL;

	ret = size;

	/* In some cases (for example GET_CHIP_ID command),
	 * SMC doesn't return the number of bytes read, even
	 * though the bytes were actually read into sm_shmem_out.
	 * So this check is needed.
	 */
	if (!size)
		size = bsize;

	if (buffer)
		memcpy(buffer, fw->sm_shmem_out_base, size);

	return ret;
}
EXPORT_SYMBOL(meson_sm_call_read);

/**
 * meson_sm_call_write - send data to secure-monitor
 *
 * @fw:		Pointer to secure-monitor firmware
 * @buffer:	Buffer containing data to send
 * @size:	Size of the data to send
 * @cmd_index:	Index of the SMC32 function ID
 * @arg0:	SMC32 Argument 0
 * @arg1:	SMC32 Argument 1
 * @arg2:	SMC32 Argument 2
 * @arg3:	SMC32 Argument 3
 * @arg4:	SMC32 Argument 4
 *
 * Return:	size of sent data on success, a negative value on error
 */
int meson_sm_call_write(struct meson_sm_firmware *fw, void *buffer,
			unsigned int size, unsigned int cmd_index, u32 arg0,
			u32 arg1, u32 arg2, u32 arg3, u32 arg4)
{
	s32 written;

	if (!fw->chip)
		return -ENOENT;

	if (size > fw->chip->shmem_size)
		return -EINVAL;

	if (!fw->chip->cmd_shmem_in_base)
		return -EINVAL;

	memcpy(fw->sm_shmem_in_base, buffer, size);

	if (meson_sm_call(fw, cmd_index, &written, arg0, arg1, arg2, arg3, arg4) < 0)
		return -EINVAL;

	if (written <= 0 || written > size)
		return -EINVAL;

	return written;
}
EXPORT_SYMBOL(meson_sm_call_write);

/**
 * meson_sm_get - get pointer to meson_sm_firmware structure.
 *
 * @sm_node:		Pointer to the secure-monitor Device Tree node.
 *
 * Return:		NULL is the secure-monitor device is not ready.
 */
struct meson_sm_firmware *meson_sm_get(struct device_node *sm_node)
{
	struct device *dev = of_find_device_by_node(sm_node);
	struct meson_sm_firmware *fw;

	if (!dev)
		return NULL;

	fw = dev_get_drvdata(dev);

	put_device(dev);

	return fw;
}
EXPORT_SYMBOL_GPL(meson_sm_get);

/**
 * meson_sm_get_thermal_calib - Read thermal sensor calibration data.
 * @fw:		Pointer to secure-monitor firmware.
 * @trim_info:	Pointer to store the returned calibration data.
 * @tsensor_id:	Sensor index to identify which sensor's calibration data
 *		to retrieve
 *
 * Return: 0 on success, negative error code on failure.
 */
int meson_sm_get_thermal_calib(struct meson_sm_firmware *fw, u32 *trim_info,
			       u32 tsensor_id)
{
	return meson_sm_call(fw, SM_THERMAL_CALIB_READ, trim_info, tsensor_id,
			     0, 0, 0, 0);
}
EXPORT_SYMBOL_GPL(meson_sm_get_thermal_calib);

#define SM_CHIP_ID_LENGTH	119
#define SM_CHIP_ID_OFFSET	4
#define SM_CHIP_ID_SIZE		12

static void meson_sm_set_soc_uid(struct meson_sm_firmware *fw)
{
	u8 id_buf[SM_CHIP_ID_LENGTH];
	int ret;

	ret = meson_sm_call_read(fw, id_buf, SM_CHIP_ID_LENGTH, SM_GET_CHIP_ID,
				 0, 0, 0, 0, 0);
	if (ret < 0) {
		pr_warn("failed to read chip id: %pe\n", ERR_PTR(ret));
		return;
	}

	/* in hex, which is the format of Linux' serial attribute */
	barebox_set_soc_uid(NULL, &id_buf[SM_CHIP_ID_OFFSET], SM_CHIP_ID_SIZE);
}

static const struct of_device_id meson_sm_ids[] = {
	{ .compatible = "amlogic,meson-gxbb-sm", .data = &gxbb_chip },
	{ /* sentinel */ },
};

static int __init meson_sm_probe(struct device *dev)
{
	const struct meson_sm_chip *chip;
	struct meson_sm_firmware *fw;

	fw = devm_kzalloc(dev, sizeof(*fw), GFP_KERNEL);
	if (!fw)
		return -ENOMEM;

	chip = device_get_match_data(dev);
	if (!chip)
		return -EINVAL;

	if (chip->cmd_shmem_in_base) {
		fw->sm_shmem_in_base = meson_sm_map_shmem(chip->cmd_shmem_in_base,
							  chip->shmem_size);
		if (WARN_ON(!fw->sm_shmem_in_base))
			goto out;
	}

	if (chip->cmd_shmem_out_base) {
		fw->sm_shmem_out_base = meson_sm_map_shmem(chip->cmd_shmem_out_base,
							   chip->shmem_size);
		if (WARN_ON(!fw->sm_shmem_out_base))
			goto unmap_in_base;
	}

	fw->chip = chip;

	dev_set_drvdata(dev, fw);

	meson_sm_set_soc_uid(fw);

	pr_info("secure-monitor enabled\n");

	return 0;

unmap_in_base:
	iounmap(fw->sm_shmem_in_base);
out:
	return -EINVAL;
}

static struct driver meson_sm_driver = {
	.name = "meson-sm",
	.of_match_table = of_match_ptr(meson_sm_ids),
	.probe = meson_sm_probe,
};
device_platform_driver(meson_sm_driver);
MODULE_DESCRIPTION("Amlogic Secure Monitor driver");
MODULE_LICENSE("GPL v2");
