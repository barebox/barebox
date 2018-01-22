/*
 * Copyright (C) 2008 Nokia Corporation
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
 * Author: Artem Bityutskiy
 *
 * UBI scanning library.
 */

#define PROGRAM_NAME "libscan"

#include <common.h>
#include <fcntl.h>
#include <crc.h>
#include <stdlib.h>
#include <malloc.h>
#include <linux/mtd/mtd.h>
#include <linux/stat.h>
#include <linux/mtd/mtd-abi.h>
#include <mtd/mtd-peb.h>
#include <mtd/libscan.h>
#include <mtd/ubi-user.h>
#include <mtd/utils.h>
#include <mtd/ubi-media.h>
#include <asm-generic/div64.h>

int libscan_ubi_scan(struct mtd_info *mtd, struct ubi_scan_info **info,
	     int verbose)
{
	int eb, v = (verbose == 2), pr = (verbose == 1), eb_cnt;
	struct ubi_scan_info *si;
	unsigned long long sum = 0;
	uint64_t lastprint = 0;

	eb_cnt = mtd_div_by_eb(mtd->size, mtd);

	si = calloc(1, sizeof(struct ubi_scan_info));
	if (!si)
		return sys_errmsg("cannot allocate %zd bytes of memory",
				  sizeof(struct ubi_scan_info));

	si->ec = calloc(eb_cnt, sizeof(uint32_t));
	if (!si->ec) {
		sys_errmsg("cannot allocate %zd bytes of memory",
			   sizeof(struct ubi_scan_info));
		goto out_si;
	}

	si->vid_hdr_offs = si->data_offs = -1;

	verbose(v, "start scanning eraseblocks 0-%d", eb_cnt);
	for (eb = 0; eb < eb_cnt; eb++) {
		int ret;
		uint32_t crc;
		struct ubi_ec_hdr ech;
		unsigned long long ec;

		if (v)
			normsg_cont("scanning eraseblock %d", eb);
		if (pr) {
			if (is_timeout(lastprint, 300 * MSECOND) ||
			    eb == eb_cnt - 1) {
				printf("\r" PROGRAM_NAME ": scanning eraseblock %d -- %2u %% complete  ",
					eb, (eb + 1) * 100 / eb_cnt);
				lastprint = get_time_ns();
			}
		}

		ret = mtd_peb_is_bad(mtd, eb);
		if (ret == -1)
			goto out_ec;
		if (ret) {
			si->bad_cnt += 1;
			si->ec[eb] = EB_BAD;
			if (v)
				printf(": bad\n");
			continue;
		}

		ret = mtd_peb_read(mtd, &ech, eb, 0, sizeof(struct ubi_ec_hdr));
		if (ret < 0 && !mtd_is_bitflip(ret)) {
			si->corrupted_cnt += 1;
			si->ec[eb] = EB_CORRUPTED;
			if (v)
				printf(": not readable\n");
			continue;
		}

		if (be32_to_cpu(ech.magic) != UBI_EC_HDR_MAGIC) {
			if (mtd_buf_all_ff(&ech, sizeof(struct ubi_ec_hdr))) {
				si->empty_cnt += 1;
				si->ec[eb] = EB_EMPTY;
				if (v)
					printf(": empty\n");
			} else {
				si->alien_cnt += 1;
				si->ec[eb] = EB_ALIEN;
				if (v)
					printf(": alien\n");
			}
			continue;
		}

		crc = crc32_no_comp(UBI_CRC32_INIT, &ech, UBI_EC_HDR_SIZE_CRC);
		if (be32_to_cpu(ech.hdr_crc) != crc) {
			si->corrupted_cnt += 1;
			si->ec[eb] = EB_CORRUPTED;
			if (v)
				printf(": bad CRC %#08x, should be %#08x\n",
				       crc, be32_to_cpu(ech.hdr_crc));
			continue;
		}

		ec = be64_to_cpu(ech.ec);
		if (ec > EC_MAX) {
			if (pr)
				printf("\n");
			errmsg("erase counter in EB %d is %llu, while this "
			       "program expects them to be less than %u",
			       eb, ec, EC_MAX);
			goto out_ec;
		}

		if (si->vid_hdr_offs == -1) {
			si->vid_hdr_offs = be32_to_cpu(ech.vid_hdr_offset);
			si->data_offs = be32_to_cpu(ech.data_offset);
			if (si->data_offs % mtd->writesize) {
				if (pr)
					printf("\n");
				if (v)
					printf(": corrupted because of the below\n");
				warnmsg("bad data offset %d at eraseblock %d (n"
					"of multiple of min. I/O unit size %d)",
					si->data_offs, eb, mtd->writesize);
				warnmsg("treat eraseblock %d as corrupted", eb);
				si->corrupted_cnt += 1;
				si->ec[eb] = EB_CORRUPTED;
				continue;

			}
		} else {
			if ((int)be32_to_cpu(ech.vid_hdr_offset) != si->vid_hdr_offs) {
				if (pr)
					printf("\n");
				if (v)
					printf(": corrupted because of the below\n");
				warnmsg("inconsistent VID header offset: was "
					"%d, but is %d in eraseblock %d",
					si->vid_hdr_offs,
					be32_to_cpu(ech.vid_hdr_offset), eb);
				warnmsg("treat eraseblock %d as corrupted", eb);
				si->corrupted_cnt += 1;
				si->ec[eb] = EB_CORRUPTED;
				continue;
			}
			if ((int)be32_to_cpu(ech.data_offset) != si->data_offs) {
				if (pr)
					printf("\n");
				if (v)
					printf(": corrupted because of the below\n");
				warnmsg("inconsistent data offset: was %d, but"
					" is %d in eraseblock %d",
					si->data_offs,
					be32_to_cpu(ech.data_offset), eb);
				warnmsg("treat eraseblock %d as corrupted", eb);
				si->corrupted_cnt += 1;
				si->ec[eb] = EB_CORRUPTED;
				continue;
			}
		}

		si->ok_cnt += 1;
		si->ec[eb] = ec;
		if (v)
			printf(": OK, erase counter %u\n", si->ec[eb]);
	}

	if (si->ok_cnt != 0) {
		/* Calculate mean erase counter */
		for (eb = 0; eb < eb_cnt; eb++) {
			if (si->ec[eb] > EC_MAX)
				continue;
			sum += si->ec[eb];
		}
		do_div(sum, si->ok_cnt);
		si->mean_ec = sum;
	}

	si->good_cnt = eb_cnt - si->bad_cnt;
	verbose(v, "finished, mean EC %lld, %d OK, %d corrupted, %d empty, %d "
		"alien, bad %d", si->mean_ec, si->ok_cnt, si->corrupted_cnt,
		si->empty_cnt, si->alien_cnt, si->bad_cnt);

	*info = si;
	if (pr)
		printf("\n");
	return 0;

out_ec:
	free(si->ec);
out_si:
	free(si);
	*info = NULL;
	return -1;
}

void libscan_ubi_scan_free(struct ubi_scan_info *si)
{
	free(si->ec);
	free(si);
}
