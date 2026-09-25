/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: 2026 Tobias Waldekranz <tobias@waldekranz.com> */

#ifndef _LVM_H
#define _LVM_H

#include <linux/types.h>

#include <disks.h>

struct cdev;

struct lvm_pv;
struct lvm_lv;

#define LVM_UUID_LEN 32

/* Delinearized representation of a Volume Group (VG) with references
 * to associated Physical (PVs) and Locical (LVs) Volumes
 */
struct lvm_vg {
	char *name;
	char uuid[LVM_UUID_LEN + 1];

	u64 seqno;

	/* Sectors per physical extent */
	blkcnt_t pe_size;

	struct lvm_pv **pvs;
	size_t num_pvs;

	struct lvm_lv **lvs;
	size_t num_lvs;
};

struct lvm_lv *lvm_vg_lv_by_name(struct lvm_vg *vg, const char *name);

/* Iterator over all available VGs, constructed by scanning all block
 * devices known to the system. If multiple PVs contain metadata for
 * the same VG, only the most recently updated version is returned to
 * the caller of lvm_vg_iter_next(). Callers must ensure that any
 * returned VGs are freed by calling lvm_vg_free().
 */
struct lvm_vg_iter;

struct lvm_vg *lvm_vg_iter_next(struct lvm_vg_iter *iter);
struct lvm_vg_iter *lvm_vg_iter_new(void);
void lvm_vg_iter_free(struct lvm_vg_iter *iter);

/* Return a VG based on its name. Internally this uses a VG iterator
 * and is thus more expensive than lvm_vg_alloc_by_cdev(), with the
 * upside that it is based on the most up-to-date metadata available.
 */
int lvm_vg_alloc_by_name(const char *name, struct lvm_vg **vgp);

/* Return the VG described by the metadata on the PV backed by
 * cdev.
 */
int lvm_vg_alloc_by_cdev(struct cdev *cdev, struct lvm_vg **vgp);

void lvm_vg_free(struct lvm_vg *vg);

struct lvm_pv {
	struct lvm_vg *vg;

	char *name;
	char uuid[LVM_UUID_LEN + 1];

	blkcnt_t dev_size;
	blkcnt_t pe_start;
	u32 pe_count;
};

/* Return the backing device for pv */
struct cdev *lvm_pv_cdev(struct lvm_pv *pv);

enum lvm_lv_type {
	LVM_LV_UNKNOWN,
	LVM_LV_LINEAR,
};

struct lvm_lv {
	struct lvm_vg *vg;
	enum lvm_lv_type type;
	char *name;
	char uuid[LVM_UUID_LEN + 1];

	blkcnt_t size;
};

/* Create a device mapper configuration table for the specified LV,
 * suitable for consumption by dm_create(). The caller takes ownership
 * of the returned string. Returns an ERR_PTR() on failure, e.g. if
 * the LV uses an unsupported mapping or references a PV that is not
 * present.
 */
char *lvm_lv_dm_ctable(struct lvm_lv *lv);

#endif	/* _LVM_H */
