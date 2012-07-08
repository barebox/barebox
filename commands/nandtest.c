/*
 * Based on nandtest.c source in mtd-utils package.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

/*
 * Implementation of pread with lseek and read.
 */
static ssize_t pread(int fd, void *buf, size_t count, off_t offset)
{
	int ret;

	/* Seek to offset */
	ret = lseek(fd, offset, SEEK_SET);
	if (ret < 0)
		perror("lseek");

	/* Read from flash and put it into buf  */
	ret = read(fd, buf, count);
	if (ret < 0)
		perror("read");

	return 0;
}

/*
 * Implementation of pwrite with lseek and write.
 */
static ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset)
{
	int ret;

	ret = lseek(fd, offset, SEEK_SET);
	if (ret < 0)
		perror("lseek");

	/* Write buf to flash */
	ret = write(fd, buf, count);
	if (ret < 0) {
		perror("write");
		if (markbad) {
			printf("Mark block bad at 0x%08x\n",
					(unsigned)(offset + memregion.offset));
			ioctl(fd, MEMSETBADBLOCK, &offset);
		}
	}

	flush(fd);
	return 0;
}

/*
 * Erase and write function.
 * Param ofs: offset on flash_device.
 * Param data: data to write on flash.
 * Param rbuf: pointer to allocated buffer to copy readed data.
 */
static int erase_and_write(off_t ofs, unsigned char *data, unsigned char *rbuf)
{
	struct erase_info_user er;
	int i, ret;

	printf("\r0x%08x: erasing... ", (unsigned)(ofs + memregion.offset));

	er.start = ofs;
	er.length = meminfo.erasesize;

	ret = erase(fd, er.length, er.start);
	if (ret < 0) {
		perror("erase");
		printf("Could't not erase flash at 0x%08x length 0x%08x.\n",
			   er.start, er.length);
		return ret;
	}

	printf("\r0x%08x: writing...", (unsigned)(ofs + memregion.offset));

	/* Write data to given offset */
	pwrite(fd, data, meminfo.erasesize, ofs);

	printf("\r0x%08x: reading...", (unsigned)(ofs + memregion.offset));

	/* Read data from offset */
	pread(fd, rbuf, meminfo.erasesize, ofs);

	ret = ioctl(fd, ECCGETSTATS, &newstats);
	if (ret < 0) {
		perror("ECCGETSTATS");
		return ret;
	}

	if (newstats.corrected > oldstats.corrected) {
		printf("\n %d bit(s) ECC corrected at 0x%08x\n",
				newstats.corrected - oldstats.corrected,
				(unsigned)(ofs + memregion.offset));
		if ((newstats.corrected-oldstats.corrected) >= MAX_ECC_BITS) {
			/* Increment ECC stats that are over MAX_ECC_BITS */
			ecc_stats_over++;
		} else {
			/* Increment ECC stat value */
			ecc_stats[(newstats.corrected-oldstats.corrected)-1]++;
		}
		/* Set oldstats to newstats */
		oldstats.corrected = newstats.corrected;
	}
	if (newstats.failed > oldstats.failed) {
		printf("\nECC failed at 0x%08x\n",
				(unsigned)(ofs + memregion.offset));
		oldstats.failed = newstats.failed;
		ecc_failed_cnt++;
	}

	printf("\r0x%08x: checking...", (unsigned)(ofs + memregion.offset));

	/* Compared written data with read data.
	 * If data is not identical, display a detailed
	 * debugging information. */
	ret = memcmp(data, rbuf, meminfo.erasesize);
	if (ret < 0) {
		printf("\ncompare failed. seed %d\n", seed);
		for (i = 0; i < meminfo.erasesize; i++) {
			if (data[i] != rbuf[i])
				printf("Byte 0x%x is %02x should be %02x\n",
				       i, rbuf[i], data[i]);
		}
		return ret;
	}
	return 0;
}

/* Print stats of nandtest. */
static void print_stats(int nr_passes, int length)
{
	int i;
	printf("-------- Summary --------\n");
	printf("Tested blocks		: %d\n", (length/meminfo.erasesize)
			* nr_passes);

	for (i = 0; i < MAX_ECC_BITS; i++)
		printf("ECC %d bit error(s)	: %d\n", i+1, ecc_stats[i]);

	printf("ECC >%d bit error(s)	: %d\n", MAX_ECC_BITS, ecc_stats_over);
	printf("ECC corrections failed	: %d\n", ecc_failed_cnt);
	printf("-------------------------\n");
}

/* Main program. */
static int do_nandtest(int argc, char *argv[])
{
	int opt, length = -1, do_nandtest_dev = -1;
	off_t flash_offset = 0;
	off_t test_ofs;
	unsigned int nr_passes = 1, pass;
	int i;
	int ret = -1;
	unsigned char *wbuf, *rbuf;

	ecc_failed_cnt = 0;
	ecc_stats_over = 0;
	markbad = 0;
	fd = -1;

	memset(ecc_stats, 0, MAX_ECC_BITS);

	while ((opt = getopt(argc, argv, "ms:p:o:l:t")) > 0) {
		switch (opt) {
		case 'm':
			markbad = 1;
			break;
		case 's':
			seed = simple_strtoul(optarg, NULL, 0);
			break;
		case 'p':
			nr_passes = simple_strtoul(optarg, NULL, 0);
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
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	/* Check if no device is given */
	if (optind >= argc)
		return COMMAND_ERROR_USAGE;

	if (do_nandtest_dev == -1) {
		printf("Please add -t parameter to start nandtest.\n");
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

	if (length == -1) {
		length = meminfo.size;
		length -= flash_offset;
	}

	printf("Flash offset: 0x%08x\n",
			(unsigned)(flash_offset+memregion.offset));
	printf("Length: 0x%08x\n", (unsigned)length);
	printf("End address: 0x%08x\n",
			(unsigned)(flash_offset+length+memregion.offset));
	printf("Erasesize: 0x%08x\n", (unsigned)(meminfo.erasesize));
	printf("Starting nandtest...\n");

	if (flash_offset % meminfo.erasesize) {
		printf("Offset 0x%08x not multiple of erase size 0x%08x\n",
			(unsigned)flash_offset, meminfo.erasesize);
		goto err;
	}
	if (length % meminfo.erasesize) {
		printf("Length 0x%08x not multiple of erase size 0x%08x\n",
			length, meminfo.erasesize);
		goto err;
	}
	if (length + flash_offset > meminfo.size) {
		printf("Length 0x%08x + offset 0x%08x exceeds "
				"device size 0x%08x\n",
			length, (unsigned)flash_offset, meminfo.size);
		goto err;
	}

	wbuf = malloc(meminfo.erasesize * 2);
	if (!wbuf) {
		printf("Could not allocate %d bytes for buffer\n",
			meminfo.erasesize * 2);
		goto err;
	}
	rbuf = wbuf + meminfo.erasesize;

	for (pass = 0; pass < nr_passes; pass++) {
		for (test_ofs = flash_offset;
				test_ofs < flash_offset+length;
				test_ofs += meminfo.erasesize) {
			loff_t __test_ofs = test_ofs;
			srand(seed);
			seed = rand();

			if (ioctl(fd, MEMGETBADBLOCK, &__test_ofs)) {
				printf("\rBad block at 0x%08x\n",
						(unsigned)(test_ofs +
							memregion.offset));
				continue;
			}

			for (i = 0; i < meminfo.erasesize; i++)
				wbuf[i] = rand();

			ret = erase_and_write(test_ofs, wbuf, rbuf);
			if (ret < 0)
				goto err2;
		}
		printf("\nFinished pass %d successfully\n", pass+1);
	}

	print_stats(nr_passes, length);

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

/* String for usage of nandtest */
static const __maybe_unused char cmd_nandtest_help[] =
"Usage: nandtest [OPTION] <device>\n"
		"  -t,	Really do a nandtest on device.\n"
		"  -m,	Mark blocks bad if they appear so.\n"
		"  -s	<seed>,	Supply random seed.\n"
		"  -p	<passes>, Number of passes.\n"
		"  -o	<offset>, Start offset on flash.\n"
		"  -l	<length>, Length of flash to test.\n";

BAREBOX_CMD_START(nandtest)
	.cmd		= do_nandtest,
	.usage		= "NAND Test",
	BAREBOX_CMD_HELP(cmd_nandtest_help)
BAREBOX_CMD_END
