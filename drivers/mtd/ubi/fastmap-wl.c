/*
 * Copyright (c) 2012 Linutronix GmbH
 * Copyright (c) 2014 sigma star gmbh
 * Author: Richard Weinberger <richard@nod.at>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
 * the GNU General Public License for more details.
 *
 */

/**
 * find_anchor_wl_entry - find wear-leveling entry to used as anchor PEB.
 * @root: the RB-tree where to look for
 */
static struct ubi_wl_entry *find_anchor_wl_entry(struct rb_root *root)
{
	struct rb_node *p;
	struct ubi_wl_entry *e, *victim = NULL;
	int max_ec = UBI_MAX_ERASECOUNTER;

	ubi_rb_for_each_entry(p, e, root, u.rb) {
		if (e->pnum < UBI_FM_MAX_START && e->ec < max_ec) {
			victim = e;
			max_ec = e->ec;
		}
	}

	return victim;
}

/**
 * return_unused_pool_pebs - returns unused PEB to the free tree.
 * @ubi: UBI device description object
 * @pool: fastmap pool description object
 */
static void return_unused_pool_pebs(struct ubi_device *ubi,
				    struct ubi_fm_pool *pool)
{
	int i;
	struct ubi_wl_entry *e;

	for (i = pool->used; i < pool->size; i++) {
		e = ubi->lookuptbl[pool->pebs[i]];
		wl_tree_add(e, &ubi->free);
		ubi->free_count++;
	}
}

static int anchor_pebs_available(struct rb_root *root)
{
	struct rb_node *p;
	struct ubi_wl_entry *e;

	ubi_rb_for_each_entry(p, e, root, u.rb)
		if (e->pnum < UBI_FM_MAX_START)
			return 1;

	return 0;
}

/**
 * ubi_wl_get_fm_peb - find a physical erase block with a given maximal number.
 * @ubi: UBI device description object
 * @anchor: This PEB will be used as anchor PEB by fastmap
 *
 * The function returns a physical erase block with a given maximal number
 * and removes it from the wl subsystem.
 * Must be called with wl_lock held!
 */
struct ubi_wl_entry *ubi_wl_get_fm_peb(struct ubi_device *ubi, int anchor)
{
	struct ubi_wl_entry *e = NULL;

	if (!ubi->free.rb_node || (ubi->free_count - ubi->beb_rsvd_pebs < 1))
		goto out;

	if (anchor)
		e = find_anchor_wl_entry(&ubi->free);
	else
		e = find_mean_wl_entry(ubi, &ubi->free);

	if (!e)
		goto out;

	self_check_in_wl_tree(ubi, e, &ubi->free);

	/* remove it from the free list,
	 * the wl subsystem does no longer know this erase block */
	rb_erase(&e->u.rb, &ubi->free);
	ubi->free_count--;
out:
	return e;
}

/**
 * ubi_refill_pools - refills all fastmap PEB pools.
 * @ubi: UBI device description object
 */
void ubi_refill_pools(struct ubi_device *ubi)
{
	struct ubi_fm_pool *wl_pool = &ubi->fm_wl_pool;
	struct ubi_fm_pool *pool = &ubi->fm_pool;
	struct ubi_wl_entry *e;
	int enough;

	return_unused_pool_pebs(ubi, wl_pool);
	return_unused_pool_pebs(ubi, pool);

	wl_pool->size = 0;
	pool->size = 0;

	for (;;) {
		enough = 0;
		if (pool->size < pool->max_size) {
			if (!ubi->free.rb_node)
				break;

			e = wl_get_wle(ubi);
			if (!e)
				break;

			pool->pebs[pool->size] = e->pnum;
			pool->size++;
		} else
			enough++;

		if (wl_pool->size < wl_pool->max_size) {
			if (!ubi->free.rb_node ||
			   (ubi->free_count - ubi->beb_rsvd_pebs < 5))
				break;

			e = find_wl_entry(ubi, &ubi->free, WL_FREE_MAX_DIFF);
			self_check_in_wl_tree(ubi, e, &ubi->free);
			rb_erase(&e->u.rb, &ubi->free);
			ubi->free_count--;

			wl_pool->pebs[wl_pool->size] = e->pnum;
			wl_pool->size++;
		} else
			enough++;

		if (enough == 2)
			break;
	}

	wl_pool->used = 0;
	pool->used = 0;
}

/**
 * ubi_wl_get_peb - get a physical eraseblock.
 * @ubi: UBI device description object
 *
 * This function returns a physical eraseblock in case of success and a
 * negative error code in case of failure.
 * Returns with ubi->fm_eba_sem held in read mode!
 */
int ubi_wl_get_peb(struct ubi_device *ubi)
{
	int ret;
	struct ubi_fm_pool *pool = &ubi->fm_pool;
	struct ubi_fm_pool *wl_pool = &ubi->fm_wl_pool;

	/* We check here also for the WL pool because at this point we can
	 * refill the WL pool synchronous. */
	if (!pool->size || !wl_pool->size || pool->used == pool->size ||
		wl_pool->used == wl_pool->size) {
		ret = ubi_update_fastmap(ubi);
		if (ret) {
			ubi_msg(ubi, "Unable to write a new fastmap: %i", ret);
			return -ENOSPC;
		}
	}

	if (!pool->size) {
		ubi_err(ubi, "Unable to get a free PEB from user WL pool");
		ret = -ENOSPC;
	} else {
		ret = pool->pebs[pool->used++];
		prot_queue_add(ubi, ubi->lookuptbl[ret]);
	}

	return ret;
}

/* get_peb_for_wl - returns a PEB to be used internally by the WL sub-system.
 *
 * @ubi: UBI device description object
 */
static struct ubi_wl_entry *get_peb_for_wl(struct ubi_device *ubi)
{
	struct ubi_fm_pool *pool = &ubi->fm_wl_pool;
	int pnum;

	if (pool->used == pool->size || !pool->size) {
		ubi_update_fastmap(ubi);
		if (pool->used == pool->size || !pool->size)
			BUG();
	}

	pnum = pool->pebs[pool->used++];

	return ubi->lookuptbl[pnum];
}

/**
 * ubi_ensure_anchor_pebs - schedule wear-leveling to produce an anchor PEB.
 * @ubi: UBI device description object
 */
int ubi_ensure_anchor_pebs(struct ubi_device *ubi)
{
	struct ubi_work *wrk;

	if (ubi->wl_scheduled)
		return 0;

	ubi->wl_scheduled = 1;

	wrk = kmalloc(sizeof(struct ubi_work), GFP_NOFS);
	if (!wrk) {
		ubi->wl_scheduled = 0;
		return -ENOMEM;
	}

	wrk->anchor = 1;
	wrk->func = &wear_leveling_worker;
	__schedule_ubi_work(ubi, wrk);
	return 0;
}

/**
 * ubi_wl_put_fm_peb - returns a PEB used in a fastmap to the wear-leveling
 * sub-system.
 * see: ubi_wl_put_peb()
 *
 * @ubi: UBI device description object
 * @fm_e: physical eraseblock to return
 * @lnum: the last used logical eraseblock number for the PEB
 * @torture: if this physical eraseblock has to be tortured
 */
int ubi_wl_put_fm_peb(struct ubi_device *ubi, struct ubi_wl_entry *fm_e,
		      int lnum, int torture)
{
	struct ubi_wl_entry *e;
	int vol_id, pnum = fm_e->pnum;

	dbg_wl("PEB %d", pnum);

	ubi_assert(pnum >= 0);
	ubi_assert(pnum < ubi->peb_count);

	e = ubi->lookuptbl[pnum];

	/* This can happen if we recovered from a fastmap the very
	 * first time and writing now a new one. In this case the wl system
	 * has never seen any PEB used by the original fastmap.
	 */
	if (!e) {
		e = fm_e;
		ubi_assert(e->ec >= 0);
		ubi->lookuptbl[pnum] = e;
	}

	vol_id = lnum ? UBI_FM_DATA_VOLUME_ID : UBI_FM_SB_VOLUME_ID;
	return schedule_erase(ubi, e, vol_id, lnum, torture, true);
}

/**
 * ubi_is_erase_work - checks whether a work is erase work.
 * @wrk: The work object to be checked
 */
int ubi_is_erase_work(struct ubi_work *wrk)
{
	return wrk->func == erase_worker;
}

static void ubi_fastmap_close(struct ubi_device *ubi)
{
	int i;

	return_unused_pool_pebs(ubi, &ubi->fm_pool);
	return_unused_pool_pebs(ubi, &ubi->fm_wl_pool);

	if (ubi->fm) {
		for (i = 0; i < ubi->fm->used_blocks; i++)
			kfree(ubi->fm->e[i]);
	}
	kfree(ubi->fm);
}

/**
 * may_reserve_for_fm - tests whether a PEB shall be reserved for fastmap.
 * See find_mean_wl_entry()
 *
 * @ubi: UBI device description object
 * @e: physical eraseblock to return
 * @root: RB tree to test against.
 */
static struct ubi_wl_entry *may_reserve_for_fm(struct ubi_device *ubi,
					   struct ubi_wl_entry *e,
					   struct rb_root *root) {
	if (e && !ubi->fm_disabled && !ubi->fm &&
	    e->pnum < UBI_FM_MAX_START)
		e = rb_entry(rb_next(root->rb_node),
			     struct ubi_wl_entry, u.rb);

	return e;
}
