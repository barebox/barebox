// SPDX-License-Identifier: GPL-2.0-only

/*
 * Based on nandtest.c source in mtd-utils package.
 */
#include <common.h>
#include <command.h>
#include <fs.h>
#include <errno.h>
#include <malloc.h>
#include <getopt.h>
#include <ioctl.h>
#include <linux/mtd/mtd-abi.h>
#include <fcntl.h>
#include <stdlib.h>
#include <progress.h>

/* Max ECC Bits that can be corrected */
#define MAX_ECC_BITS 8

/*
 * Structures for flash memory information.
 */
static struct region_info_user memregion;
static struct mtd_info_user meminfo;
static struct mtd_ecc_stats oldstats, newstats;

static int fd, seed;
/* Markbad option flag */
static int markbad;

/* ECC-Calculation stats  */
static unsigned int ecc_stats[MAX_ECC_BITS];
static unsigned int ecc_stats_over;
static unsigned int ecc_failed_cnt;

/* Helper for progression bar */
static loff_t pb_len;
static loff_t pb_start;

static void pb_init(void)
{
	init_progression_bar(pb_len);
}

static void pb_update(const loff_t done)
{
	show_progress(done - pb_start);
}

static void pb_rewrite(const loff_t done)
{
	pb_init();
	pb_update(done);
}


/*
 * Implementation of pwrite with lseek and write.
 */
static ssize_t __pwrite(int fd, const void *buf,
		size_t count, loff_t offset)
{
	ssize_t ret;

	/* Write buf to flash */
	ret = pwrite(fd, buf, count, offset);
	if (ret < 0) {
		perror("pwrite");
		if (markbad) {
			printf("\nMark block bad at 0x%08llx\n",
					offset + memregion.offset);
			ioctl(fd, MEMSETBADBLOCK, &offset);
			pb_rewrite(offset);
		}
	}

	flush(fd);
	return ret;
}

static int _read_get_stats(loff_t ofs, unsigned char *buf)
{
	int ret;

	/* Read data from offset */
	pread(fd, buf, meminfo.writesize, ofs);

	ret = ioctl(fd, ECCGETSTATS, &newstats);
	if (ret < 0) {
		perror("\nECCGETSTATS");
		return ret;
	}

	if (newstats.corrected > oldstats.corrected) {
		printf("\n %d bit(s) ECC corrected at page 0x%08llx\n",
				newstats.corrected - oldstats.corrected,
				ofs + memregion.offset);
		pb_rewrite(ofs);
		if ((newstats.corrected-oldstats.corrected) >=
				MAX_ECC_BITS) {
			/* Increment ECC stats that
			 * are over MAX_ECC_BITS */
			ecc_stats_over++;
		} else {
			/* Increment ECC stat value */
			ecc_stats[(newstats.corrected -
					oldstats.corrected) - 1]++;
		}
		/* Set oldstats to newstats */
		oldstats.corrected = newstats.corrected;
	}

	if (newstats.failed > oldstats.failed) {
		printf("\nECC failed at page 0x%08llx\n",
				ofs + memregion.offset);
		pb_rewrite(ofs);
		oldstats.failed = newstats.failed;
		ecc_failed_cnt++;
	}

	return 0;
}

/*
 * Read and report correctec ECC bits.
 * Param ofs: offset on flash_device.
 * Param rbuf: pointer to allocated buffer to copy readed data.
 * Param length: length of testing area
 */
static int read_corrected(loff_t ofs, unsigned char *rbuf, loff_t length)
{
	unsigned int i;
	int ret;

	for (i = 0; i < meminfo.erasesize;
			i += meminfo.writesize) {
		ret = _read_get_stats(ofs + i, rbuf + i);
		if (ret)
			return ret;
	}

	return 0;
}

/*
 * Erase and write function.
 * Param ofs: offset on flash_device.
 * Param data: data to write on flash.
 * Param rbuf: pointer to allocated buffer to copy readed data.
 * Param length: length of testing area
 */
static int erase_and_write(loff_t ofs, unsigned char *data,
		unsigned char *rbuf, loff_t length)
{
	struct erase_info_user er;
	unsigned int i;
	int ret;

	er.start = ofs;
	er.length = meminfo.erasesize;

	ret = erase(fd, er.length, er.start);
	if (ret < 0) {
		perror("\nerase");
		printf("Could't not erase flash at 0x%08llx length 0x%08llx.\n",
			   er.start, er.length);
		return ret;
	}

	for (i = 0; i < meminfo.erasesize;
			i += meminfo.writesize) {
		/* Write data to given offset */
		__pwrite(fd, data + i, meminfo.writesize,
				ofs + i);

		ret = _read_get_stats(ofs + i, rbuf + i);
		if (ret)
			return ret;
	}

	/* Compared written data with read data.
	 * If data is not identical, display a detailed
	 * debugging information. */
	ret = memcmp(data, rbuf, meminfo.erasesize);
	if (ret < 0) {
		printf("\ncompare failed. seed %d\n", seed);
		for (i = 0; i < meminfo.erasesize; i++) {
			if (data[i] != rbuf[i])
				printf("Block 0x%llx byte 0x%0x (page 0x%x offset 0x%x) is %02x should be %02x\n",
				       div64_ul(ofs, meminfo.erasesize), i,
				       i / meminfo.writesize, i % meminfo.writesize,
				       rbuf[i], data[i]);
		}
		return ret;
	}
	return 0;
}

/* Print stats of nandtest. */
static void print_stats(int nr_passes, loff_t length)
{
	unsigned int i;
	uint64_t blocks = (uint64_t)length;

	do_div(blocks, meminfo.erasesize);
	printf("-------- Summary --------\n");
	printf("Tested blocks		: %lld\n", blocks * nr_passes);

	for (i = 0; i < MAX_ECC_BITS; i++)
		printf("ECC %d bit error(s)	: %u\n", i + 1, ecc_stats[i]);

	printf("ECC >%d bit error(s)	: %u\n", MAX_ECC_BITS, ecc_stats_over);
	printf("ECC corrections failed	: %u\n", ecc_failed_cnt);
	printf("-------------------------\n");
}

/* Main program. */
static int do_nandtest(int argc, char *argv[])
{
	int opt, do_nandtest_dev = -1, do_nandtest_ro = 0, ret = -1;
	loff_t flash_offset = 0, test_ofs, length = 0, flash_end = 0;
	unsigned int nr_iterations = 1, iter;
	unsigned char *wbuf, *rbuf;

	ecc_failed_cnt = 0;
	ecc_stats_over = 0;
	markbad = 0;
	fd = -1;

	memset(ecc_stats, 0, sizeof(ecc_stats));

	while ((opt = getopt(argc, argv, "ms:i:o:l:tr")) > 0) {
		switch (opt) {
		case 'm':
			markbad = 1;
			break;
		case 's':
			seed = simple_strtoul(optarg, NULL, 0);
			break;
		case 'i':
			nr_iterations = simple_strtoul(optarg, NULL, 0);
			break;
		case 'o':
			flash_offset = simple_strtoul(optarg, NULL, 0);
			break;
		case 'l':
			length = simple_strtoul(optarg, NULL, 0);
			break;
		case 't':
			do_nandtest_dev = 1;
			break;
		case 'r':
			do_nandtest_dev = 1;
			do_nandtest_ro = 1;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	/* Check if no device is given */
	if (optind >= argc)
		return COMMAND_ERROR_USAGE;

	if (do_nandtest_dev == -1) {
		printf("Please add -t or -r parameter to start nandtest.\n");
		return 0;
	}

	printf("Open device %s\n", argv[optind]);

	fd = open(argv[optind], O_RDWR);
	if (fd < 0) {
		perror("open");
		return COMMAND_ERROR_USAGE;
	}

	/* Getting flash information. */

	ret = ioctl(fd, MEMGETINFO, &meminfo);
	if (ret < 0) {
		perror("MEMGETINFO");
		goto err;
	}

	ret = ioctl(fd, MEMGETREGIONINFO, &memregion);
	if (ret < 0) {
		perror("MEMGETREGIONINFO");
		goto err;
	}

	ret = ioctl(fd, ECCGETSTATS, &oldstats);
	if (ret < 0) {
		perror("ECCGETSTATS");
		goto err;
	}

	if (!length) {
		length = meminfo.size;
		length -= flash_offset;
	}

	printf("Flash offset: 0x%08llx\n",
			flash_offset + memregion.offset);
	printf("Length: 0x%08llx\n", length);
	printf("End address: 0x%08llx\n",
			flash_offset + length + memregion.offset);
	printf("Erasesize: 0x%08x\n", meminfo.erasesize);
	printf("Starting nandtest...\n");

	if (!IS_ALIGNED(meminfo.erasesize, meminfo.writesize)) {
		printf("Erasesize 0x%08x is not a multiple "
				"of writesize 0x%08x.\n"
				"Please check driver implementation\n",
				meminfo.erasesize, meminfo.writesize);
		goto err;
	}
	if (!IS_ALIGNED(flash_offset, meminfo.erasesize)) {
		printf("Offset 0x%08llx not multiple of erase size 0x%08x\n",
			flash_offset, meminfo.erasesize);
		goto err;
	}
	if (!IS_ALIGNED(length, meminfo.erasesize)) {
		printf("Length 0x%08llx not multiple of erase size 0x%08x\n",
			length, meminfo.erasesize);
		goto err;
	}
	if (length + flash_offset > meminfo.size) {
		printf("Length 0x%08llx + offset 0x%08llx exceeds "
				"device size 0x%08llx\n", length,
				flash_offset, meminfo.size);
		goto err;
	}

	flash_end = length + flash_offset;

	wbuf = malloc(meminfo.erasesize * 2);
	if (!wbuf) {
		printf("Could not allocate %d bytes for buffer\n",
			meminfo.erasesize * 2);
		goto err;
	}
	rbuf = wbuf + meminfo.erasesize;

	pb_start = flash_offset;
	pb_len = length;

	for (iter = 0; iter < nr_iterations; iter++) {
		pb_init();
		for (test_ofs = flash_offset;
				test_ofs < flash_end;
				test_ofs += meminfo.erasesize) {
			pb_update(test_ofs);
			srand(seed);
			seed = rand();

			if (ioctl(fd, MEMGETBADBLOCK, &test_ofs)) {
				printf("\nBad block at 0x%08llx\n",
						test_ofs + memregion.offset);
				pb_rewrite(test_ofs);
				continue;
			}
			if (do_nandtest_ro) {
				ret = read_corrected(test_ofs, rbuf, length);
			} else {
				get_random_bytes(wbuf, meminfo.erasesize);
				ret = erase_and_write(test_ofs, wbuf,
						rbuf, length);
			}
			if (ret < 0)
				goto err2;
		}
		pb_update(test_ofs);
		printf("\nFinished pass %d successfully\n", iter + 1);
	}

	print_stats(nr_iterations, length);

	ret = close(fd);
	if (ret < 0) {
		perror("close");
		goto err2;
	}

	free(wbuf);

	return 0;
err2:
	free(wbuf);
err:
	printf("Error occurred.\n");
	close(fd);
	return 1;
}

BAREBOX_CMD_HELP_START(nandtest)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-t",  "Really do a nandtest on device")
BAREBOX_CMD_HELP_OPT ("-r",  "Readonly nandtest on device")
BAREBOX_CMD_HELP_OPT ("-m",  "Mark blocks bad if they appear so")
BAREBOX_CMD_HELP_OPT ("-s SEED",   "supply random seed")
BAREBOX_CMD_HELP_OPT ("-i ITERATIONS",  "nNumber of iterations")
BAREBOX_CMD_HELP_OPT ("-o OFFS",  "start offset on flash")
BAREBOX_CMD_HELP_OPT ("-l LEN",   "length of flash to test")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(nandtest)
	.cmd		= do_nandtest,
	BAREBOX_CMD_DESC("NAND flash memory test")
	BAREBOX_CMD_OPTS("[-tmsiol] NANDDEVICE")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_nandtest_help)
BAREBOX_CMD_END
