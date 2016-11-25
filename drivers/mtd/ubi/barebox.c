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

	if (!priv->written && !vol->updating) {
		if (vol->vol_type == UBI_STATIC_VOLUME)
			return -EROFS;

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

		if (remaining && vol->vol_type == UBI_DYNAMIC_VOLUME) {
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

		if (vol->vol_type == UBI_STATIC_VOLUME)
			cdev->size = priv->written;

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

static int ubi_volume_cdev_truncate(struct cdev *cdev, size_t size)
{
	struct ubi_volume_cdev_priv *priv = cdev->priv;
	struct ubi_device *ubi = priv->ubi;
	struct ubi_volume *vol = priv->vol;
	uint64_t rsvd_bytes;

	rsvd_bytes = (long long)vol->reserved_pebs *
			ubi->leb_size - vol->data_pad;
	if (size > rsvd_bytes)
		return -ENOSPC;

	return 0;
}

static int ubi_volume_cdev_ioctl(struct cdev *cdev, int cmd, void *buf)
{
	struct ubi_volume_cdev_priv *priv = cdev->priv;
	struct ubi_device *ubi = priv->ubi;
	struct ubi_volume *vol = priv->vol;
	int err = 0;

	switch (cmd) {
	/* Volume update command */
	case UBI_IOCVOLUP:
	{
		int64_t bytes, rsvd_bytes;

		err = copy_from_user(&bytes, buf, sizeof(int64_t));
		if (err) {
			err = -EFAULT;
			break;
		}

		rsvd_bytes = (long long)vol->reserved_pebs *
				ubi->leb_size - vol->data_pad;

		if (bytes < 0 || bytes > rsvd_bytes) {
			err = -EINVAL;
			break;
		}

		err = ubi_start_update(ubi, vol, bytes);
		if (bytes == 0)
			ubi_volume_notify(ubi, vol, UBI_VOLUME_UPDATED);

		break;
	}

	default:
		err = -ENOTTY;
		break;
	}
	return err;
}

static struct file_operations ubi_volume_fops = {
	.open	= ubi_volume_cdev_open,
	.close	= ubi_volume_cdev_close,
	.read   = ubi_volume_cdev_read,
	.write  = ubi_volume_cdev_write,
	.lseek	= ubi_volume_cdev_lseek,
	.ioctl  = ubi_volume_cdev_ioctl,
	.truncate = ubi_volume_cdev_truncate,
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

int ubi_api_create_volume(int ubi_num, struct ubi_mkvol_req *req)
{
	struct ubi_device *ubi;
	int ret;

	ubi = ubi_get_device(ubi_num);
	if (!ubi)
		return -ENODEV;

	if (!req->bytes)
		req->bytes = (__s64)ubi->avail_pebs * ubi->leb_size;
	ret = ubi_create_volume(ubi, req);

	ubi_put_device(ubi);

	return ret;
}

int ubi_api_remove_volume(struct ubi_volume_desc *desc, int no_vtbl)
{
	return ubi_remove_volume(desc, no_vtbl);
}

int ubi_api_rename_volumes(int ubi_num, struct ubi_rnvol_req *req)
{
	int i, n, err;
	struct list_head rename_list;
	struct ubi_rename_entry *re, *re1;
	struct ubi_device *ubi;

	if (req->count < 0 || req->count > UBI_MAX_RNVOL)
		return -EINVAL;

	if (req->count == 0)
		return 0;

	ubi = ubi_get_device(ubi_num);
	if (!ubi)
		return -ENODEV;

	/* Validate volume IDs and names in the request */
	for (i = 0; i < req->count; i++) {
		if (req->ents[i].vol_id < 0 ||
		    req->ents[i].vol_id >= ubi->vtbl_slots)
			return -EINVAL;
		if (req->ents[i].name_len < 0)
			return -EINVAL;
		if (req->ents[i].name_len > UBI_VOL_NAME_MAX)
			return -ENAMETOOLONG;
		req->ents[i].name[req->ents[i].name_len] = '\0';
		n = strlen(req->ents[i].name);
		if (n != req->ents[i].name_len)
			return -EINVAL;
	}

	/* Make sure volume IDs and names are unique */
	for (i = 0; i < req->count - 1; i++) {
		for (n = i + 1; n < req->count; n++) {
			if (req->ents[i].vol_id == req->ents[n].vol_id) {
				ubi_err(ubi, "duplicated volume id %d",
					req->ents[i].vol_id);
				return -EINVAL;
			}
			if (!strcmp(req->ents[i].name, req->ents[n].name)) {
				ubi_err(ubi, "duplicated volume name \"%s\"",
					req->ents[i].name);
				return -EINVAL;
			}
		}
	}

	/* Create the re-name list */
	INIT_LIST_HEAD(&rename_list);
	for (i = 0; i < req->count; i++) {
		int vol_id = req->ents[i].vol_id;
		int name_len = req->ents[i].name_len;
		const char *name = req->ents[i].name;

		re = kzalloc(sizeof(struct ubi_rename_entry), GFP_KERNEL);
		if (!re) {
			err = -ENOMEM;
			goto out_free;
		}

		re->desc = ubi_open_volume(ubi->ubi_num, vol_id, UBI_READONLY);
		if (IS_ERR(re->desc)) {
			err = PTR_ERR(re->desc);
			ubi_err(ubi, "cannot open volume %d, error %d",
				vol_id, err);
			kfree(re);
			goto out_free;
		}

		/* Skip this re-naming if the name does not really change */
		if (re->desc->vol->name_len == name_len &&
		    !memcmp(re->desc->vol->name, name, name_len)) {
			ubi_close_volume(re->desc);
			kfree(re);
			continue;
		}

		re->new_name_len = name_len;
		memcpy(re->new_name, name, name_len);
		list_add_tail(&re->list, &rename_list);
		dbg_gen("will rename volume %d from \"%s\" to \"%s\"",
			vol_id, re->desc->vol->name, name);
	}

	if (list_empty(&rename_list))
		return 0;

	/* Find out the volumes which have to be removed */
	list_for_each_entry(re, &rename_list, list) {
		struct ubi_volume_desc *desc;
		int no_remove_needed = 0;

		/*
		 * Volume @re->vol_id is going to be re-named to
		 * @re->new_name, while its current name is @name. If a volume
		 * with name @re->new_name currently exists, it has to be
		 * removed, unless it is also re-named in the request (@req).
		 */
		list_for_each_entry(re1, &rename_list, list) {
			if (re->new_name_len == re1->desc->vol->name_len &&
			    !memcmp(re->new_name, re1->desc->vol->name,
				    re1->desc->vol->name_len)) {
				no_remove_needed = 1;
				break;
			}
		}

		if (no_remove_needed)
			continue;

		/*
		 * It seems we need to remove volume with name @re->new_name,
		 * if it exists.
		 */
		desc = ubi_open_volume_nm(ubi->ubi_num, re->new_name,
					  UBI_EXCLUSIVE);
		if (IS_ERR(desc)) {
			err = PTR_ERR(desc);
			if (err == -ENODEV)
				/* Re-naming into a non-existing volume name */
				continue;

			/* The volume exists but busy, or an error occurred */
			ubi_err(ubi, "cannot open volume \"%s\", error %d",
				re->new_name, err);
			goto out_free;
		}

		re1 = kzalloc(sizeof(struct ubi_rename_entry), GFP_KERNEL);
		if (!re1) {
			err = -ENOMEM;
			ubi_close_volume(desc);
			goto out_free;
		}

		re1->remove = 1;
		re1->desc = desc;
		list_add(&re1->list, &rename_list);
		dbg_gen("will remove volume %d, name \"%s\"",
			re1->desc->vol->vol_id, re1->desc->vol->name);
	}

	mutex_lock(&ubi->device_mutex);
	err = ubi_rename_volumes(ubi, &rename_list);
	mutex_unlock(&ubi->device_mutex);

out_free:
	list_for_each_entry_safe(re, re1, &rename_list, list) {
		ubi_close_volume(re->desc);
		list_del(&re->list);
		kfree(re);
	}
	return err;
}

static int ubi_cdev_ioctl(struct cdev *cdev, int cmd, void *buf)
{
	struct ubi_device *ubi = cdev->priv;

	switch (cmd) {
	/*
	 * Only supported ioctl is a barebox specific one to get the ubi_num
	 * from the file descriptor. The rest is implemented as function calls
	 * directly.
	 */
	case UBI_IOCGETUBINUM:
		*(u32 *)buf = ubi->ubi_num;
		break;
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
