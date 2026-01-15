// SPDX-License-Identifier: GPL-2.0-only
/* Common QEMU fw_cfg board code */

#include <fs.h>
#include <init.h>
#include <driver.h>
#include <xfuncs.h>
#include <envfs.h>
#include <machine_id.h>
#include <barebox-info.h>
#include <linux/uuid.h>
#include <linux/qemu_fw_cfg.h>

struct fw_cfg_cdev {
	struct cdev cdev;
	u16 size_selector;
	u16 data_selector;
	void *cache;
	size_t cached_size;	/* How many bytes from start are cached */
};

static struct fw_cfg_cdev *to_fw_cfg_cdev(struct cdev *cdev)
{
	return container_of(cdev, struct fw_cfg_cdev, cdev);
}

static int qemu_fw_cfg_read(int fd, u16 selector, void *buf, size_t size)
{
	ssize_t ret;

	ret = ioctl(fd, FW_CFG_SELECT, &selector);
	if (ret || !buf)
		return ret;

	ret = pread(fd, buf, size, 0);
	return ret < 0 ? ret : 0;
}

static ssize_t fw_cfg_cdev_read(struct cdev *cdev, void *buf, size_t count,
				loff_t offset, ulong flags)
{
	struct fw_cfg_cdev *fcc = to_fw_cfg_cdev(cdev);
	size_t end_pos = offset + count;
	size_t to_read, to_copy;
	int ret;

	/* Bounds check */
	if (offset >= cdev->size)
		return 0;

	if (end_pos > cdev->size)
		end_pos = cdev->size;

	count = end_pos - offset;

	/* Allocate cache on first read */
	if (!fcc->cache) {
		fcc->cache = malloc(cdev->size);
		if (!fcc->cache)
			return -ENOMEM;
		fcc->cached_size = 0;
	}

	/* Extend cache if needed */
	if (end_pos > fcc->cached_size) {
		int fd;

		to_read = end_pos - fcc->cached_size;

		fd = open("/dev/fw_cfg", O_RDWR);
		if (fd < 0)
			return fd;

		ret = qemu_fw_cfg_read(fd, fcc->data_selector,
				       fcc->cache + fcc->cached_size, to_read);

		close(fd);

		if (ret)
			return ret;

		fcc->cached_size = end_pos;
	}

	/* Copy from cache to user buffer */
	to_copy = count;

	if (buf)
		memcpy(buf, fcc->cache + offset, to_copy);

	return to_copy;
}

static int fw_cfg_cdev_mmap(struct cdev *cdev, void **map, int flags)
{
	struct fw_cfg_cdev *fcc = to_fw_cfg_cdev(cdev);
	ssize_t ret;

	if (flags & PROT_WRITE)
		return -EACCES;

	ret = fw_cfg_cdev_read(cdev, NULL, ~0, 0, 0);
	if (ret == 0)
		ret = -EINVAL;
	if (ret < 0)
		return ret;

	*map = fcc->cache;

	return PTR_ERR_OR_ZERO(*map);
}

static int fw_cfg_cdev_flush(struct cdev *cdev)
{
	struct fw_cfg_cdev *fcc = to_fw_cfg_cdev(cdev);

	if (fcc->cache) {
		free(fcc->cache);
		fcc->cache = NULL;
		fcc->cached_size = 0;
	}

	return 0;
}

static struct cdev_operations fw_cfg_cdev_ops = {
	.read = fw_cfg_cdev_read,
	.flush = fw_cfg_cdev_flush,
	.memmap = fw_cfg_cdev_mmap,
};

static int fw_cfg_register_cdev(int fd, const char *name,
				u16 size_selector, u16 data_selector)
{
	struct fw_cfg_cdev *fcc;
	u32 size;
	int ret;

	/* Read size upfront */
	ret = qemu_fw_cfg_read(fd, size_selector, &size, sizeof(size));
	if (!ret && !size)
		ret = -ENOENT;
	if (ret < 0)
		return ret;

	fcc = xzalloc(sizeof(*fcc));
	fcc->size_selector = size_selector;
	fcc->data_selector = data_selector;
	fcc->cache = xzalloc(size);
	fcc->cached_size = size;

	ret = qemu_fw_cfg_read(fd, data_selector,
			       fcc->cache, fcc->cached_size);
	if (ret)
		return ret;

	fcc->cdev.name = xstrdup(name);
	fcc->cdev.size = size;
	fcc->cdev.ops = &fw_cfg_cdev_ops;

	ret = devfs_create(&fcc->cdev);
	if (ret < 0) {
		free(fcc->cdev.name);
		free(fcc);
		return ret;
	}

	return 0;
}

static int qemu_fw_cfg_boot_init(void)
{
	struct {
		const char prefix[16];
		uuid_t uuid;
	} machine_hashable = { "qemubareboxuuid"};
	__always_unused size_t size;
	int fd, ret;

	fd = open("/dev/fw_cfg", O_RDWR);
	if (fd < 0)
		return 0;

	/* Specified by -uuid. Zero by default */
	ret = qemu_fw_cfg_read(fd, FW_CFG_UUID, &machine_hashable.uuid,
			       sizeof(machine_hashable.uuid));
	if (!ret) {
		barebox_set_product_uuid(&machine_hashable.uuid);
		if (!machine_id_get_hashable(&size))
			machine_id_set_hashable(&machine_hashable, sizeof(machine_hashable));
	}

	/* Register fw_cfg cdevs */
	fw_cfg_register_cdev(fd, "fw_cfg.kernel", FW_CFG_KERNEL_SIZE, FW_CFG_KERNEL_DATA);
	fw_cfg_register_cdev(fd, "fw_cfg.initrd", FW_CFG_INITRD_SIZE, FW_CFG_INITRD_DATA);
	fw_cfg_register_cdev(fd, "fw_cfg.cmdline", FW_CFG_CMDLINE_SIZE, FW_CFG_CMDLINE_DATA);

	defaultenv_append_directory(defaultenv_qemu_fw_cfg);

	/* Reset cdev to beginning */
	ret = qemu_fw_cfg_read(fd, FW_CFG_SIGNATURE, NULL, 0);
	if (ret)
		return ret;

	close(fd);
	return 0;
}
crypto_initcall(qemu_fw_cfg_boot_init);
