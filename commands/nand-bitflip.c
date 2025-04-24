// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <getopt.h>
#include <fs.h>
#include <fcntl.h>
#include <ioctl.h>
#include <linux/mtd/mtd.h>
#include <mtd/mtd-peb.h>

static int bitflip_mtd_read(struct mtd_info *mtd, loff_t from, size_t len,
			    size_t *retlen, void *buf)
{
	int ret_code;

	if (mtd->_read_oob) {
		struct mtd_oob_ops ops = {
			.len = len,
			.datbuf = buf,
		};

		ret_code = mtd->_read_oob(mtd, from, &ops);
	} else {
		ret_code = mtd->_read(mtd, from, len, retlen, buf);
	}

	return ret_code;
}

static int do_nand_bitflip(int argc, char *argv[])
{
	int opt, ret, fd;
	static struct mtd_info_user meminfo;
	int block = 0;
	int random = 0;
	int num_bitflips = 1;
	loff_t offset = 0, roffset;
	int check = 0;
	size_t r;
	void *buf;

	while ((opt = getopt(argc, argv, "b:rn:o:c")) > 0) {
		switch (opt) {
		case 'r':
			random = 1;
			break;
		case 'n':
			num_bitflips = simple_strtoul(optarg, NULL, 0);
			break;
		case 'b':
			block = simple_strtoul(optarg, NULL, 0);
			break;
		case 'o':
			offset = strtoull_suffix(optarg, NULL, 0);
			break;
		case 'c':
			check = 1;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (optind >= argc)
		return COMMAND_ERROR_USAGE;

	fd = open(argv[optind], O_RDWR);
	if (fd < 0)
		return fd;

	ret = ioctl(fd, MEMGETINFO, &meminfo);

	close(fd);

	if (ret)
		return ret;

	block += mtd_div_by_eb(offset, meminfo.mtd);
	offset = mtd_mod_by_eb(offset, meminfo.mtd);
	if ((int)offset % meminfo.mtd->writesize) {
		printf("offset has to be pagesize aligned\n");
		return 1;
	}

	if (!check) {
		ret = mtd_peb_create_bitflips(meminfo.mtd, block, offset, meminfo.writesize,
					      num_bitflips, random, 1);
		if (ret) {
			printf("Creating bitflips failed with: %pe\n", ERR_PTR(ret));
			return ret;
		}
	}

	buf = xzalloc(meminfo.writesize);

	roffset = (loff_t)block * meminfo.mtd->erasesize + offset;
	ret = bitflip_mtd_read(meminfo.mtd, roffset, meminfo.writesize, &r, buf);
	if (ret > 0) {
		printf("page at block %d, offset 0x%08llx has %d bitflips%s\n",
		       block, offset, ret,
		       ret >= meminfo.mtd->bitflip_threshold ? ", needs cleanup" : "");
	} else if (!ret) {
		printf("No bitflips found on block %d, offset 0x%08llx\n", block, offset);
	} else {
		printf("Reading block %d, offset 0x%08llx failed with: %pe\n", block, offset,
		       ERR_PTR(ret));
	}

	free(buf);

	return 0;
}

BAREBOX_CMD_HELP_START(nand_bitflip)
BAREBOX_CMD_HELP_TEXT("This command creates bitflips on Nand pages.")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-b <block>",  "block to work on")
BAREBOX_CMD_HELP_OPT ("-c\t",  "Check only for bitflips")
BAREBOX_CMD_HELP_OPT ("-o <offset>",  "offset in Nand")
BAREBOX_CMD_HELP_OPT ("-r\t",  "flip random bits")
BAREBOX_CMD_HELP_OPT ("-n <numbitflips>",  "Specify maximum number of bitflips to generate")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(nand_bitflip)
	.cmd		= do_nand_bitflip,
	BAREBOX_CMD_DESC("Create bitflips on Nand pages")
	BAREBOX_CMD_OPTS("NANDDEV")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_nand_bitflip_help)
BAREBOX_CMD_END
