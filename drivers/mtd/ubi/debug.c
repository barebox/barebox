/*
 * Copyright (c) International Business Machines Corp., 2006
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Author: Artem Bityutskiy (Битюцкий Артём)
 */

#include "ubi.h"

/**
 * ubi_dump_flash - dump a region of flash.
 * @ubi: UBI device description object
 * @pnum: the physical eraseblock number to dump
 * @offset: the starting offset within the physical eraseblock to dump
 * @len: the length of the region to dump
 */
void ubi_dump_flash(struct ubi_device *ubi, int pnum, int offset, int len)
{
	int err;
	size_t read;
	void *buf;
	loff_t addr = (loff_t)pnum * ubi->peb_size + offset;

	buf = vmalloc(len);
	if (!buf)
		return;
	err = mtd_read(ubi->mtd, addr, len, &read, buf);
	if (err && err != -EUCLEAN) {
		ubi_err("error %d while reading %d bytes from PEB %d:%d, read %zd bytes",
			err, len, pnum, offset, read);
		goto out;
	}

	ubi_msg("dumping %d bytes of data from PEB %d, offset %d",
		len, pnum, offset);
	print_hex_dump(KERN_DEBUG, "", DUMP_PREFIX_OFFSET, 32, 1, buf, len, 1);
out:
	vfree(buf);
	return;
}

/**
 * ubi_dump_ec_hdr - dump an erase counter header.
 * @ec_hdr: the erase counter header to dump
 */
void ubi_dump_ec_hdr(const struct ubi_ec_hdr *ec_hdr)
{
	pr_err("Erase counter header dump:\n");
	pr_err("\tmagic          %#08x\n", be32_to_cpu(ec_hdr->magic));
	pr_err("\tversion        %d\n", (int)ec_hdr->version);
	pr_err("\tec             %llu\n", (long long)be64_to_cpu(ec_hdr->ec));
	pr_err("\tvid_hdr_offset %d\n", be32_to_cpu(ec_hdr->vid_hdr_offset));
	pr_err("\tdata_offset    %d\n", be32_to_cpu(ec_hdr->data_offset));
	pr_err("\timage_seq      %d\n", be32_to_cpu(ec_hdr->image_seq));
	pr_err("\thdr_crc        %#08x\n", be32_to_cpu(ec_hdr->hdr_crc));
	pr_err("erase counter header hexdump:\n");
	print_hex_dump(KERN_DEBUG, "", DUMP_PREFIX_OFFSET, 32, 1,
		       ec_hdr, UBI_EC_HDR_SIZE, 1);
}

/**
 * ubi_dump_vid_hdr - dump a volume identifier header.
 * @vid_hdr: the volume identifier header to dump
 */
void ubi_dump_vid_hdr(const struct ubi_vid_hdr *vid_hdr)
{
	pr_err("Volume identifier header dump:\n");
	pr_err("\tmagic     %08x\n", be32_to_cpu(vid_hdr->magic));
	pr_err("\tversion   %d\n",  (int)vid_hdr->version);
	pr_err("\tvol_type  %d\n",  (int)vid_hdr->vol_type);
	pr_err("\tcopy_flag %d\n",  (int)vid_hdr->copy_flag);
	pr_err("\tcompat    %d\n",  (int)vid_hdr->compat);
	pr_err("\tvol_id    %d\n",  be32_to_cpu(vid_hdr->vol_id));
	pr_err("\tlnum      %d\n",  be32_to_cpu(vid_hdr->lnum));
	pr_err("\tdata_size %d\n",  be32_to_cpu(vid_hdr->data_size));
	pr_err("\tused_ebs  %d\n",  be32_to_cpu(vid_hdr->used_ebs));
	pr_err("\tdata_pad  %d\n",  be32_to_cpu(vid_hdr->data_pad));
	pr_err("\tsqnum     %llu\n",
		(unsigned long long)be64_to_cpu(vid_hdr->sqnum));
	pr_err("\thdr_crc   %08x\n", be32_to_cpu(vid_hdr->hdr_crc));
	pr_err("Volume identifier header hexdump:\n");
	print_hex_dump(KERN_DEBUG, "", DUMP_PREFIX_OFFSET, 32, 1,
		       vid_hdr, UBI_VID_HDR_SIZE, 1);
}

/**
 * ubi_dump_vol_info - dump volume information.
 * @vol: UBI volume description object
 */
void ubi_dump_vol_info(const struct ubi_volume *vol)
{
	pr_err("Volume information dump:\n");
	pr_err("\tvol_id          %d\n", vol->vol_id);
	pr_err("\treserved_pebs   %d\n", vol->reserved_pebs);
	pr_err("\talignment       %d\n", vol->alignment);
	pr_err("\tdata_pad        %d\n", vol->data_pad);
	pr_err("\tvol_type        %d\n", vol->vol_type);
	pr_err("\tname_len        %d\n", vol->name_len);
	pr_err("\tusable_leb_size %d\n", vol->usable_leb_size);
	pr_err("\tused_ebs        %d\n", vol->used_ebs);
	pr_err("\tused_bytes      %lld\n", vol->used_bytes);
	pr_err("\tlast_eb_bytes   %d\n", vol->last_eb_bytes);
	pr_err("\tcorrupted       %d\n", vol->corrupted);
	pr_err("\tupd_marker      %d\n", vol->upd_marker);

	if (vol->name_len <= UBI_VOL_NAME_MAX &&
	    strnlen(vol->name, vol->name_len + 1) == vol->name_len) {
		pr_err("\tname            %s\n", vol->name);
	} else {
		pr_err("\t1st 5 characters of name: %c%c%c%c%c\n",
		       vol->name[0], vol->name[1], vol->name[2],
		       vol->name[3], vol->name[4]);
	}
}

/**
 * ubi_dump_vtbl_record - dump a &struct ubi_vtbl_record object.
 * @r: the object to dump
 * @idx: volume table index
 */
void ubi_dump_vtbl_record(const struct ubi_vtbl_record *r, int idx)
{
	int name_len = be16_to_cpu(r->name_len);

	pr_err("Volume table record %d dump:\n", idx);
	pr_err("\treserved_pebs   %d\n", be32_to_cpu(r->reserved_pebs));
	pr_err("\talignment       %d\n", be32_to_cpu(r->alignment));
	pr_err("\tdata_pad        %d\n", be32_to_cpu(r->data_pad));
	pr_err("\tvol_type        %d\n", (int)r->vol_type);
	pr_err("\tupd_marker      %d\n", (int)r->upd_marker);
	pr_err("\tname_len        %d\n", name_len);

	if (r->name[0] == '\0') {
		pr_err("\tname            NULL\n");
		return;
	}

	if (name_len <= UBI_VOL_NAME_MAX &&
	    strnlen(&r->name[0], name_len + 1) == name_len) {
		pr_err("\tname            %s\n", &r->name[0]);
	} else {
		pr_err("\t1st 5 characters of name: %c%c%c%c%c\n",
			r->name[0], r->name[1], r->name[2], r->name[3],
			r->name[4]);
	}
	pr_err("\tcrc             %#08x\n", be32_to_cpu(r->crc));
}

/**
 * ubi_dump_av - dump a &struct ubi_ainf_volume object.
 * @av: the object to dump
 */
void ubi_dump_av(const struct ubi_ainf_volume *av)
{
	pr_err("Volume attaching information dump:\n");
	pr_err("\tvol_id         %d\n", av->vol_id);
	pr_err("\thighest_lnum   %d\n", av->highest_lnum);
	pr_err("\tleb_count      %d\n", av->leb_count);
	pr_err("\tcompat         %d\n", av->compat);
	pr_err("\tvol_type       %d\n", av->vol_type);
	pr_err("\tused_ebs       %d\n", av->used_ebs);
	pr_err("\tlast_data_size %d\n", av->last_data_size);
	pr_err("\tdata_pad       %d\n", av->data_pad);
}

/**
 * ubi_dump_aeb - dump a &struct ubi_ainf_peb object.
 * @aeb: the object to dump
 * @type: object type: 0 - not corrupted, 1 - corrupted
 */
void ubi_dump_aeb(const struct ubi_ainf_peb *aeb, int type)
{
	pr_err("eraseblock attaching information dump:\n");
	pr_err("\tec       %d\n", aeb->ec);
	pr_err("\tpnum     %d\n", aeb->pnum);
	if (type == 0) {
		pr_err("\tlnum     %d\n", aeb->lnum);
		pr_err("\tscrub    %d\n", aeb->scrub);
		pr_err("\tsqnum    %llu\n", aeb->sqnum);
	}
}

/**
 * ubi_dump_mkvol_req - dump a &struct ubi_mkvol_req object.
 * @req: the object to dump
 */
void ubi_dump_mkvol_req(const struct ubi_mkvol_req *req)
{
	char nm[17];

	pr_err("Volume creation request dump:\n");
	pr_err("\tvol_id    %d\n",   req->vol_id);
	pr_err("\talignment %d\n",   req->alignment);
	pr_err("\tbytes     %lld\n", (long long)req->bytes);
	pr_err("\tvol_type  %d\n",   req->vol_type);
	pr_err("\tname_len  %d\n",   req->name_len);

	memcpy(nm, req->name, 16);
	nm[16] = 0;
	pr_err("\t1st 16 characters of name: %s\n", nm);
}
