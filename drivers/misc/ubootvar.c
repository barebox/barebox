// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * U-Boot environment vriable blob driver
 *
 * Copyright (C) 2019 Zodiac Inflight Innovations
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <malloc.h>
#include <envfs.h>
#include <fs.h>
#include <libfile.h>
#include <command.h>
#include <crc.h>

enum ubootvar_flag_scheme {
	FLAG_NONE,
	FLAG_BOOLEAN,
	FLAG_INCREMENTAL,
};

struct ubootvar_data {
	struct cdev cdev;
	char *path[2];
	bool current;
	uint8_t flag;
	int count;
	void *data;
	size_t size;
};

static int ubootvar_flush(struct cdev *cdev)
{
	struct device *dev = cdev->dev;
	struct ubootvar_data *ubdata = dev->priv;
	const char *path = ubdata->path[!ubdata->current];
	uint32_t crc = 0xffffffff;
	resource_size_t size;
	const void *data;
	int fd, ret = 0;

	fd = open(path, O_WRONLY);
	if (fd < 0) {
		dev_err(dev, "Failed to open %s\n", path);
		return -errno;
	}
	/*
	 * FIXME: This code needs to do a proper protect/unprotect and
	 * erase calls to work on MTD devices
	 */

	/*
	 * Write a dummy CRC first as a way of invalidating the
	 * environment in case we fail mid-flushing
	 */
	if (write_full(fd, &crc, sizeof(crc)) != sizeof(crc)) {
		dev_err(dev, "Failed to write dummy CRC\n");
		ret = -errno;
		goto close_fd;
	}

	if (ubdata->count > 1) {
		/*
		 * FIXME: This assumes FLAG_INCREMENTAL
		 */
		const uint8_t flag = ++ubdata->flag;

		if (write_full(fd, &flag, sizeof(flag)) != sizeof(flag)) {
			dev_dbg(dev, "Failed to write flag\n");
			ret = -errno;
			goto close_fd;
		}
	}

	data = (const void *)ubdata->data;
	size = ubdata->size;

	/*
	 * Write out and flush all of the new environment data
	 */
	if (write_full(fd, data, size) != size) {
		dev_dbg(dev, "Failed to write data\n");
		ret = -errno;
		goto close_fd;
	}

	if (flush(fd)) {
		dev_dbg(dev, "Failed to flush written data\n");
		ret = -errno;
		goto close_fd;
	}
	/*
	 * Now that all of the environment data is out, we can go back
	 * to the start of the block and write correct CRC, to finish
	 * the processs.
	 */
	if (lseek(fd, 0, SEEK_SET) != 0) {
		dev_dbg(dev, "lseek() failed\n");
		ret = -errno;
		goto close_fd;
	}

	crc = crc32(0, data, size);
	if (write_full(fd, &crc, sizeof(crc)) != sizeof(crc)) {
		dev_dbg(dev, "Failed to write valid CRC\n");
		ret = -errno;
		goto close_fd;
	}
	/*
	 * Now that we've successfully written new environment blob
	 * out, switch current partition.
	 */
	ubdata->current = !ubdata->current;

close_fd:
	close(fd);
	return ret;
}

static ssize_t
ubootvar_read(struct cdev *cdev, void *buf, size_t count, loff_t offset,
	      unsigned long flags)
{
	struct device *dev = cdev->dev;
	struct ubootvar_data *ubdata = dev->priv;

	WARN_ON(flags & O_RWSIZE_MASK);

	memcpy(buf, ubdata->data + offset, count);

	return count;
}

static ssize_t
ubootvar_write(struct cdev *cdev, const void *buf, size_t count,
	       loff_t offset, unsigned long flags)
{
	struct device *dev = cdev->dev;
	struct ubootvar_data *ubdata = dev->priv;

	WARN_ON(flags & O_RWSIZE_MASK);

	memcpy(ubdata->data + offset, buf, count);

	return count;
}

static int ubootvar_memmap(struct cdev *cdev, void **map, int flags)
{
	struct device *dev = cdev->dev;
	struct ubootvar_data *ubdata = dev->priv;

	*map = ubdata->data;

	return 0;
}

static struct cdev_operations ubootvar_ops = {
	.read = ubootvar_read,
	.write = ubootvar_write,
	.memmap = ubootvar_memmap,
	.flush = ubootvar_flush,
};

static void ubootenv_info(struct device *dev)
{
	struct ubootvar_data *ubdata = dev->priv;

	printf("Current environment copy: %s\n",
	       ubdata->path[ubdata->current]);
}

static int ubootenv_probe(struct device *dev)
{
	struct ubootvar_data *ubdata;
	unsigned int crc_ok = 0;
	int ret, i, current, count = 0;
	uint32_t crc[2];
	uint8_t flag[2] = { FLAG_NONE, FLAG_NONE };
	size_t size[2];
	void *blob[2] = { NULL, NULL };
	uint8_t *data[2];

	/*
	 * FIXME: Flag scheme is determined by the type of underlined
	 * non-volatible device, so it should probably come from
	 * Device Tree binding. Currently we just assume incremental
	 * scheme since that is what is used on SD/eMMC devices.
	 */
	enum ubootvar_flag_scheme flag_scheme = FLAG_INCREMENTAL;

	ubdata = xzalloc(sizeof(*ubdata));

	ret = of_find_path(dev->of_node, "device-path-0",
			   &ubdata->path[0],
			   OF_FIND_PATH_FLAGS_BB);
	if (ret)
		ret = of_find_path(dev->of_node, "device-path",
				   &ubdata->path[0],
				   OF_FIND_PATH_FLAGS_BB);

	if (ret) {
		dev_err(dev, "Failed to find first device\n");
		goto out;
	}

	count++;

	if (!of_find_path(dev->of_node, "device-path-1",
			  &ubdata->path[1],
			  OF_FIND_PATH_FLAGS_BB)) {
		count++;
	} else {
		/*
		 * If there's no redundant environment partition we
		 * configure both paths to point to the same device,
		 * so that writing logic could stay the same for both
		 * redundant and non-redundant cases
		 */
		ubdata->path[1] = strdup(ubdata->path[0]);
	}

	for (i = 0; i < count; i++) {
		data[i] = blob[i] = read_file(ubdata->path[i], &size[i]);
		if (!blob[i]) {
			dev_err(dev, "Failed to read U-Boot environment\n");
			ret = -EIO;
			goto out;
		}

		crc[i] = *(uint32_t *)data[i];

		size[i] -= sizeof(uint32_t);
		data[i] += sizeof(uint32_t);

		if (count > 1) {
			/*
			 * When used in primary/redundant
			 * configuration, environment header has an
			 * additional "flag" byte
			 */
			flag[i] = *data[i];
			size[i] -= sizeof(uint8_t);
			data[i] += sizeof(uint8_t);
		}

		crc_ok |= (crc32(0, data[i], size[i]) == crc[i]) << i;
	}

	switch (crc_ok) {
	case 0b00:
		current = 0;
		memset(data[0], 0, size[0]);
		dev_info(dev, "No good partitions found, creating an empty one\n");
		break;
	case 0b11:
		/*
		 * Both partition are valid, so we need to examine
		 * flags to determine which one to use as current
		 */
		switch (flag_scheme) {
		case FLAG_INCREMENTAL:
			if ((flag[0] == 0xff && flag[1] == 0) ||
			    (flag[1] == 0xff && flag[0] == 0)) {
				/*
				 * When flag overflow happens current
				 * partition is the one whose counter
				 * reached zero first. That is if
				 * flag[1] == 0 is true (1), then i
				 * would be 1 as well
				 */
				current = flag[1] == 0;
			} else {
				/*
				 * In no-overflow case the partition
				 * with higher flag value is
				 * considered current
				 */
				current = flag[1] > flag[0];
			}
			break;
		default:
			ret = -EINVAL;
			dev_err(dev, "Unknown flag scheme %u\n", flag_scheme);
			goto out;
		}
		break;
	default:
		/*
		 * Only one partition is valid, so the choice of the
		 * current one is obvious
		 */
		current = __ffs(crc_ok);
		break;
	};

	ubdata->data = data[current];
	ubdata->size = size[current];

	ubdata->cdev.name = basprintf("ubootvar%d",
				      cdev_find_free_index("ubootvar"));
	ubdata->cdev.size = size[current];
	ubdata->cdev.ops = &ubootvar_ops;
	ubdata->cdev.dev = dev;
	ubdata->cdev.filetype = filetype_ubootvar;
	ubdata->current = current;
	ubdata->count = count;
	ubdata->flag = flag[current];

	dev->priv = ubdata;

	ret = devfs_create(&ubdata->cdev);
	if (ret) {
		dev_err(dev, "Failed to create corresponding cdev\n");
		goto out;
	}

	cdev_create_default_automount(&ubdata->cdev);

	if (count > 1) {
		/*
		 * We won't be using read data from redundant
		 * parttion, so we may as well free at this point
		 */
		free(blob[!current]);
	}

	dev->info = ubootenv_info;

	return 0;
out:
	for (i = 0; i < count; i++)
		free(blob[i]);

	free(ubdata->path[0]);
	free(ubdata->path[1]);
	free(ubdata);

	return ret;
}

static struct of_device_id ubootenv_dt_ids[] = {
	{
		.compatible = "barebox,uboot-environment",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, ubootenv_dt_ids);

static struct driver ubootenv_driver = {
	.name		= "uboot-environment",
	.probe		= ubootenv_probe,
	.of_compatible	= ubootenv_dt_ids,
};
late_platform_driver(ubootenv_driver);
