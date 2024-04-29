// SPDX-License-Identifier: GPL-2.0-only
/*
 * JFFS2 -- Journalling Flash File System, Version 2.
 *
 * Copyright © 2001-2007 Red Hat, Inc.
 * Copyright © 2004-2010 David Woodhouse <dwmw2@infradead.org>
 * Copyright © 2004 Ferenc Havasi <havasi@inf.u-szeged.hu>,
 *		    University of Szeged, Hungary
 *
 * Created by Arjan van de Ven <arjan@infradead.org>
 */
#define pr_fmt(fmt) "jffs2: " fmt
#include <common.h>
#include "compr.h"

/* Available compressors are on this list */
static LIST_HEAD(jffs2_compressor_list);

/* Statistics for blocks stored without compression */
static uint32_t none_stat_decompr_blocks=0;

/*
 * Return 1 to use this compression
 */
int jffs2_decompress(struct jffs2_sb_info *c, struct jffs2_inode_info *f,
		     uint16_t comprtype, unsigned char *cdata_in,
		     unsigned char *data_out, uint32_t cdatalen, uint32_t datalen)
{
	struct jffs2_compressor *this;
	int ret;

	/* Older code had a bug where it would write non-zero 'usercompr'
	   fields. Deal with it. */
	if ((comprtype & 0xff) <= JFFS2_COMPR_ZLIB)
		comprtype &= 0xff;

	switch (comprtype & 0xff) {
	case JFFS2_COMPR_NONE:
		/* This should be special-cased elsewhere, but we might as well deal with it */
		memcpy(data_out, cdata_in, datalen);
		none_stat_decompr_blocks++;
		break;
	case JFFS2_COMPR_ZERO:
		memset(data_out, 0, datalen);
		break;
	default:
		list_for_each_entry(this, &jffs2_compressor_list, list) {
			if (comprtype == this->compr) {
				this->usecount++;
				ret = this->decompress(cdata_in, data_out, cdatalen, datalen);
				if (ret) {
					pr_warn("Decompressor \"%s\" returned %d\n",
						this->name, ret);
				}
				else {
					this->stat_decompr_blocks++;
				}
				this->usecount--;
				return ret;
			}
		}
		pr_warn("compression type 0x%02x not available\n", comprtype);
		return -EIO;
	}
	return 0;
}

int jffs2_register_compressor(struct jffs2_compressor *comp)
{
	struct jffs2_compressor *this;

	if (!comp->name) {
		pr_warn("NULL compressor name at registering JFFS2 compressor. Failed.\n");
		return -1;
	}
	comp->compr_buf_size=0;
	comp->compr_buf=NULL;
	comp->usecount=0;
	comp->stat_compr_orig_size=0;
	comp->stat_compr_new_size=0;
	comp->stat_compr_blocks=0;
	comp->stat_decompr_blocks=0;
	jffs2_dbg(1, "Registering JFFS2 compressor \"%s\"\n", comp->name);

	list_for_each_entry(this, &jffs2_compressor_list, list) {
		if (this->priority < comp->priority) {
			list_add(&comp->list, this->list.prev);
			goto out;
		}
	}
	list_add_tail(&comp->list, &jffs2_compressor_list);
out:
	D2(list_for_each_entry(this, &jffs2_compressor_list, list) {
		printk(KERN_DEBUG "Compressor \"%s\", prio %d\n", this->name, this->priority);
	})


	return 0;
}

int jffs2_unregister_compressor(struct jffs2_compressor *comp)
{
	D2(struct jffs2_compressor *this);

	jffs2_dbg(1, "Unregistering JFFS2 compressor \"%s\"\n", comp->name);

	if (comp->usecount) {
		pr_warn("Compressor module is in use. Unregister failed.\n");
		return -1;
	}
	list_del(&comp->list);

	D2(list_for_each_entry(this, &jffs2_compressor_list, list) {
		printk(KERN_DEBUG "Compressor \"%s\", prio %d\n", this->name, this->priority);
	})
	return 0;
}

int __init jffs2_compressors_init(void)
{
/* Registering compressors */
#ifdef CONFIG_FS_JFFS2_COMPRESSION_ZLIB
	jffs2_zlib_init();
#endif
#ifdef CONFIG_FS_JFFS2_COMPRESSION_LZO
	jffs2_lzo_init();
#endif
#ifdef CONFIG_FS_JFFS2_COMPRESSION_RTIME
	jffs2_rtime_init();
#endif
#ifdef CONFIG_FS_JFFS2_COMPRESSION_RUBIN
	jffs2_dynrubin_init();
#endif
/* Setting default compression mode */
	return 0;
}

int jffs2_compressors_exit(void)
{
#ifdef CONFIG_FS_JFFS2_COMPRESSION_ZLIB
	jffs2_zlib_exit();
#endif
#ifdef CONFIG_FS_JFFS2_COMPRESSION_LZO
	jffs2_lzo_exit();
#endif
#ifdef CONFIG_FS_JFFS2_COMPRESSION_RTIME
	jffs2_rtime_exit();
#endif
#ifdef CONFIG_FS_JFFS2_COMPRESSION_RUBIN
	jffs2_dynrubin_exit();
#endif
	return 0;
}
