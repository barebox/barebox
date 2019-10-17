/*
 * Copyright (c) International Business Machines Corp., 2006
 * Copyright (c) Nokia Corporation, 2007
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
 * the GNU General Public License for more details.
 *
 * Author: Artem Bityutskiy (Битюцкий Артём),
 *         Frank Haverkamp
 */

/*
 * This file includes UBI initialization and building of UBI devices.
 *
 * When UBI is initialized, it attaches all the MTD devices specified as the
 * module load parameters or the kernel boot parameters. If MTD devices were
 * specified, UBI does not attach any MTD device, but it is possible to do
 * later using the "UBI control device".
 */

#include <linux/err.h>
#include <linux/stringify.h>
#include <linux/stat.h>
#include <linux/log2.h>
#include "ubi.h"

/* Maximum length of the 'mtd=' parameter */
#define MTD_PARAM_LEN_MAX 64

/* Maximum value for the number of bad PEBs per 1024 PEBs */
#define MAX_MTD_UBI_BEB_LIMIT 768

/**
 * struct mtd_dev_param - MTD device parameter description data structure.
 * @name: MTD character device node path, MTD device name, or MTD device number
 *        string
 * @vid_hdr_offs: VID header offset
 * @max_beb_per1024: maximum expected number of bad PEBs per 1024 PEBs
 */
struct mtd_dev_param {
	char name[MTD_PARAM_LEN_MAX];
	int vid_hdr_offs;
	int max_beb_per1024;
};

/* MTD devices specification parameters */
#ifdef CONFIG_MTD_UBI_FASTMAP
/* UBI module parameter to enable fastmap automatically on non-fastmap images */
static bool fm_autoconvert = 1;
#endif

/* All UBI devices in system */
struct ubi_device *ubi_devices[UBI_MAX_DEVICES];

/**
 * ubi_volume_notify - send a volume change notification.
 * @ubi: UBI device description object
 * @vol: volume description object of the changed volume
 * @ntype: notification type to send (%UBI_VOLUME_ADDED, etc)
 *
 * This is a helper function which notifies all subscribers about a volume
 * change event (creation, removal, re-sizing, re-naming, updating). Returns
 * zero in case of success and a negative error code in case of failure.
 */
int ubi_volume_notify(struct ubi_device *ubi, struct ubi_volume *vol, int ntype)
{
	int ret = 0;

#ifdef CONFIG_MTD_UBI_FASTMAP

	switch (ntype) {
	case UBI_VOLUME_ADDED:
	case UBI_VOLUME_REMOVED:
	case UBI_VOLUME_RESIZED:
	case UBI_VOLUME_RENAMED:
		ret = ubi_update_fastmap(ubi);
		if (ret)
			ubi_msg(ubi, "Unable to write a new fastmap: %i", ret);
	}
#endif
	return ret;
}

/**
 * ubi_get_device - get UBI device.
 * @ubi_num: UBI device number
 *
 * This function returns UBI device description object for UBI device number
 * @ubi_num, or %NULL if the device does not exist. This function increases the
 * device reference count to prevent removal of the device. In other words, the
 * device cannot be removed if its reference count is not zero.
 */
struct ubi_device *ubi_get_device(int ubi_num)
{
	struct ubi_device *ubi;

	ubi = ubi_devices[ubi_num];
	if (!ubi)
		return NULL;

	ubi->ref_count++;

	return ubi;
}

/**
 * ubi_put_device - drop an UBI device reference.
 * @ubi: UBI device description object
 */
void ubi_put_device(struct ubi_device *ubi)
{
	ubi->ref_count--;
}

/**
 * kill_volumes - destroy all user volumes.
 * @ubi: UBI device description object
 */
static void kill_volumes(struct ubi_device *ubi)
{
	int i;

	for (i = 0; i < ubi->vtbl_slots; i++)
		if (ubi->volumes[i])
			ubi_free_volume(ubi, ubi->volumes[i]);
}

/**
 * uif_init - initialize user interfaces for an UBI device.
 * @ubi: UBI device description object
 * @ref: set to %1 on exit in case of failure if a reference to @ubi->dev was
 *       taken, otherwise set to %0
 *
 * This function initializes various user interfaces for an UBI device. If the
 * initialization fails at an early stage, this function frees all the
 * resources it allocated, returns an error, and @ref is set to %0. However,
 * if the initialization fails after the UBI device was registered in the
 * driver core subsystem, this function takes a reference to @ubi->dev, because
 * otherwise the release function ('dev_release()') would free whole @ubi
 * object. The @ref argument is set to %1 in this case. The caller has to put
 * this reference.
 *
 * This function returns zero in case of success and a negative error code in
 * case of failure.
 */
static int uif_init(struct ubi_device *ubi, int *ref)
{
	int i, err;

	*ref = 0;
	sprintf(ubi->ubi_name, UBI_NAME_STR "%d", ubi->ubi_num);

	dev_set_name(&ubi->dev, "%s.ubi", ubi->mtd->cdev.name);
	ubi->dev.id = DEVICE_ID_SINGLE;
	ubi->dev.parent = &ubi->mtd->class_dev;

	err = register_device(&ubi->dev);
	if (err)
		goto out_unreg;

	err = ubi_cdev_add(ubi);
	if (err) {
		ubi_err(ubi, "cannot add character device");
		goto out_dev;
	}

	for (i = 0; i < ubi->vtbl_slots; i++)
		if (ubi->volumes[i]) {
			err = ubi_add_volume(ubi, ubi->volumes[i]);
			if (err) {
				ubi_err(ubi, "cannot add volume %d", i);
				goto out_volumes;
			}
		}

	return 0;

out_volumes:
	kill_volumes(ubi);
	devfs_remove(&ubi->cdev);
out_dev:
	unregister_device(&ubi->dev);
out_unreg:
	ubi_err(ubi, "cannot initialize UBI %s, error %d",
		ubi->ubi_name, err);
	return err;
}

/**
 * uif_close - close user interfaces for an UBI device.
 * @ubi: UBI device description object
 *
 * Note, since this function un-registers UBI volume device objects (@vol->dev),
 * the memory allocated voe the volumes is freed as well (in the release
 * function).
 */
static void uif_close(struct ubi_device *ubi)
{
	kill_volumes(ubi);
	unregister_device(&ubi->dev);
	ubi_cdev_remove(ubi);
}

/**
 * ubi_free_internal_volumes - free internal volumes.
 * @ubi: UBI device description object
 */
void ubi_free_internal_volumes(struct ubi_device *ubi)
{
	int i;

	for (i = ubi->vtbl_slots;
	     i < ubi->vtbl_slots + UBI_INT_VOL_COUNT; i++) {
		ubi_eba_replace_table(ubi->volumes[i], NULL);
		ubi_fastmap_destroy_checkmap(ubi->volumes[i]);
		kfree(ubi->volumes[i]);
	}
}

static int get_bad_peb_limit(const struct ubi_device *ubi, int max_beb_per1024)
{
	int limit, device_pebs;
	uint64_t device_size;

	if (!max_beb_per1024)
		return 0;

	/*
	 * Here we are using size of the entire flash chip and
	 * not just the MTD partition size because the maximum
	 * number of bad eraseblocks is a percentage of the
	 * whole device and bad eraseblocks are not fairly
	 * distributed over the flash chip. So the worst case
	 * is that all the bad eraseblocks of the chip are in
	 * the MTD partition we are attaching (ubi->mtd).
	 */
	if (ubi->mtd->master)
		device_size = ubi->mtd->master->size;
	else
		device_size = ubi->mtd->size;

	device_pebs = mtd_div_by_eb(device_size, ubi->mtd);
	limit = mult_frac(device_pebs, max_beb_per1024, 1024);

	/* Round it up */
	if (mult_frac(limit, 1024, max_beb_per1024) < device_pebs)
		limit += 1;

	return limit;
}

/**
 * io_init - initialize I/O sub-system for a given UBI device.
 * @ubi: UBI device description object
 * @max_beb_per1024: maximum expected number of bad PEB per 1024 PEBs
 *
 * If @ubi->vid_hdr_offset or @ubi->leb_start is zero, default offsets are
 * assumed:
 *   o EC header is always at offset zero - this cannot be changed;
 *   o VID header starts just after the EC header at the closest address
 *     aligned to @io->hdrs_min_io_size;
 *   o data starts just after the VID header at the closest address aligned to
 *     @io->min_io_size
 *
 * This function returns zero in case of success and a negative error code in
 * case of failure.
 */
static int io_init(struct ubi_device *ubi, int max_beb_per1024)
{
	dbg_gen("sizeof(struct ubi_ainf_peb) %zu", sizeof(struct ubi_ainf_peb));
	dbg_gen("sizeof(struct ubi_wl_entry) %zu", sizeof(struct ubi_wl_entry));

	if (ubi->mtd->numeraseregions != 0) {
		/*
		 * Some flashes have several erase regions. Different regions
		 * may have different eraseblock size and other
		 * characteristics. It looks like mostly multi-region flashes
		 * have one "main" region and one or more small regions to
		 * store boot loader code or boot parameters or whatever. I
		 * guess we should just pick the largest region. But this is
		 * not implemented.
		 */
		ubi_err(ubi, "multiple regions, not implemented");
		return -EINVAL;
	}

	if (ubi->vid_hdr_offset < 0)
		return -EINVAL;

	/*
	 * Note, in this implementation we support MTD devices with 0x7FFFFFFF
	 * physical eraseblocks maximum.
	 */

	ubi->peb_size   = ubi->mtd->erasesize;
	ubi->peb_count  = mtd_div_by_eb(ubi->mtd->size, ubi->mtd);
	ubi->flash_size = ubi->mtd->size;

	if (mtd_can_have_bb(ubi->mtd)) {
		ubi->bad_allowed = 1;
		ubi->bad_peb_limit = get_bad_peb_limit(ubi, max_beb_per1024);
	}

	if (ubi->mtd->type == MTD_NORFLASH) {
		ubi_assert(ubi->mtd->writesize == 1);
		ubi->nor_flash = 1;
	}

	ubi->min_io_size = ubi->mtd->writesize;
	ubi->hdrs_min_io_size = ubi->mtd->writesize >> ubi->mtd->subpage_sft;

	/*
	 * Make sure minimal I/O unit is power of 2. Note, there is no
	 * fundamental reason for this assumption. It is just an optimization
	 * which allows us to avoid costly division operations.
	 */
	if (!is_power_of_2(ubi->min_io_size)) {
		ubi_err(ubi, "min. I/O unit (%d) is not power of 2",
			ubi->min_io_size);
		return -EINVAL;
	}

	ubi_assert(ubi->hdrs_min_io_size > 0);
	ubi_assert(ubi->hdrs_min_io_size <= ubi->min_io_size);
	ubi_assert(ubi->min_io_size % ubi->hdrs_min_io_size == 0);

	ubi->max_write_size = ubi->mtd->writebufsize;
	/*
	 * Maximum write size has to be greater or equivalent to min. I/O
	 * size, and be multiple of min. I/O size.
	 */
	if (ubi->max_write_size < ubi->min_io_size ||
	    ubi->max_write_size % ubi->min_io_size ||
	    !is_power_of_2(ubi->max_write_size)) {
		ubi_err(ubi, "bad write buffer size %d for %d min. I/O unit",
			ubi->max_write_size, ubi->min_io_size);
		return -EINVAL;
	}

	/* Calculate default aligned sizes of EC and VID headers */
	ubi->ec_hdr_alsize = ALIGN(UBI_EC_HDR_SIZE, ubi->hdrs_min_io_size);
	ubi->vid_hdr_alsize = ALIGN(UBI_VID_HDR_SIZE, ubi->hdrs_min_io_size);

	dbg_gen("min_io_size      %d", ubi->min_io_size);
	dbg_gen("max_write_size   %d", ubi->max_write_size);
	dbg_gen("hdrs_min_io_size %d", ubi->hdrs_min_io_size);
	dbg_gen("ec_hdr_alsize    %d", ubi->ec_hdr_alsize);
	dbg_gen("vid_hdr_alsize   %d", ubi->vid_hdr_alsize);

	if (ubi->vid_hdr_offset == 0)
		/* Default offset */
		ubi->vid_hdr_offset = ubi->vid_hdr_aloffset =
				      ubi->ec_hdr_alsize;
	else {
		ubi->vid_hdr_aloffset = ubi->vid_hdr_offset &
						~(ubi->hdrs_min_io_size - 1);
		ubi->vid_hdr_shift = ubi->vid_hdr_offset -
						ubi->vid_hdr_aloffset;
	}

	/* Similar for the data offset */
	ubi->leb_start = ubi->vid_hdr_offset + UBI_VID_HDR_SIZE;
	ubi->leb_start = ALIGN(ubi->leb_start, ubi->min_io_size);

	dbg_gen("vid_hdr_offset   %d", ubi->vid_hdr_offset);
	dbg_gen("vid_hdr_aloffset %d", ubi->vid_hdr_aloffset);
	dbg_gen("vid_hdr_shift    %d", ubi->vid_hdr_shift);
	dbg_gen("leb_start        %d", ubi->leb_start);

	/* The shift must be aligned to 32-bit boundary */
	if (ubi->vid_hdr_shift % 4) {
		ubi_err(ubi, "unaligned VID header shift %d",
			ubi->vid_hdr_shift);
		return -EINVAL;
	}

	/* Check sanity */
	if (ubi->vid_hdr_offset < UBI_EC_HDR_SIZE ||
	    ubi->leb_start < ubi->vid_hdr_offset + UBI_VID_HDR_SIZE ||
	    ubi->leb_start > ubi->peb_size - UBI_VID_HDR_SIZE ||
	    ubi->leb_start & (ubi->min_io_size - 1)) {
		ubi_err(ubi, "bad VID header (%d) or data offsets (%d)",
			ubi->vid_hdr_offset, ubi->leb_start);
		return -EINVAL;
	}

	/*
	 * Set maximum amount of physical erroneous eraseblocks to be 10%.
	 * Erroneous PEB are those which have read errors.
	 */
	ubi->max_erroneous = ubi->peb_count / 10;
	if (ubi->max_erroneous < 16)
		ubi->max_erroneous = 16;
	dbg_gen("max_erroneous    %d", ubi->max_erroneous);

	/*
	 * It may happen that EC and VID headers are situated in one minimal
	 * I/O unit. In this case we can only accept this UBI image in
	 * read-only mode.
	 */
	if (ubi->vid_hdr_offset + UBI_VID_HDR_SIZE <= ubi->hdrs_min_io_size) {
		ubi_warn(ubi, "EC and VID headers are in the same minimal I/O unit, switch to read-only mode");
		ubi->ro_mode = 1;
	}

	ubi->leb_size = ubi->peb_size - ubi->leb_start;

	if (!(ubi->mtd->flags & MTD_WRITEABLE)) {
		ubi_msg(ubi, "MTD device %d is write-protected, attach in read-only mode",
			ubi->mtd->index);
		ubi->ro_mode = 1;
	}

	/*
	 * Note, ideally, we have to initialize @ubi->bad_peb_count here. But
	 * unfortunately, MTD does not provide this information. We should loop
	 * over all physical eraseblocks and invoke mtd->block_is_bad() for
	 * each physical eraseblock. So, we leave @ubi->bad_peb_count
	 * uninitialized so far.
	 */

	return 0;
}

/**
 * autoresize - re-size the volume which has the "auto-resize" flag set.
 * @ubi: UBI device description object
 * @vol_id: ID of the volume to re-size
 *
 * This function re-sizes the volume marked by the %UBI_VTBL_AUTORESIZE_FLG in
 * the volume table to the largest possible size. See comments in ubi-header.h
 * for more description of the flag. Returns zero in case of success and a
 * negative error code in case of failure.
 */
static int autoresize(struct ubi_device *ubi, int vol_id)
{
	struct ubi_volume_desc desc;
	struct ubi_volume *vol = ubi->volumes[vol_id];
	int err, old_reserved_pebs = vol->reserved_pebs;

	if (ubi->ro_mode) {
		ubi_warn(ubi, "skip auto-resize because of R/O mode");
		return 0;
	}

	/*
	 * Clear the auto-resize flag in the volume in-memory copy of the
	 * volume table, and 'ubi_resize_volume()' will propagate this change
	 * to the flash.
	 */
	ubi->vtbl[vol_id].flags &= ~UBI_VTBL_AUTORESIZE_FLG;

	if (ubi->avail_pebs == 0) {
		struct ubi_vtbl_record vtbl_rec;

		/*
		 * No available PEBs to re-size the volume, clear the flag on
		 * flash and exit.
		 */
		vtbl_rec = ubi->vtbl[vol_id];
		err = ubi_change_vtbl_record(ubi, vol_id, &vtbl_rec);
		if (err)
			ubi_err(ubi, "cannot clean auto-resize flag for volume %d",
				vol_id);
	} else {
		desc.vol = vol;
		err = ubi_resize_volume(&desc,
					old_reserved_pebs + ubi->avail_pebs);
		if (err)
			ubi_err(ubi, "cannot auto-resize volume %d",
				vol_id);
	}

	if (err)
		return err;

	ubi_msg(ubi, "volume %d (\"%s\") re-sized from %d to %d LEBs",
		vol_id, vol->name, old_reserved_pebs, vol->reserved_pebs);
	return 0;
}

/**
 * ubi_attach_mtd_dev - attach an MTD device.
 * @mtd: MTD device description object
 * @ubi_num: number to assign to the new UBI device
 * @vid_hdr_offset: VID header offset
 * @max_beb_per1024: maximum expected number of bad PEB per 1024 PEBs
 *
 * This function attaches MTD device @mtd_dev to UBI and assigns @ubi_num number
 * to the newly created UBI device, unless @ubi_num is %UBI_DEV_NUM_AUTO, in
 * which case this function finds a vacant device number and assigns it
 * automatically. Returns the new UBI device number in case of success and a
 * negative error code in case of failure.
 *
 * Note, the invocation of this function has to be serialized by the
 * @ubi_devices_mutex.
 */
int ubi_attach_mtd_dev(struct mtd_info *mtd, int ubi_num,
		       int vid_hdr_offset, int max_beb_per1024)
{
	struct ubi_device *ubi;
	int i, err, ref = 0;

	if (max_beb_per1024 < 0 || max_beb_per1024 > MAX_MTD_UBI_BEB_LIMIT)
		return -EINVAL;

	if (!max_beb_per1024)
		max_beb_per1024 = CONFIG_MTD_UBI_BEB_LIMIT;

	/*
	 * Check if we already have the same MTD device attached.
	 *
	 * Note, this function assumes that UBI devices creations and deletions
	 * are serialized, so it does not take the &ubi_devices_lock.
	 */
	for (i = 0; i < UBI_MAX_DEVICES; i++) {
		ubi = ubi_devices[i];
		if (ubi && mtd == ubi->mtd) {
			pr_err("ubi: mtd%d is already attached to ubi%d\n",
				mtd->index, i);
			return -EEXIST;
		}
	}

	/*
	 * Make sure this MTD device is not emulated on top of an UBI volume
	 * already. Well, generally this recursion works fine, but there are
	 * different problems like the UBI module takes a reference to itself
	 * by attaching (and thus, opening) the emulated MTD device. This
	 * results in inability to unload the module. And in general it makes
	 * no sense to attach emulated MTD devices, so we prohibit this.
	 */
	if (mtd->type == MTD_UBIVOLUME) {
		pr_err("ubi: refuse attaching mtd%d - it is already emulated on top of UBI\n",
			mtd->index);
		return -EINVAL;
	}

	if (ubi_num == UBI_DEV_NUM_AUTO) {
		/* Search for an empty slot in the @ubi_devices array */
		for (ubi_num = 0; ubi_num < UBI_MAX_DEVICES; ubi_num++)
			if (!ubi_devices[ubi_num])
				break;
		if (ubi_num == UBI_MAX_DEVICES) {
			pr_err("ubi: only %d UBI devices may be created\n",
				UBI_MAX_DEVICES);
			return -ENFILE;
		}
	} else {
		if (ubi_num >= UBI_MAX_DEVICES)
			return -EINVAL;

		/* Make sure ubi_num is not busy */
		if (ubi_devices[ubi_num]) {
			pr_err("ubi: ubi%i already exists\n", ubi_num);
			return -EEXIST;
		}
	}

	ubi = kzalloc(sizeof(struct ubi_device), GFP_KERNEL);
	if (!ubi)
		return -ENOMEM;

	ubi->mtd = mtd;
	ubi->ubi_num = ubi_num;
	ubi->vid_hdr_offset = vid_hdr_offset;
	ubi->autoresize_vol_id = -1;

#ifdef CONFIG_MTD_UBI_FASTMAP
	ubi->fm_pool.used = ubi->fm_pool.size = 0;
	ubi->fm_wl_pool.used = ubi->fm_wl_pool.size = 0;

	/*
	 * fm_pool.max_size is 5% of the total number of PEBs but it's also
	 * between UBI_FM_MAX_POOL_SIZE and UBI_FM_MIN_POOL_SIZE.
	 */
	ubi->fm_pool.max_size = min(((int)mtd_div_by_eb(ubi->mtd->size,
		ubi->mtd) / 100) * 5, UBI_FM_MAX_POOL_SIZE);
	ubi->fm_pool.max_size = max(ubi->fm_pool.max_size,
		UBI_FM_MIN_POOL_SIZE);

	ubi->fm_wl_pool.max_size = ubi->fm_pool.max_size / 2;
	ubi->fm_disabled = !fm_autoconvert;

	if (!ubi->fm_disabled && (int)mtd_div_by_eb(ubi->mtd->size, ubi->mtd)
	    <= UBI_FM_MAX_START) {
		ubi_err(ubi, "More than %i PEBs are needed for fastmap, sorry.",
			UBI_FM_MAX_START);
		ubi->fm_disabled = 1;
	}

	ubi_debug("default fastmap pool size: %d", ubi->fm_pool.max_size);
	ubi_debug("default fastmap WL pool size: %d", ubi->fm_wl_pool.max_size);
#else
	ubi->fm_disabled = 1;
#endif
	err = io_init(ubi, max_beb_per1024);
	if (err)
		goto out_free;

	err = -ENOMEM;
	ubi->peb_buf = vmalloc(ubi->peb_size);
	if (!ubi->peb_buf)
		goto out_free;

#ifdef CONFIG_MTD_UBI_FASTMAP
	ubi->fm_size = ubi_calc_fm_size(ubi);
	ubi->fm_buf = kzalloc(ubi->fm_size, GFP_KERNEL);
	if (!ubi->fm_buf)
		goto out_free;
#endif
	err = ubi_attach(ubi, 0);
	if (err) {
		ubi_err(ubi, "failed to attach mtd%d, error %d",
			mtd->index, err);
		goto out_free;
	}

	ubi->thread_enabled = 1;

	/* No threading, call ubi_thread directly */
	ubi_thread(ubi);

	if (ubi->autoresize_vol_id != -1) {
		err = autoresize(ubi, ubi->autoresize_vol_id);
		if (err)
			goto out_detach;
	}

	/* Make device "available" before it becomes accessible via sysfs */
	ubi_devices[ubi_num] = ubi;

	err = uif_init(ubi, &ref);
	if (err)
		goto out_detach;

	ubi_msg(ubi, "attached mtd%d (name \"%s\", size %llu MiB) to ubi%d",
		mtd->index, mtd->name, ubi->flash_size >> 20, ubi_num);
	ubi_msg(ubi, "PEB size: %d bytes (%d KiB), LEB size: %d bytes",
		ubi->peb_size, ubi->peb_size >> 10, ubi->leb_size);
	ubi_msg(ubi, "min./max. I/O unit sizes: %d/%d, sub-page size %d",
		ubi->min_io_size, ubi->max_write_size, ubi->hdrs_min_io_size);
	ubi_msg(ubi, "VID header offset: %d (aligned %d), data offset: %d",
		ubi->vid_hdr_offset, ubi->vid_hdr_aloffset, ubi->leb_start);
	ubi_msg(ubi, "good PEBs: %d, bad PEBs: %d, corrupted PEBs: %d",
		ubi->good_peb_count, ubi->bad_peb_count, ubi->corr_peb_count);
	ubi_msg(ubi, "user volume: %d, internal volumes: %d, max. volumes count: %d",
		ubi->vol_count - UBI_INT_VOL_COUNT, UBI_INT_VOL_COUNT,
		ubi->vtbl_slots);
	ubi_msg(ubi, "max/mean erase counter: %d/%d, WL threshold: %d, image sequence number: %u",
		ubi->max_ec, ubi->mean_ec, UBI_WL_THRESHOLD,
		ubi->image_seq);
	ubi_msg(ubi, "available PEBs: %d, total reserved PEBs: %d, PEBs reserved for bad PEB handling: %d",
		ubi->avail_pebs, ubi->rsvd_pebs, ubi->beb_rsvd_pebs);

	dev_add_param_uint32_ro(&ubi->dev, "peb_size", &ubi->peb_size, "%u");
	dev_add_param_uint32_ro(&ubi->dev, "leb_size", &ubi->leb_size, "%u");
	dev_add_param_uint32_ro(&ubi->dev, "vid_header_offset", &ubi->vid_hdr_offset, "%u");
	dev_add_param_uint32_ro(&ubi->dev, "min_io_size", &ubi->min_io_size, "%u");
	dev_add_param_uint32_ro(&ubi->dev, "sub_page_size", &ubi->hdrs_min_io_size, "%u");
	dev_add_param_uint32_ro(&ubi->dev, "good_peb_count", &ubi->good_peb_count, "%u");
	dev_add_param_uint32_ro(&ubi->dev, "bad_peb_count", &ubi->bad_peb_count, "%u");
	dev_add_param_uint32_ro(&ubi->dev, "max_erase_counter", &ubi->max_ec, "%u");
	dev_add_param_uint32_ro(&ubi->dev, "mean_erase_counter", &ubi->mean_ec, "%u");
	dev_add_param_uint32_ro(&ubi->dev, "available_pebs", &ubi->avail_pebs, "%u");
	dev_add_param_uint32_ro(&ubi->dev, "reserved_pebs", &ubi->rsvd_pebs, "%u");

	return ubi_num;

out_detach:
	ubi_devices[ubi_num] = NULL;
	ubi_wl_close(ubi);
	ubi_free_internal_volumes(ubi);
	vfree(ubi->vtbl);
out_free:
	vfree(ubi->peb_buf);
	vfree(ubi->fm_buf);
	kfree(ubi);
	return err;
}

/**
 * ubi_detach_mtd_dev - detach an MTD device.
 * @ubi_num: UBI device number to detach from
 * @anyway: detach MTD even if device reference count is not zero
 *
 * This function destroys an UBI device number @ubi_num and detaches the
 * underlying MTD device. Returns zero in case of success and %-EBUSY if the
 * UBI device is busy and cannot be destroyed, and %-EINVAL if it does not
 * exist.
 *
 * Note, the invocation of this function has to be serialized by the
 * @ubi_devices_mutex.
 */
int ubi_detach_mtd_dev(int ubi_num, int anyway)
{
	struct ubi_device *ubi;

	if (ubi_num < 0 || ubi_num >= UBI_MAX_DEVICES)
		return -EINVAL;

	ubi = ubi_get_device(ubi_num);
	if (!ubi)
		return -EINVAL;

	ubi->ref_count--;

	if (ubi->ref_count)
		return -EBUSY;

	ubi_devices[ubi_num] = NULL;

	ubi_assert(ubi_num == ubi->ubi_num);

	ubi_msg(ubi, "detaching mtd%d from ubi%d", ubi->mtd->index, ubi_num);
#ifdef CONFIG_MTD_UBI_FASTMAP
	/* If we don't write a new fastmap at detach time we lose all
	 * EC updates that have been made since the last written fastmap. */
	ubi_update_fastmap(ubi);
#endif

	uif_close(ubi);

	ubi_wl_close(ubi);
	ubi_free_internal_volumes(ubi);
	vfree(ubi->vtbl);
	vfree(ubi->peb_buf);
	vfree(ubi->fm_buf);
	ubi_msg(ubi, "mtd%d is detached", ubi->mtd->index);
	kfree(ubi);

	return 0;
}
