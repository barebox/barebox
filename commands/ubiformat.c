/*
 * Copyright (C) 2008 Nokia Corporation
 * Copyright (C) 2012 Wolfram Sang, Pengutronix
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
 */

/*
 * An utility to format MTD devices into UBI and flash UBI images.
 *
 * Author: Artem Bityutskiy
 */

/*
 * Maximum amount of consequtive eraseblocks which are considered as normal by
 * this utility. Otherwise it is assume that something is wrong with the flash
 * or the driver, and eraseblocks are stopped being marked as bad.
 */
#define MAX_CONSECUTIVE_BAD_BLOCKS 4

#define PROGRAM_NAME    "ubiformat"

#include <common.h>
#include <command.h>
#include <fs.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <crc.h>
#include <stdlib.h>
#include <clock.h>
#include <malloc.h>
#include <ioctl.h>
#include <libbb.h>
#include <linux/mtd/mtd.h>
#include <linux/kernel.h>
#include <linux/stat.h>
#include <linux/log2.h>
#include <linux/mtd/mtd-abi.h>
#include <mtd/libmtd.h>
#include <mtd/libscan.h>
#include <mtd/libubigen.h>
#include <mtd/ubi-user.h>
#include <mtd/utils.h>
#include <mtd/ubi-media.h>

/* The variables below are set by command line arguments */
struct args {
	unsigned int yes:1;
	unsigned int quiet:1;
	unsigned int verbose:1;
	unsigned int override_ec:1;
	unsigned int novtbl:1;
	unsigned int manual_subpage;
	int subpage_size;
	int vid_hdr_offs;
	int ubi_ver;
	uint32_t image_seq;
	long long ec;
	const char *image;
	const char *node;
	int node_fd;
};

static struct args args;

static int parse_opt(int argc, char *argv[])
{
	srand(get_time_ns());
	memset(&args, 0, sizeof(args));
	args.ubi_ver = 1;
	args.image_seq = rand();

	while (1) {
		int key;
		unsigned long int image_seq;

		key = getopt(argc, argv, "nyyqve:x:s:O:f:S:");
		if (key == -1)
			break;

		switch (key) {
		case 's':
			args.subpage_size = strtoull_suffix(optarg, NULL, 0);
			if (args.subpage_size <= 0)
				return errmsg("bad sub-page size: \"%s\"", optarg);
			if (!is_power_of_2(args.subpage_size))
				return errmsg("sub-page size should be power of 2");
			break;

		case 'O':
			args.vid_hdr_offs = simple_strtoul(optarg, NULL, 0);
			if (args.vid_hdr_offs <= 0)
				return errmsg("bad VID header offset: \"%s\"", optarg);
			break;

		case 'e':
			args.ec = simple_strtoull(optarg, NULL, 0);
			if (args.ec < 0)
				return errmsg("bad erase counter value: \"%s\"", optarg);
			if (args.ec >= EC_MAX)
				return errmsg("too high erase %llu, counter, max is %u", args.ec, EC_MAX);
			args.override_ec = 1;
			break;

		case 'f':
			args.image = optarg;
			break;

		case 'n':
			args.novtbl = 1;
			break;

		case 'y':
			args.yes = 1;
			break;

		case 'q':
			args.quiet = 1;
			break;

		case 'x':
			args.ubi_ver = simple_strtoul(optarg, NULL, 0);
			if (args.ubi_ver < 0)
				return errmsg("bad UBI version: \"%s\"", optarg);
			break;

		case 'Q':
			image_seq = simple_strtoul(optarg, NULL, 0);
			if (image_seq > 0xFFFFFFFF)
				return errmsg("bad UBI image sequence number: \"%s\"", optarg);
			args.image_seq = image_seq;
			break;

		case 'v':
			args.verbose = 1;
			break;

		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (args.quiet && args.verbose)
		return errmsg("using \"-q\" and \"-v\" at the same time does not make sense");

	if (optind == argc)
		return errmsg("MTD device name was not specified");
	else if (optind != argc - 1)
		return errmsg("more then one MTD device specified");

	if (args.image && args.novtbl)
		return errmsg("-n cannot be used together with -f");


	args.node = argv[optind];
	return 0;
}

static void print_bad_eraseblocks(const struct mtd_dev_info *mtd,
				  const struct ubi_scan_info *si)
{
	int first = 1, eb;

	if (si->bad_cnt == 0)
		return;

	normsg_cont("%d bad eraseblocks found, numbers: ", si->bad_cnt);
	for (eb = 0; eb < mtd->eb_cnt; eb++) {
		if (si->ec[eb] != EB_BAD)
			continue;
		if (first) {
			printf("%d", eb);
			first = 0;
		} else
			printf(", %d", eb);
	}
	printf("\n");
}

static int change_ech(struct ubi_ec_hdr *hdr, uint32_t image_seq,
		      long long ec)
{
	uint32_t crc;

	/* Check the EC header */
	if (be32_to_cpu(hdr->magic) != UBI_EC_HDR_MAGIC)
		return errmsg("bad UBI magic %#08x, should be %#08x",
			      be32_to_cpu(hdr->magic), UBI_EC_HDR_MAGIC);

	crc = crc32_no_comp(UBI_CRC32_INIT, hdr, UBI_EC_HDR_SIZE_CRC);
	if (be32_to_cpu(hdr->hdr_crc) != crc)
		return errmsg("bad CRC %#08x, should be %#08x\n",
			      crc, be32_to_cpu(hdr->hdr_crc));

	hdr->image_seq = cpu_to_be32(image_seq);
	hdr->ec = cpu_to_be64(ec);
	crc = crc32_no_comp(UBI_CRC32_INIT, hdr, UBI_EC_HDR_SIZE_CRC);
	hdr->hdr_crc = cpu_to_be32(crc);

	return 0;
}

static int drop_ffs(const struct mtd_dev_info *mtd, const void *buf, int len)
{
	int i;

        for (i = len - 1; i >= 0; i--)
		if (((const uint8_t *)buf)[i] != 0xFF)
		      break;

        /* The resulting length must be aligned to the minimum flash I/O size */
	len = i + 1;
	len = (len + mtd->min_io_size - 1) / mtd->min_io_size;
	len *=  mtd->min_io_size;
	return len;
}

static int open_file(off_t *sz)
{
	int fd;
	struct stat st;

	if (stat(args.image, &st))
		return sys_errmsg("cannot open \"%s\"", args.image);

	*sz = st.st_size;
	fd  = open(args.image, O_RDONLY);
	if (fd < 0)
		return sys_errmsg("cannot open \"%s\"", args.image);

	return fd;
}

/*
 * Returns %-1 if consecutive bad blocks exceeds the
 * MAX_CONSECUTIVE_BAD_BLOCKS and returns %0 otherwise.
 */
static int consecutive_bad_check(int eb)
{
	static int consecutive_bad_blocks = 1;
	static int prev_bb = -1;

	if (prev_bb == -1)
		prev_bb = eb;

	if (eb == prev_bb + 1)
		consecutive_bad_blocks += 1;
	else
		consecutive_bad_blocks = 1;

	prev_bb = eb;

	if (consecutive_bad_blocks >= MAX_CONSECUTIVE_BAD_BLOCKS) {
		if (!args.quiet)
			printf("\n");
		return errmsg("consecutive bad blocks exceed limit: %d, bad flash?",
		              MAX_CONSECUTIVE_BAD_BLOCKS);
	}

	return 0;
}

/* TODO: we should actually torture the PEB before marking it as bad */
static int mark_bad(const struct mtd_dev_info *mtd, struct ubi_scan_info *si, int eb)
{
	int err;

	if (!args.quiet)
		normsg_cont("marking block %d bad\n", eb);

	if (!mtd->bb_allowed) {
		if (!args.quiet)
			printf("\n");
		return errmsg("bad blocks not supported by this flash");
	}

	err = mtd_mark_bad(mtd, args.node_fd, eb);
	if (err)
		return err;

	si->bad_cnt += 1;
	si->ec[eb] = EB_BAD;

	return consecutive_bad_check(eb);
}

static int flash_image(const struct mtd_dev_info *mtd,
		       const struct ubigen_info *ui, struct ubi_scan_info *si)
{
	int fd, img_ebs, eb, written_ebs = 0, divisor, ret = -1;
	off_t st_size;
	char *buf = NULL;

	fd = open_file(&st_size);
	if (fd < 0)
		return fd;

	buf = malloc(mtd->eb_size);
	if (!buf) {
		sys_errmsg("cannot allocate %d bytes of memory", mtd->eb_size);
		goto out_close;
	}

	img_ebs = st_size / mtd->eb_size;

	if (img_ebs > si->good_cnt) {
		sys_errmsg("file \"%s\" is too large (%lld bytes)",
			   args.image, (long long)st_size);
		goto out_close;
	}

	if (st_size % mtd->eb_size) {
		sys_errmsg("file \"%s\" (size %lld bytes) is not multiple of "
			   "eraseblock size (%d bytes)",
			   args.image, (long long)st_size, mtd->eb_size);
		goto out_close;
	}

	if (st_size == 0) {
		sys_errmsg("file \"%s\" has size 0 bytes", args.image);
		goto out_close;
	}

	verbose(args.verbose, "will write %d eraseblocks", img_ebs);
	divisor = img_ebs;
	for (eb = 0; eb < mtd->eb_cnt; eb++) {
		int err, new_len;
		long long ec;

		if (!args.quiet && !args.verbose) {
			printf("\r" PROGRAM_NAME ": flashing eraseblock %d -- %2u %% complete  ",
			       eb, (eb + 1) * 100 / mtd->eb_cnt);
		}

		if (si->ec[eb] == EB_BAD) {
			divisor += 1;
			continue;
		}

		if (args.verbose) {
			normsg_cont("eraseblock %d: erase", eb);
		}

		err = libmtd_erase(mtd, args.node_fd, eb);
		if (err) {
			if (!args.quiet)
				printf("\n");
			sys_errmsg("failed to erase eraseblock %d", eb);

			if (errno != EIO)
				goto out_close;

			if (mark_bad(mtd, si, eb))
				goto out_close;

			continue;
		}

		err = read_full(fd, buf, mtd->eb_size);
		if (err < 0) {
			sys_errmsg("failed to read eraseblock %d from \"%s\"",
				   written_ebs, args.image);
			goto out_close;
		}

		if (args.override_ec)
			ec = args.ec;
		else if (si->ec[eb] <= EC_MAX)
			ec = si->ec[eb] + 1;
		else
			ec = si->mean_ec;

		if (args.verbose) {
			printf(", change EC to %lld", ec);
		}

		err = change_ech((struct ubi_ec_hdr *)buf, ui->image_seq, ec);
		if (err) {
			errmsg("bad EC header at eraseblock %d of \"%s\"",
			       written_ebs, args.image);
			goto out_close;
		}

		if (args.verbose) {
			printf(", write data\n");
		}

		new_len = drop_ffs(mtd, buf, mtd->eb_size);

		err = libmtd_write(mtd, args.node_fd, eb, 0, buf, new_len);
		if (err) {
			sys_errmsg("cannot write eraseblock %d", eb);

			if (errno != EIO)
				goto out_close;

			err = mtd_torture(mtd, args.node_fd, eb);
			if (err) {
				if (mark_bad(mtd, si, eb))
					goto out_close;
			}
			continue;
		}
		if (++written_ebs >= img_ebs)
			break;
	}

	if (!args.quiet && !args.verbose)
		printf("\n");

	ret = eb + 1;

out_close:
	free(buf);
	close(fd);
	return ret;
}

static int format(const struct mtd_dev_info *mtd,
		  const struct ubigen_info *ui, struct ubi_scan_info *si,
		  int start_eb, int novtbl)
{
	int eb, err, write_size;
	struct ubi_ec_hdr *hdr;
	struct ubi_vtbl_record *vtbl;
	int eb1 = -1, eb2 = -1;
	long long ec1 = -1, ec2 = -1;

	write_size = UBI_EC_HDR_SIZE + mtd->subpage_size - 1;
	write_size /= mtd->subpage_size;
	write_size *= mtd->subpage_size;
	hdr = malloc(write_size);
	if (!hdr)
		return sys_errmsg("cannot allocate %d bytes of memory", write_size);
	memset(hdr, 0xFF, write_size);

	for (eb = start_eb; eb < mtd->eb_cnt; eb++) {
		long long ec;

		if (!args.quiet && !args.verbose) {
			printf("\r" PROGRAM_NAME ": formatting eraseblock %d -- %2u %% complete  ",
			       eb, (eb + 1 - start_eb) * 100 / (mtd->eb_cnt - start_eb));
		}

		if (si->ec[eb] == EB_BAD)
			continue;

		if (args.override_ec)
			ec = args.ec;
		else if (si->ec[eb] <= EC_MAX)
			ec = si->ec[eb] + 1;
		else
			ec = si->mean_ec;
		ubigen_init_ec_hdr(ui, hdr, ec);

		if (args.verbose) {
			normsg_cont("eraseblock %d: erase", eb);
		}

		err = libmtd_erase(mtd, args.node_fd, eb);
		if (err) {
			if (!args.quiet)
				printf("\n");

			sys_errmsg("failed to erase eraseblock %d", eb);
			if (errno != EIO)
				goto out_free;

			if (mark_bad(mtd, si, eb))
				goto out_free;
			continue;
		}

		if ((eb1 == -1 || eb2 == -1) && !novtbl) {
			if (eb1 == -1) {
				eb1 = eb;
				ec1 = ec;
			} else if (eb2 == -1) {
				eb2 = eb;
				ec2 = ec;
			}
			if (args.verbose)
				printf(", do not write EC, leave for vtbl\n");
			continue;
		}

		if (args.verbose) {
			printf(", write EC %lld\n", ec);
		}

		err = libmtd_write(mtd, args.node_fd, eb, 0, hdr, write_size);
		if (err) {
			if (!args.quiet && !args.verbose)
				printf("\n");
			sys_errmsg("cannot write EC header (%d bytes buffer) to eraseblock %d",
				   write_size, eb);

			if (errno != EIO) {
				if (!args.subpage_size != mtd->min_io_size)
					normsg("may be sub-page size is "
					       "incorrect?");
				goto out_free;
			}

			err = mtd_torture(mtd, args.node_fd, eb);
			if (err) {
				if (mark_bad(mtd, si, eb))
					goto out_free;
			}
			continue;

		}
	}

	if (!args.quiet && !args.verbose)
		printf("\n");

	if (!novtbl) {
		if (eb1 == -1 || eb2 == -1) {
			errmsg("no eraseblocks for volume table");
			goto out_free;
		}

		verbose(args.verbose, "write volume table to eraseblocks %d and %d", eb1, eb2);
		vtbl = ubigen_create_empty_vtbl(ui);
		if (!vtbl)
			goto out_free;

		err = ubigen_write_layout_vol(ui, eb1, eb2, ec1,  ec2, vtbl,
					      args.node_fd);
		free(vtbl);
		if (err) {
			errmsg("cannot write layout volume");
			goto out_free;
		}
	}

	free(hdr);
	return 0;

out_free:
	free(hdr);
	return -1;
}

int do_ubiformat(int argc, char *argv[])
{
	int err, verbose;
	struct mtd_dev_info mtd;
	struct ubigen_info ui;
	struct ubi_scan_info *si;

	err = parse_opt(argc, argv);
	if (err)
		return err;

	err = mtd_get_dev_info(args.node, &mtd);
	if (err) {
		sys_errmsg("cannot get information about \"%s\"", args.node);
		goto out_close_mtd;
	}

	if (!is_power_of_2(mtd.min_io_size)) {
		errmsg("min. I/O size is %d, but should be power of 2",
		       mtd.min_io_size);
		goto out_close_mtd;
	}

	if (args.subpage_size && args.subpage_size != mtd.subpage_size) {
		mtd.subpage_size = args.subpage_size;
		args.manual_subpage = 1;
	}

	if (args.manual_subpage) {
		/* Do some sanity check */
		if (args.subpage_size > mtd.min_io_size) {
			errmsg("sub-page cannot be larger than min. I/O unit");
			goto out_close_mtd;
		}

		if (mtd.min_io_size % args.subpage_size) {
			errmsg("min. I/O unit size should be multiple of "
			       "sub-page size");
			goto out_close_mtd;
		}
	}

	args.node_fd = open(args.node, O_RDWR);
	if (args.node_fd < 0) {
		sys_errmsg("cannot open \"%s\"", args.node);
		goto out_close_mtd;
	}

	/* Validate VID header offset if it was specified */
	if (args.vid_hdr_offs != 0) {
		if (args.vid_hdr_offs % 8) {
			errmsg("VID header offset has to be multiple of min. I/O unit size");
			goto out_close;
		}
		if (args.vid_hdr_offs + (int)UBI_VID_HDR_SIZE > mtd.eb_size) {
			errmsg("bad VID header offset");
			goto out_close;
		}
	}

	if (!mtd.writable) {
		errmsg("%s (%s) is a read-only device", mtd.node, args.node);
		goto out_close;
	}

	/* Make sure this MTD device is not attached to UBI */
	/* FIXME! Find a proper way to do this in barebox! */

	if (!args.quiet) {
		normsg_cont("%s (%s), size %lld bytes (%s)", mtd.node, mtd.type_str,
			mtd.size, size_human_readable(mtd.size));
		printf(", %d eraseblocks of %d bytes (%s)", mtd.eb_cnt,
			mtd.eb_size, size_human_readable(mtd.eb_size));
		printf(", min. I/O size %d bytes\n", mtd.min_io_size);
	}

	if (args.quiet)
		verbose = 0;
	else if (args.verbose)
		verbose = 2;
	else
		verbose = 1;
	err = libscan_ubi_scan(&mtd, args.node_fd, &si, verbose);
	if (err) {
		errmsg("failed to scan %s (%s)", mtd.node, args.node);
		goto out_close;
	}

	if (si->good_cnt == 0) {
		errmsg("all %d eraseblocks are bad", si->bad_cnt);
		goto out_free;
	}

	if (si->good_cnt < 2 && (!args.novtbl || args.image)) {
		errmsg("too few non-bad eraseblocks (%d) on %s",
		       si->good_cnt, mtd.node);
		goto out_free;
	}

	if (!args.quiet) {
		if (si->ok_cnt)
			normsg("%d eraseblocks have valid erase counter, mean value is %lld",
			       si->ok_cnt, si->mean_ec);
		if (si->empty_cnt)
			normsg("%d eraseblocks are supposedly empty", si->empty_cnt);
		if (si->corrupted_cnt)
			normsg("%d corrupted erase counters", si->corrupted_cnt);
		print_bad_eraseblocks(&mtd, si);
	}

	if (si->alien_cnt) {
		if (!args.quiet)
			warnmsg("%d of %d eraseblocks contain non-ubifs data",
				si->alien_cnt, si->good_cnt);
		if (!args.yes && !args.quiet)
			warnmsg("use '-y' to force erasing");
		if (!args.yes)
			goto out_free;
	}

	if (!args.override_ec && si->empty_cnt < si->good_cnt) {
		int percent = (si->ok_cnt * 100) / si->good_cnt;

		/*
		 * Make sure the majority of eraseblocks have valid
		 * erase counters.
		 */
		if (percent < 50) {
			if (!args.quiet) {
				warnmsg("only %d of %d eraseblocks have valid erase counter",
					si->ok_cnt, si->good_cnt);
				if (args.yes) {
					normsg("erase counter 0 will be used for all eraseblocks");
					normsg("note, arbitrary erase counter value may be specified using -e option");

				} else {
					warnmsg("use '-y' to force erase counters");
				}
			}

			if (!args.yes)
				goto out_free;

			args.ec = 0;
			args.override_ec = 1;

		} else if (percent < 95) {
			if (!args.quiet) {
				warnmsg("only %d of %d eraseblocks have valid erase counter",
					si->ok_cnt, si->good_cnt);
				if (args.yes)
					normsg("mean erase counter %lld will be used for the rest of eraseblock",
					       si->mean_ec);
				else
					warnmsg("use '-y' to force erase counters");
			}

			if (!args.yes)
				goto out_free;

			args.ec = si->mean_ec;
			args.override_ec = 1;
		}
	}

	if (!args.quiet && args.override_ec)
		normsg("use erase counter %lld for all eraseblocks", args.ec);

	ubigen_info_init(&ui, mtd.eb_size, mtd.min_io_size, mtd.subpage_size,
			 args.vid_hdr_offs, args.ubi_ver, args.image_seq);

	if (si->vid_hdr_offs != -1 && ui.vid_hdr_offs != si->vid_hdr_offs) {
		/*
		 * Hmm, what we read from flash and what we calculated using
		 * min. I/O unit size and sub-page size differs.
		 */
		if (!args.quiet) {
			warnmsg("VID header and data offsets on flash are %d and %d, "
				"which is different to requested offsets %d and %d",
				si->vid_hdr_offs, si->data_offs, ui.vid_hdr_offs,
				ui.data_offs);
			normsg("using offsets %d and %d",  ui.vid_hdr_offs, ui.data_offs);
		}
	}

	if (args.image) {
		err = flash_image(&mtd, &ui, si);
		if (err < 0)
			goto out_free;

		err = format(&mtd, &ui, si, err, 1);
		if (err)
			goto out_free;
	} else {
		err = format(&mtd, &ui, si, 0, args.novtbl);
		if (err)
			goto out_free;
	}

	libscan_ubi_scan_free(si);
	close(args.node_fd);
	return 0;

out_free:
	libscan_ubi_scan_free(si);
out_close:
	close(args.node_fd);
out_close_mtd:
	return 1;
}

BAREBOX_CMD_HELP_START(ubiformat)
BAREBOX_CMD_HELP_USAGE(PROGRAM_NAME " <MTD device file name> [-s <bytes>] [-O <offs>] [-n]\n"
	"\t[-f <file>] [-e <value>] [-x <num>] [-Q <num>] [-y] [-q] [-v]\n")
BAREBOX_CMD_HELP_SHORT("A tool to format MTD devices and flash UBI images\n")
BAREBOX_CMD_HELP_OPT("-s <bytes>", "minimum input/output unit used for UBI headers, "
"e.g. sub-page size in case of NAND flash (equivalent to the minimum input/output "
"unit size by default)\n")
BAREBOX_CMD_HELP_OPT("-O <offs>", "offset if the VID header from start of the "
"physical eraseblock (default is the next minimum I/O unit or sub-page after the EC "
"header)\n")
BAREBOX_CMD_HELP_OPT("-n", "only erase all eraseblock and preserve erase "
"counters, do not write empty volume table\n")
BAREBOX_CMD_HELP_OPT("-f <file>", "flash image file\n")
BAREBOX_CMD_HELP_OPT("-e <value>", "use <value> as the erase counter value for all eraseblocks\n")
BAREBOX_CMD_HELP_OPT("-x <num>", "UBI version number to put to EC headers "
"(default is 1)\n")
BAREBOX_CMD_HELP_OPT("-Q <num>", "32-bit UBI image sequence number to use "
"(by default a random number is picked)\n")
BAREBOX_CMD_HELP_OPT("-q", "suppress progress percentage information\n")
BAREBOX_CMD_HELP_OPT("-v", "be verbose\n")
BAREBOX_CMD_HELP_TEXT(
"Example 1: " PROGRAM_NAME " /dev/nand0 -y - format nand0 and assume yes\n"
"Example 2: " PROGRAM_NAME " /dev/nand0 -q -e 0 - format nand0,\n"
"           be quiet and force erase counter value 0.\n";
)
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(ubiformat)
	.cmd		= do_ubiformat,
	.usage		= "format an ubi volume",
	BAREBOX_CMD_HELP(cmd_ubiformat_help)
BAREBOX_CMD_END
