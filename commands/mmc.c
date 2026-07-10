// SPDX-License-Identifier: GPL-2.0-only

#include <command.h>
#include <errno.h>
#include <mci.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <linux/kernel.h>
#include <linux/kstrtox.h>
#include <dma.h>

static int mmc_enh_area_setmax(struct mci *mci, u8 *ext_csd)
{
	unsigned i;
	struct {
		unsigned index;
		unsigned value;
	} regval[] = {
		{
			.index = EXT_CSD_ERASE_GROUP_DEF,
			.value = 1,
		}, {
			.index = EXT_CSD_ENH_START_ADDR,
			.value = 0,
		}, {
			.index = EXT_CSD_ENH_START_ADDR + 1,
			.value = 0,
		}, {
			.index = EXT_CSD_ENH_START_ADDR + 2,
			.value = 0,
		}, {
			.index = EXT_CSD_ENH_START_ADDR + 3,
			.value = 0,
		}, {
			.index = EXT_CSD_ENH_SIZE_MULT,
			.value = ext_csd[EXT_CSD_MAX_ENH_SIZE_MULT],
		}, {
			.index = EXT_CSD_ENH_SIZE_MULT + 1,
			.value = ext_csd[EXT_CSD_MAX_ENH_SIZE_MULT + 1],
		}, {
			.index = EXT_CSD_ENH_SIZE_MULT + 2,
			.value = ext_csd[EXT_CSD_MAX_ENH_SIZE_MULT + 2],
		}, {
			.index = EXT_CSD_PARTITIONS_ATTRIBUTE,
			.value = ext_csd[EXT_CSD_PARTITIONS_ATTRIBUTE] | EXT_CSD_ENH_USR_MASK,
		}
	};

	for (i = 0; i < ARRAY_SIZE(regval); ++i) {
		int ret = mci_switch(mci, regval[i].index, regval[i].value);
		if (ret) {
			printf("Failure to write to register %u", regval[i].index);
			return ret;
		}
	}

	return 0;
}

/*
 * Provision a sized enhanced user data area at @start_kib of @length_kib
 * bytes; length is rounded up to the device's WP-group unit. The one-shot
 * PARTITION_SETTING_COMPLETED is not written here.
 */
static int mmc_enh_area_set(struct mci *mci, u8 *ext_csd,
			    u64 start_kib, u64 length_kib)
{
	u32 hc_wp_grp = ext_csd[EXT_CSD_HC_WP_GRP_SIZE];
	u32 hc_erase_grp = ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE];
	u32 max_mult = (u32)ext_csd[EXT_CSD_MAX_ENH_SIZE_MULT] |
		       (u32)ext_csd[EXT_CSD_MAX_ENH_SIZE_MULT + 1] << 8 |
		       (u32)ext_csd[EXT_CSD_MAX_ENH_SIZE_MULT + 2] << 16;
	u64 unit_kib, start_sectors, rem;
	u32 size_mult;
	int i, ret;

	if (!hc_wp_grp || !hc_erase_grp) {
		printf("Device reports zero HC_WP_GRP_SIZE / HC_ERASE_GRP_SIZE\n");
		return -EINVAL;
	}

	/* JEDEC unit for ENH_START_ADDR / ENH_SIZE_MULT */
	unit_kib = (u64)hc_wp_grp * hc_erase_grp * 512;

	div64_u64_rem(start_kib, unit_kib, &rem);
	if (rem) {
		printf("Start %llu KiB not a multiple of unit %llu KiB\n",
		       (unsigned long long)start_kib,
		       (unsigned long long)unit_kib);
		return -EINVAL;
	}

	/* Round length up so the requested region is fully covered */
	size_mult = DIV_ROUND_UP_ULL(length_kib, unit_kib);

	if (size_mult > max_mult) {
		printf("Requested %llu KiB (mult=%u) exceeds MAX_ENH_SIZE_MULT=%u\n",
		       (unsigned long long)length_kib, size_mult, max_mult);
		return -EINVAL;
	}

	/* ENH_START_ADDR is in 512-byte sectors */
	start_sectors = start_kib * 2;

	ret = mci_switch(mci, EXT_CSD_ERASE_GROUP_DEF, 1);
	if (ret) {
		printf("Failure to write EXT_CSD_ERASE_GROUP_DEF\n");
		return ret;
	}

	for (i = 0; i < 4; i++) {
		ret = mci_switch(mci, EXT_CSD_ENH_START_ADDR + i,
				 (start_sectors >> (8 * i)) & 0xff);
		if (ret) {
			printf("Failure to write EXT_CSD_ENH_START_ADDR[%d]\n", i);
			return ret;
		}
	}

	for (i = 0; i < 3; i++) {
		ret = mci_switch(mci, EXT_CSD_ENH_SIZE_MULT + i,
				 (size_mult >> (8 * i)) & 0xff);
		if (ret) {
			printf("Failure to write EXT_CSD_ENH_SIZE_MULT[%d]\n", i);
			return ret;
		}
	}

	ret = mci_switch(mci, EXT_CSD_PARTITIONS_ATTRIBUTE,
			 ext_csd[EXT_CSD_PARTITIONS_ATTRIBUTE] |
			 EXT_CSD_ENH_USR_MASK);
	if (ret) {
		printf("Failure to write EXT_CSD_PARTITIONS_ATTRIBUTE\n");
		return ret;
	}

	printf("Enhanced user data area: start=%llu KiB, size=%llu KiB "
	       "(mult=%u, unit=%llu KiB, max_mult=%u)\n",
	       (unsigned long long)start_kib,
	       (unsigned long long)size_mult * unit_kib,
	       size_mult, (unsigned long long)unit_kib, max_mult);

	return 0;
}

static int mmc_partitioning_complete(struct mci *mci)
{
	int ret;

	ret = mci_switch(mci, EXT_CSD_PARTITION_SETTING_COMPLETED, 1);
	if (ret)
		printf("Failure to write to EXT_CSD_PARTITION_SETTING_COMPLETED\n");

	return ret;
}

/*
 * enh_area [-c] /dev/mmcX
 *     -> fill the entire user area (legacy)
 * enh_area set [-c] <start_KiB> <length_KiB> /dev/mmcX
 *     -> sized, mmc-utils compatible
 */
static int do_mmc_enh_area(int argc, char *argv[])
{
	const char *devpath;
	struct mci *mci;
	u8 *ext_csd;
	u64 start_kib = 0, length_kib = 0;
	bool sized = false;
	int set_completed = 0;
	int opt;
	int ret;

	if (argc >= 2 && !strcmp(argv[1], "set")) {
		sized = true;
		/* Shift past "set" so getopt() and the trailing positional
		 * arguments line up the same way as for the legacy form. */
		argv++;
		argc--;
	}

	optind = 1;
	while ((opt = getopt(argc, argv, "c")) > 0) {
		switch (opt) {
		case 'c':
			printf("Use -c to complete the partitioning is deprecated, use separate partition_complete command instead\n");
			set_completed = 1;
			break;
		}
	}

	if (sized) {
		if (argc - optind != 3) {
			printf("Usage: mmc enh_area set [-c] <start_KiB> <length_KiB> /dev/mmcX\n");
			return COMMAND_ERROR_USAGE;
		}
		if (kstrtou64(argv[optind], 0, &start_kib) ||
		    kstrtou64(argv[optind + 1], 0, &length_kib)) {
			printf("Invalid start/length value\n");
			return COMMAND_ERROR_USAGE;
		}
		devpath = argv[optind + 2];
	} else {
		if (argc - optind != 1) {
			printf("Usage: mmc enh_area [-c] /dev/mmcX\n");
			return COMMAND_ERROR_USAGE;
		}
		devpath = argv[optind];
	}

	mci = mci_get_device_by_devpath(devpath);
	if (!mci) {
		printf("Failure to open %s as mci device\n", devpath);
		return COMMAND_ERROR;
	}

	ext_csd = mci_get_ext_csd(mci);
	if (IS_ERR(ext_csd))
		return COMMAND_ERROR;

	if (!(ext_csd[EXT_CSD_PARTITIONING_SUPPORT] & EXT_CSD_ENH_ATTRIBUTE_EN_MASK)) {
		printf("Device doesn't support enhanced area\n");
		goto error;
	}

	if (ext_csd[EXT_CSD_PARTITION_SETTING_COMPLETED]) {
		printf("Partitioning already finalized\n");
		goto error;
	}

	if (sized)
		ret = mmc_enh_area_set(mci, ext_csd, start_kib, length_kib);
	else
		ret = mmc_enh_area_setmax(mci, ext_csd);
	if (ret)
		goto error;

	dma_free(ext_csd);

	if (set_completed) {
		ret = mmc_partitioning_complete(mci);
		if (ret)
			return COMMAND_ERROR;
		printf("Now power cycle the device to let it reconfigure itself.\n");
	}

	return COMMAND_SUCCESS;

error:
	dma_free(ext_csd);
	return COMMAND_ERROR;
}

static int do_mmc_write_reliability(int argc, char *argv[])
{
	const char *devpath;
	struct mci *mci;
	u8 *ext_csd;
	int ret;

	if (argc - optind != 1) {
		printf("Usage: mmc write_reliability /dev/mmcX\n");
		return COMMAND_ERROR_USAGE;
	}

	devpath = argv[optind];

	mci = mci_get_device_by_devpath(devpath);
	if (!mci) {
		printf("Failure to open %s as mci device\n", devpath);
		return COMMAND_ERROR;
	}

	ext_csd = mci_get_ext_csd(mci);
	if (IS_ERR(ext_csd))
		return COMMAND_ERROR;

	if (ext_csd[EXT_CSD_PARTITION_SETTING_COMPLETED]) {
		printf("Partitioning already finalized\n");
		goto error;
	}

	if (!(ext_csd[EXT_CSD_WR_REL_PARAM] & EXT_CSD_EN_REL_WR)) {
		printf("Device doesn't support the enhanced definition of reliable write\n");
		goto error;
	}

	/* Some cards may have HS_CTRL_REL set as 0. This is obsolete
	 * per-spec, but seems to indicate that write reliability was
	 * factory-set. Therefore, just bail out early here
	 */
	if ((ext_csd[EXT_CSD_WR_REL_SET] & 0x1f) == 0x1f) {
		printf("reliable write already set\n");
		goto out;
	}

	if (!(ext_csd[EXT_CSD_WR_REL_PARAM] & EXT_CSD_HS_CTRL_REL)) {
		printf("Device doesn't support WR_REL_SET writes\n");
		goto error;
	}

	/*
	 * Host has one opportunity to write all of the bits. Separate writes to
	 * individual bits are not permitted so set all bits for now.
	 */
	ret = mci_switch(mci, EXT_CSD_WR_REL_SET, 0x1f);
	if (ret) {
		printf("Failure to write to EXT_CSD_WR_REL_SET\n");
		goto error;
	}

out:
	dma_free(ext_csd);

	return COMMAND_SUCCESS;

error:
	dma_free(ext_csd);
	return COMMAND_ERROR;
}

static int do_mmc_partition_complete(int argc, char *argv[])
{
	const char *devpath;
	struct mci *mci;
	int ret;

	if (argc - optind != 1) {
		printf("Usage: mmc partition_complete /dev/mmcX\n");
		return COMMAND_ERROR_USAGE;
	}

	devpath = argv[optind];

	mci = mci_get_device_by_devpath(devpath);
	if (!mci) {
		printf("Failure to open %s as mci device\n", devpath);
		return COMMAND_ERROR_USAGE;
	}

	ret = mmc_partitioning_complete(mci);
	if (ret)
		return COMMAND_ERROR;

	printf("Now power cycle the device to let it reconfigure itself.\n");

	return COMMAND_SUCCESS;
}

static struct {
	const char *cmd;
	int (*func)(int argc, char *argv[]);
} mmc_subcmds[] = {
	{
		.cmd = "enh_area",
		.func = do_mmc_enh_area,
	}, {
		.cmd = "write_reliability",
		.func = do_mmc_write_reliability,
	}, {
		.cmd = "partition_complete",
		.func = do_mmc_partition_complete,
	}
};

static int do_mmc(int argc, char *argv[])
{
	size_t i;
	int (*func)(int argc, char *argv[]) = NULL;

	if (argc < 2) {
		printf("mmc: required subcommand missing\n");
		return 1;
	}

	for (i = 0; i < ARRAY_SIZE(mmc_subcmds); ++i) {
		if (strcmp(mmc_subcmds[i].cmd, argv[1]) == 0) {
			func = mmc_subcmds[i].func;
			break;
		}
	}

	if (func) {
		return func(argc - 1, argv + 1);
	} else {
		printf("mmc: subcommand \"%s\" not found\n", argv[1]);
		return COMMAND_ERROR_USAGE;
	}
}

BAREBOX_CMD_HELP_START(mmc)
BAREBOX_CMD_HELP_TEXT("Modifies mmc properties.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Subcommand enh_area without arguments creates an enhanced")
BAREBOX_CMD_HELP_TEXT("area of maximal size; enh_area set provisions a region of the")
BAREBOX_CMD_HELP_TEXT("requested length (rounded up to the device's enhanced-area unit).")
BAREBOX_CMD_HELP_TEXT("Note, with -c this is an irreversible action.")
BAREBOX_CMD_HELP_OPT("-c", "complete partitioning (deprecated)")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("The subcommand write_reliability enable write reliability")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("The subcommand partition_complete set PARTITION_SETTING_COMPLETED (irreversible action)")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(mmc)
	.cmd = do_mmc,
	BAREBOX_CMD_OPTS("partition_complete|write_reliability|enh_area [set [-c] <start_KiB> <length_KiB>|[-c]] /dev/mmcX")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_mmc_help)
BAREBOX_CMD_END
