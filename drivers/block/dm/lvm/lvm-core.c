// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2026 Tobias Waldekranz <tobias@waldekranz.com>

#include <block.h>
#include <disks.h>
#include <driver.h>
#include <lvm.h>
#include <qsort.h>
#include <stdio.h>
#include <string.h>
#include <xfuncs.h>

#include <asm/unaligned.h>

#include <linux/ctype.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/sprintf.h>
#include <linux/types.h>

#include "lvm2.h"
#include "lvm-md.h"

struct lvm_pv_priv {
	struct lvm_pv pv;

	struct cdev *cdev;

	/* Only populated for a standalone PV returned by lvm_pv_alloc();
	 * the PVs hanging off a VG share the originating PV's metadata.
	 */
	char *text;
	struct lvm_md *md;
};
#define to_pv_priv(_pv)	container_of((_pv), struct lvm_pv_priv, pv)

struct lvm_seg {
	struct lvm_pv *pv;

	sector_t start;		/* logical start, in sectors */
	blkcnt_t len;		/* in sectors */
	sector_t phys;		/* physical start on the PV, in sectors */
};

struct lvm_lv_priv {
	struct lvm_lv lv;

	struct lvm_seg *segs;
	size_t num_segs;
};
#define to_lv_priv(_lv)	container_of((_lv), struct lvm_lv_priv, lv)

static void lvm_uuid_strcpy(char dst[LVM_UUID_LEN + 1], const char *src, size_t len)
{
	size_t i = 0;

	while (len-- && i < LVM_UUID_LEN) {
		if (isalnum(*src))
			dst[i++] = *src;
		src++;
	}
	dst[i] = '\0';
}

/* Look for a valid LVM2 label in any of the first 4 sectors of cdev,
 * return the first one found along with the offset to the PV header.
 */
static int lvm_read_label(struct cdev *cdev, u8 *sector, u32 *pv_offset)
{
	struct lvm2_label *label = (void *)sector;
	int s;

	for (s = 0; s < LVM2_LABEL_SCAN_SECTORS; s++) {
		if (cdev_read(cdev, sector, SECTOR_SIZE,
			      (loff_t)s << SECTOR_SHIFT, 0) != SECTOR_SIZE)
			return -EIO;

		if (memcmp(label->id, LVM2_LABEL_ID, sizeof(label->id)))
			continue;

		/* A label that was copied or relocated says it lives
		 * somewhere else, and must not be used.
		 */
		if (get_unaligned_le64(&label->sector) != (u64)s)
			continue;

		/* The CRC covers everything after the crc field itself. */
		if (lvm2_crc(&label->pv_offset,
			     SECTOR_SIZE - offsetof(struct lvm2_label, pv_offset))
		    != get_unaligned_le32(&label->crc))
			continue;

		if (memcmp(label->type, LVM2_LABEL_TYPE, sizeof(label->type)))
			continue;

		*pv_offset = get_unaligned_le32(&label->pv_offset);
		if (*pv_offset >= SECTOR_SIZE - sizeof(struct lvm2_pv_header))
			continue;

		return 0;
	}

	return -ENOENT;
}

/* Read the committed text metadata document from one metadata area,
 * validating its checksum. Returns a NUL-terminated, freshly allocated
 * buffer and its length, or an error.
 */
static int lvm_read_mda(struct cdev *cdev, u64 mda_offset, u64 mda_size,
			char **textp, size_t *lenp)
{
	struct lvm2_md_header *hdr;
	struct lvm2_md_area *area;
	u8 hbuf[SECTOR_SIZE];
	u64 off, size, wrap;
	char *text;
	u32 flags;

	if (cdev_read(cdev, hbuf, sizeof(hbuf), mda_offset, 0) != sizeof(hbuf))
		return -EIO;

	hdr = (void *)hbuf;
	if (memcmp(hdr->magic, LVM2_MDA_MAGIC, sizeof(hdr->magic)))
		return -EILSEQ;

	/* The checksum covers the whole header, i.e. the raw location
	 * descriptors as well, but not the checksum itself.
	 */
	if (lvm2_crc(hdr->magic, SECTOR_SIZE - offsetof(struct lvm2_md_header, magic))
	    != get_unaligned_le32(&hdr->checksum))
		return -EILSEQ;

	if (get_unaligned_le32(&hdr->version) != LVM2_MDA_VERSION)
		return -ENOTSUPP;

	/* The first descriptor points at the committed metadata. Any
	 * further ones describe a document that LVM was in the middle
	 * of committing, which we have no business activating.
	 */
	area = &hdr->area[0];
	off = get_unaligned_le64(&area->offset);
	size = get_unaligned_le64(&area->size);
	flags = get_unaligned_le32(&area->flags);

	if (!size || (flags & LVM2_RAW_LOCN_IGNORED))
		return -ENOENT;

	/* The metadata lives in a ring buffer that starts right after
	 * the header, and a document may wrap around the end of it.
	 */
	if (off < SECTOR_SIZE || off >= mda_size || size > mda_size)
		return -EILSEQ;

	text = malloc(size + 1);
	if (!text)
		return -ENOMEM;

	if (off + size > mda_size) {
		wrap = mda_size - off;
		if (cdev_read(cdev, text, wrap, mda_offset + off, 0) != (ssize_t)wrap)
			goto err_free_text;
		if (cdev_read(cdev, text + wrap, size - wrap, mda_offset + SECTOR_SIZE, 0)
		    != (ssize_t)(size - wrap))
			goto err_free_text;
	} else {
		if (cdev_read(cdev, text, size, mda_offset + off, 0) != (ssize_t)size)
			goto err_free_text;
	}

	if (lvm2_crc(text, size) != get_unaligned_le32(&area->checksum))
		goto err_free_text;

	text[size] = '\0';
	*textp = text;
	*lenp = size;
	return 0;

err_free_text:
	free(text);
	return -EILSEQ;
}

/* Read label + pv_header + committed metadata from cdev. On success
 * the raw 32-byte PV uuid is returned in uuid[] along with the parsed
 * metadata document.
 */
static int lvm_pv_probe(struct cdev *cdev, char uuid[LVM_UUID_LEN + 1],
			char **textp, struct lvm_md **mdp)
{
	u64 mda_off[8], mda_size[8];
	struct lvm2_pv_header *pvh;
	int err, i, num_mda = 0;
	struct lvm2_area *area;
	u8 sector[SECTOR_SIZE];
	u32 pv_offset;
	char *text;
	size_t len;

	err = lvm_read_label(cdev, sector, &pv_offset);
	if (err)
		return err;

	pvh = (void *)(sector + pv_offset);
	lvm_uuid_strcpy(uuid, (char *)pvh->uuid, sizeof(pvh->uuid));

	/* Layout is:
	 *
	 * [DATA-AREA-0]
	 * ...
	 * [DATA-AREA-N]
	 * [ZERO-AREA  ]
	 * [META-AREA-0]
	 * ...
	 * [META-AREA-N]
	 * [ZERO-AREA  ]
	 *
	 * Start by seeking past the data-areas list...
	 */
	for (area = pvh->area;
	     (u8 *)(area + 1) <= sector + SECTOR_SIZE
		     && get_unaligned_le64(&area->offset);
	     area++)
		;

	/*  ...and then the zero separator, to find the meta-areas. */
	for (area++;
	     (u8 *)(area + 1) <= sector + SECTOR_SIZE
		     && num_mda < (int)ARRAY_SIZE(mda_off);
	     area++) {
		mda_off[num_mda] = get_unaligned_le64(&area->offset);
		mda_size[num_mda] = get_unaligned_le64(&area->size);
		if (!mda_off[num_mda])
			break;

		num_mda++;
	}

	if (!num_mda)
		return -ENOENT;

	for (i = 0; i < num_mda; i++) {
		err = lvm_read_mda(cdev, mda_off[i], mda_size[i], &text, &len);
		if (err)
			continue;

		err = lvm_md_parse_alloc(text, len, mdp);
		if (err) {
			free(text);
			continue;
		}

		*textp = text;
		return 0;
	}

	return -EILSEQ;
}

static int lvm_pv_alloc(struct cdev *cdev, struct lvm_pv **pvptr)
{
	struct lvm_pv_priv *pvp;
	int err;

	pvp = xzalloc(sizeof(*pvp));
	pvp->cdev = cdev;

	err = lvm_pv_probe(cdev, pvp->pv.uuid, &pvp->text, &pvp->md);
	if (err) {
		free(pvp);
		return err;
	}

	*pvptr = &pvp->pv;
	return 0;
}

static void lvm_pv_free(struct lvm_pv *pv)
{
	struct lvm_pv_priv *pvp;

	if (!pv)
		return;

	pvp = to_pv_priv(pv);
	lvm_md_free(pvp->md);
	free(pvp->text);
	free(pvp->pv.name);
	free(pvp);
}

static struct cdev *lvm_cdev_by_uuid(const char *uuid)
{
	char found[LVM_UUID_LEN + 1];
	struct lvm2_pv_header *pvh;
	u8 sector[SECTOR_SIZE];
	struct cdev *cdev;
	u32 pv_offset;

	for_each_cdev(cdev) {
		if (!cdev_is_block_device(cdev))
			continue;

		if (lvm_read_label(cdev, sector, &pv_offset))
			continue;

		pvh = (void *)(sector + pv_offset);
		lvm_uuid_strcpy(found, (char *)pvh->uuid, sizeof(pvh->uuid));

		if (!strcmp(found, uuid))
			return cdev;
	}

	return NULL;
}

static struct lvm_pv *lvm_vg_add_pv(struct lvm_vg *vg, const struct lvm_md *md,
				    const lvm_tok_t *pvkey, struct lvm_pv *origin)
{
	const lvm_tok_t *id, *pvsect = lvm_md_val(md, pvkey);
	struct lvm_pv_priv *pvp;
	struct lvm_pv *pv;
	u64 v;

	if (!pvsect || pvsect->type != LVM_TOK_SECTION)
		return NULL;

	pvp = xzalloc(sizeof(*pvp));
	pv = &pvp->pv;
	pv->vg = vg;
	pv->name = lvm_md_tok_xstrdup(md, pvkey);

	id = lvm_md_find(md, pvsect, "id");
	if (id && id->type == LVM_TOK_STRING)
		lvm_uuid_strcpy(pv->uuid, md->text + id->start, id->end - id->start);

	if (!lvm_md_u64(md, pvsect, "dev_size", &v))
		pv->dev_size = v;
	if (!lvm_md_u64(md, pvsect, "pe_start", &v))
		pv->pe_start = v;
	if (!lvm_md_u64(md, pvsect, "pe_count", &v))
		pv->pe_count = v;

	/* The originating PV's device is known for free. Any other
	 * PVs are resolved later via lvm_pv_cdev().
	 */
	if (origin && !strcmp(pv->uuid, origin->uuid))
		pvp->cdev = to_pv_priv(origin)->cdev;

	vg->pvs = xrealloc(vg->pvs, (vg->num_pvs + 1) * sizeof(*vg->pvs));
	vg->pvs[vg->num_pvs++] = pv;
	return pv;
}

static struct lvm_pv *lvm_vg_pv_by_name(struct lvm_vg *vg, const char *name,
					size_t len)
{
	size_t i;

	for (i = 0; i < vg->num_pvs; i++) {
		if (!strncmp(vg->pvs[i]->name, name, len) &&
		    vg->pvs[i]->name[len] == '\0')
			return vg->pvs[i];
	}

	return NULL;
}

/* Parse one segment of an LV. Returns 0 on a supported (linear)
 * segment, or a negative error for anything we cannot map.
 */
static int lvm_lv_add_seg(struct lvm_lv_priv *lpriv, struct lvm_vg *vg,
			  const struct lvm_md *md, const lvm_tok_t *segsect)
{
	u64 pe_count, pe_start, pe_offset, stripe_count;
	const lvm_tok_t *stripes, *pvtok, *offtok;
	struct lvm_seg *seg;
	struct lvm_pv *pv;
	char *type;
	int err;

	type = lvm_md_strdup(md, segsect, "type");
	if (!type)
		return -EINVAL;

	if (lvm_md_u64(md, segsect, "start_extent", &pe_start) ||
	    lvm_md_u64(md, segsect, "extent_count", &pe_count)) {
		err = -EINVAL;
		goto out;
	}

	err = -ENOTSUPP;

	if (strcmp(type, "striped") ||
	    lvm_md_u64(md, segsect, "stripe_count", &stripe_count) ||
	    stripe_count != 1)
		goto out;

	stripes = lvm_md_find(md, segsect, "stripes");
	if (!stripes || stripes->type != LVM_TOK_ARRAY)
		goto out;

	pvtok = lvm_md_first(md, stripes);
	offtok = pvtok ? lvm_md_next(md, stripes, pvtok) : NULL;
	if (!pvtok || !offtok || lvm_md_tok_u64(md, offtok, &pe_offset))
		goto out;

	pv = lvm_vg_pv_by_name(vg, md->text + pvtok->start,
			       pvtok->end - pvtok->start);
	if (!pv)
		goto out;

	seg = &lpriv->segs[lpriv->num_segs++];
	seg->pv = pv;
	seg->start = pe_start * vg->pe_size;
	seg->len = pe_count * vg->pe_size;
	seg->phys = pv->pe_start + pe_offset * vg->pe_size;
	lpriv->lv.size += seg->len;
	err = 0;
out:
	free(type);
	return err;
}

static int lvm_seg_cmp(const void *_sega, const void *_segb)
{
	const struct lvm_seg *sega = _sega, *segb = _segb;

	if (sega->start < segb->start)
		return -1;
	if (sega->start > segb->start)
		return 1;
	return 0;
}

static void lvm_vg_add_lv(struct lvm_vg *vg, const struct lvm_md *md,
			  const lvm_tok_t *lvkey)
{
	const lvm_tok_t *id, *lvsect = lvm_md_val(md, lvkey);
	struct lvm_lv_priv *lvp;
	const lvm_tok_t *seg;
	struct lvm_lv *lv;
	u64 num_segs = 0;
	int i;

	if (!lvsect || lvsect->type != LVM_TOK_SECTION)
		return;

	lvp = xzalloc(sizeof(*lvp));
	lv = &lvp->lv;
	lv->vg = vg;
	lv->type = LVM_LV_LINEAR;
	lv->name = lvm_md_tok_xstrdup(md, lvkey);

	id = lvm_md_find(md, lvsect, "id");
	if (id && id->type == LVM_TOK_STRING)
		lvm_uuid_strcpy(lv->uuid, md->text + id->start, id->end - id->start);

	lvm_md_u64(md, lvsect, "segment_count", &num_segs);
	if (num_segs)
		lvp->segs = xzalloc(num_segs * sizeof(*lvp->segs));

	for (i = 1; i <= (int)num_segs; i++) {
		seg = lvm_md_findf(md, lvsect, "segment%d", i);
		if (!seg || seg->type != LVM_TOK_SECTION ||
		    lvm_lv_add_seg(lvp, vg, md, seg)) {
			/* Unsupported mapping: keep the LV visible but
			 * mark it so that activation is refused.
			 */
			lv->type = LVM_LV_UNKNOWN;
			break;
		}
	}

	if (lv->type == LVM_LV_LINEAR && lvp->num_segs)
		qsort(lvp->segs, lvp->num_segs, sizeof(*lvp->segs),
		      lvm_seg_cmp);

	vg->lvs = xrealloc(vg->lvs, (vg->num_lvs + 1) * sizeof(*vg->lvs));
	vg->lvs[vg->num_lvs++] = lv;
}

static int lvm_vg_alloc(struct lvm_pv *pv, struct lvm_vg **vgp)
{
	const lvm_tok_t *vgsect, *vgkey, *pvs, *lvs, *key;
	struct lvm_pv_priv *priv = to_pv_priv(pv);
	const struct lvm_md *md = priv->md;
	struct lvm_vg *vg;
	char *id;
	u64 v;

	if (!md)
		return -EINVAL;

	vgsect = lvm_md_vgsect(md, &vgkey);
	if (!vgsect)
		return -EINVAL;

	vg = xzalloc(sizeof(*vg));
	vg->name = lvm_md_tok_xstrdup(md, vgkey);

	if (!lvm_md_u64(md, vgsect, "seqno", &v))
		vg->seqno = v;
	if (!lvm_md_u64(md, vgsect, "extent_size", &v))
		vg->pe_size = v;

	id = lvm_md_strdup(md, vgsect, "id");
	if (id) {
		lvm_uuid_strcpy(vg->uuid, id, strlen(id));
		free(id);
	}

	pvs = lvm_md_find(md, vgsect, "physical_volumes");
	if (pvs && pvs->type == LVM_TOK_SECTION) {
		lvm_md_for_each(md, key, pvs)
			lvm_vg_add_pv(vg, md, key, pv);
	}

	lvs = lvm_md_find(md, vgsect, "logical_volumes");
	if (lvs && lvs->type == LVM_TOK_SECTION) {
		lvm_md_for_each(md, key, lvs)
			lvm_vg_add_lv(vg, md, key);
	}

	*vgp = vg;
	return 0;
}

void lvm_vg_free(struct lvm_vg *vg)
{
	size_t i;

	if (!vg)
		return;

	for (i = 0; i < vg->num_lvs; i++) {
		struct lvm_lv_priv *lvp = to_lv_priv(vg->lvs[i]);

		free((char *)lvp->lv.name);
		free(lvp->segs);
		free(lvp);
	}
	free(vg->lvs);

	for (i = 0; i < vg->num_pvs; i++) {
		struct lvm_pv_priv *ppriv = to_pv_priv(vg->pvs[i]);

		free(ppriv->pv.name);
		free(ppriv);
	}
	free(vg->pvs);

	free((char *)vg->name);
	free(vg);
}

struct cdev *lvm_pv_cdev(struct lvm_pv *pv)
{
	struct lvm_pv_priv *pvp = to_pv_priv(pv);

	/* Resolve and cache the backing device on first use. */
	if (!pvp->cdev)
		pvp->cdev = lvm_cdev_by_uuid(pv->uuid);

	return pvp->cdev;
}

struct lvm_lv *lvm_vg_lv_by_name(struct lvm_vg *vg, const char *name)
{
	size_t i;

	for (i = 0; i < vg->num_lvs; i++) {
		if (!strcmp(vg->lvs[i]->name, name))
			return vg->lvs[i];
	}

	return NULL;
}

char *lvm_lv_dm_ctable(struct lvm_lv *lv)
{
	struct lvm_lv_priv *lvp = to_lv_priv(lv);
	char *table = NULL;
	struct lvm_seg *s;
	struct cdev *cdev;
	size_t i;

	if (lv->type != LVM_LV_LINEAR)
		return ERR_PTR(-ENOTSUPP);

	for (i = 0, s = lvp->segs; i < lvp->num_segs; i++, s++) {
		cdev = s->pv ? lvm_pv_cdev(s->pv) : NULL;
		if (!cdev) {
			free(table);
			return ERR_PTR(-ENODEV);
		}

		table = xrasprintf(table, "%llu %llu linear /dev/%s %llu\n",
				   (u64)s->start, (u64)s->len,
				   cdev_name(cdev), (u64)s->phys);
	}

	if (!table)
		return ERR_PTR(-EINVAL);

	return table;
}

struct lvm_vg_iter {
	struct lvm_vg **vgs;
	int num, cur;
};

void lvm_vg_iter_free(struct lvm_vg_iter *iter)
{
	for (; iter->cur < iter->num; iter->cur++)
		lvm_vg_free(iter->vgs[iter->cur]);

	free(iter->vgs);
	free(iter);
}

struct lvm_vg *lvm_vg_iter_next(struct lvm_vg_iter *iter)
{
	if (iter->cur >= iter->num)
		return NULL;

	return iter->vgs[iter->cur++];
}

struct lvm_vg_iter *lvm_vg_iter_new(void)
{
	struct lvm_vg_iter *iter;
	struct cdev *cdev;
	struct lvm_pv *pv;
	struct lvm_vg *vg;
	int err, i;

	iter = xzalloc(sizeof(*iter));

	for_each_cdev(cdev) {
		if (!cdev_is_block_device(cdev))
			continue;

		if (lvm_pv_alloc(cdev, &pv))
			continue;

		err = lvm_vg_alloc(pv, &vg);
		lvm_pv_free(pv);
		if (err)
			continue;

		for (i = 0; i < iter->num; i++) {
			/* Use the most recently updated VG metadata
			 * when multiple versions are available.
			 */
			if (strcmp(vg->uuid, iter->vgs[i]->uuid))
				continue;

			if (vg->seqno > iter->vgs[i]->seqno) {
				lvm_vg_free(iter->vgs[i]);
				iter->vgs[i] = vg;
			}

			goto next;
		}

		iter->vgs = xrealloc(iter->vgs, (iter->num + 1) * sizeof(*iter->vgs));
		iter->vgs[iter->num++] = vg;
next:
	}

	return iter;
}

int lvm_vg_alloc_by_name(const char *name, struct lvm_vg **vgp)
{
	struct lvm_vg_iter *iter;
	struct lvm_vg *vg;

	iter = lvm_vg_iter_new();
	while ((vg = lvm_vg_iter_next(iter))) {
		if (!strcmp(vg->name, name)) {
			lvm_vg_iter_free(iter);
			*vgp = vg;
			return 0;
		}
	}

	lvm_vg_iter_free(iter);
	return -ENOENT;
}

int lvm_vg_alloc_by_cdev(struct cdev *cdev, struct lvm_vg **vgp)
{
	struct lvm_pv *pv;
	struct lvm_vg *vg;
	int err;

	if (!cdev_is_block_device(cdev))
		return -EINVAL;

	err = lvm_pv_alloc(cdev, &pv);
	if (err)
		return err;

	err = lvm_vg_alloc(pv, &vg);
	lvm_pv_free(pv);
	if (err)
		return err;

	*vgp = vg;
	return 0;
}
