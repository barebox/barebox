#include <common.h>
#include <fcntl.h>
#include <fs.h>
#include <ioctl.h>
#include "ubi-barebox.h"
#include "ubi.h"

LIST_HEAD(ubi_volumes_list);

struct ubi_volume_cdev_priv {
	struct ubi_device *ubi;
	struct ubi_volume *vol;
	int written;
};

static ssize_t ubi_volume_cdev_read(struct cdev *cdev, void *buf, size_t size,
		loff_t offset, unsigned long flags)
{
	struct ubi_volume_cdev_priv *priv = cdev->priv;
	struct ubi_volume *vol = priv->vol;
	struct ubi_device *ubi = priv->ubi;
	int err, lnum, off, len;
	size_t count_save = size;
	unsigned long long tmp;
	loff_t offp = offset;
	int usable_leb_size = vol->usable_leb_size;

	ubi_debug("%s: %zd @ 0x%08llx", __func__, size, offset);

	len = size > usable_leb_size ? usable_leb_size : size;

	tmp = offp;
	off = do_div(tmp, usable_leb_size);
	lnum = tmp;
	do {
		if (off + len >= usable_leb_size)
			len = usable_leb_size - off;

		err = ubi_eba_read_leb(ubi, vol, lnum, buf, off, len, 0);
		if (err) {
			ubi_err(ubi, "read error: %s", strerror(-err));
			break;
		}
		off += len;
		if (off == usable_leb_size) {
			lnum += 1;
			off -= usable_leb_size;
		}

		size -= len;
		offp += len;

		buf += len;
		len = size > usable_leb_size ? usable_leb_size : size;
	} while (size);

	return count_save;
}

static ssize_t ubi_volume_cdev_write(struct cdev* cdev, const void *buf,
		size_t size, loff_t offset, unsigned long flags)
{
	struct ubi_volume_cdev_priv *priv = cdev->priv;
	struct ubi_volume *vol = priv->vol;
	struct ubi_device *ubi = priv->ubi;
	int err;

	if (!priv->written) {
		err = ubi_start_update(ubi, vol, vol->used_bytes);
		if (err < 0) {
			ubi_err(ubi, "Cannot start volume update");
			return err;
		}
	}

	err = ubi_more_update_data(ubi, vol, buf, size);
	if (err < 0) {
		ubi_err(ubi, "Couldnt or partially wrote data");
		return err;
	}

	priv->written += size;

	return size;
}

static int ubi_volume_cdev_open(struct cdev *cdev, unsigned long flags)
{
	struct ubi_volume_cdev_priv *priv = cdev->priv;

	priv->written = 0;

	return 0;
}

static int ubi_volume_cdev_close(struct cdev *cdev)
{
	struct ubi_volume_cdev_priv *priv = cdev->priv;
	struct ubi_volume *vol = priv->vol;
	struct ubi_device *ubi = priv->ubi;
	int err;

	if (priv->written) {
		int remaining = vol->usable_leb_size -
				(priv->written % vol->usable_leb_size);

		if (remaining) {
			void *buf = kmalloc(remaining, GFP_KERNEL);

			if (!buf)
				return -ENOMEM;

			memset(buf, 0xff, remaining);

			err = ubi_more_update_data(ubi, vol, buf, remaining);

			kfree(buf);

			if (err < 0) {
				ubi_err(ubi, "Couldnt or partially wrote data");
				return err;
			}
		}

		err = ubi_finish_update(ubi, vol);
		if (err)
			return err;

		err = ubi_check_volume(ubi, vol->vol_id);
		if (err < 0) {
			ubi_err(ubi, "ubi volume check failed: %s", strerror(err));
			return err;
		}

		if (err) {
			ubi_warn(ubi, "volume %d on UBI device %d is corrupted",
					vol->vol_id, ubi->ubi_num);
			vol->corrupted = 1;
		}

		vol->checked = 1;
		ubi_volume_notify(ubi, vol, UBI_VOLUME_UPDATED);
	}

	return 0;
}

static loff_t ubi_volume_cdev_lseek(struct cdev *cdev, loff_t ofs)
{
	struct ubi_volume_cdev_priv *priv = cdev->priv;

	/* We can only update ubi volumes sequentially */
	if (priv->written)
		return -EINVAL;

	return ofs;
}

static struct file_operations ubi_volume_fops = {
	.open	= ubi_volume_cdev_open,
	.close	= ubi_volume_cdev_close,
	.read   = ubi_volume_cdev_read,
	.write  = ubi_volume_cdev_write,
	.lseek	= ubi_volume_cdev_lseek,
};

int ubi_volume_cdev_add(struct ubi_device *ubi, struct ubi_volume *vol)
{
	struct cdev *cdev = &vol->cdev;
	struct ubi_volume_cdev_priv *priv;
	int ret;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);

	priv->vol = vol;
	priv->ubi = ubi;

	cdev->ops = &ubi_volume_fops;
	cdev->name = basprintf("%s.%s", ubi->cdev.name, vol->name);
	cdev->priv = priv;
	cdev->size = vol->used_bytes;
	cdev->dev = &vol->dev;
	ubi_msg(ubi, "registering %s as /dev/%s", vol->name, cdev->name);
	ret = devfs_create(cdev);
	if (ret) {
		kfree(priv);
		free(cdev->name);
	}

	list_add_tail(&vol->list, &ubi_volumes_list);

	return 0;
}

void ubi_volume_cdev_remove(struct ubi_volume *vol)
{
	struct cdev *cdev = &vol->cdev;
	struct ubi_volume_cdev_priv *priv = cdev->priv;

	list_del(&vol->list);

	devfs_remove(cdev);
	unregister_device(&vol->dev);
	kfree(cdev->name);
	kfree(priv);
}

static int ubi_cdev_ioctl(struct cdev *cdev, int cmd, void *buf)
{
	struct ubi_volume_desc *desc;
	struct ubi_device *ubi = cdev->priv;
	struct ubi_mkvol_req *req = buf;

	switch (cmd) {
	case UBI_IOCRMVOL:
		desc = ubi_open_volume_nm(ubi->ubi_num, req->name,
                                           UBI_EXCLUSIVE);
		if (IS_ERR(desc))
			return PTR_ERR(desc);
		ubi_remove_volume(desc, 0);
		ubi_close_volume(desc);
		break;
	case UBI_IOCMKVOL:
		if (!req->bytes)
			req->bytes = (__s64)ubi->avail_pebs * ubi->leb_size;
		return ubi_create_volume(ubi, req);
	};

	return 0;
}

static struct file_operations ubi_fops = {
	.ioctl	= ubi_cdev_ioctl,
};

int ubi_cdev_add(struct ubi_device *ubi)
{
	struct cdev *cdev = &ubi->cdev;
	int ret;

	cdev->ops = &ubi_fops;
	cdev->name = basprintf("%s.ubi", ubi->mtd->cdev.name);
	cdev->priv = ubi;
	cdev->size = 0;

	ubi_msg(ubi, "registering /dev/%s", cdev->name);
	ret = devfs_create(cdev);
	if (ret)
		kfree(cdev->name);

	return ret;
}

void ubi_cdev_remove(struct ubi_device *ubi)
{
	struct cdev *cdev = &ubi->cdev;

	ubi_msg(ubi, "removing %s", cdev->name);

	devfs_remove(cdev);
	kfree(cdev->name);
}

static void ubi_umount_volumes(struct ubi_device *ubi)
{
	int i;

	for (i = 0; i < ubi->vtbl_slots; i++) {
		struct ubi_volume *vol = ubi->volumes[i];
		if (!vol)
			continue;
		umount_by_cdev(&vol->cdev);
	}
}

/**
 * ubi_detach - detach an UBI device
 * @ubi_num: The UBI device number
 *
 * UBI volumes used by UBIFS will be unmounted before detaching the
 * UBI device.
 *
 * @return: 0 for success, negative error code otherwise
 */
int ubi_detach(int ubi_num)
{
	struct ubi_device *ubi;

	if (ubi_num < 0 || ubi_num >= UBI_MAX_DEVICES)
		return -EINVAL;

	ubi = ubi_devices[ubi_num];
	if (!ubi)
		return -ENOENT;

	ubi_umount_volumes(ubi);

	return ubi_detach_mtd_dev(ubi_num, 1);
}

/**
 * ubi_num_get_by_mtd - find the ubi number to the given mtd
 * @mtd: the mtd device
 *
 * @return: positive or zero for a UBI number, negative error code otherwise
 */
int ubi_num_get_by_mtd(struct mtd_info *mtd)
{
	int i;
	struct ubi_device *ubi;

	for (i = 0; i < UBI_MAX_DEVICES; i++) {
		ubi = ubi_devices[i];
		if (ubi && mtd == ubi->mtd)
			return ubi->ubi_num;
	}

	return -ENOENT;
}
