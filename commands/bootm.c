// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: © 2000-2006 Wolfgang Denk <wd@denx.de>, DENX Software Engineering

/*
 * Boot support
 */
#include <common.h>
#include <driver.h>
#include <command.h>
#include <image.h>
#include <malloc.h>
#include <environment.h>
#include <asm/byteorder.h>
#include <xfuncs.h>
#include <getopt.h>
#include <fcntl.h>
#include <fs.h>
#include <errno.h>
#include <bootm.h>
#include <rtc.h>
#include <init.h>
#include <of.h>
#include <uncompress.h>
#include <memory.h>
#include <filetype.h>
#include <binfmt.h>
#include <globalvar.h>
#include <magicvar.h>
#include <asm-generic/memory_layout.h>

#define BOOTM_OPTS_COMMON "sca:e:vo:fdt:"

#ifdef CONFIG_BOOTM_INITRD
#define BOOTM_OPTS BOOTM_OPTS_COMMON "L:r:"
#else
#define BOOTM_OPTS BOOTM_OPTS_COMMON
#endif

static int do_bootm(int argc, char *argv[])
{
	int opt;
	struct bootm_data data = {};
	int ret = 1;

	bootm_data_init_defaults(&data);

	while ((opt = getopt(argc, argv, BOOTM_OPTS)) > 0) {
		switch(opt) {
		case 'c':
			if (data.verify < BOOTM_VERIFY_HASH)
				data.verify = BOOTM_VERIFY_HASH;
			break;
		case 's':
			data.verify = BOOTM_VERIFY_SIGNATURE;
			break;
#ifdef CONFIG_BOOTM_INITRD
		case 'L':
			data.initrd_address = simple_strtoul(optarg, NULL, 0);
			break;
		case 'r':
			data.initrd_file = optarg;
			break;
#endif
		case 'a':
			data.os_address = simple_strtoul(optarg, NULL, 0);
			break;
		case 'e':
			data.os_entry = simple_strtoul(optarg, NULL, 0);
			break;
		case 'v':
			data.verbose++;
			break;
		case 'o':
			data.oftree_file = optarg;
			break;
		case 'f':
			data.force = 1;
			break;
		case 'd':
			data.dryrun = 1;
			break;
		case 't':
			data.tee_file = optarg;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (optind != argc)
		data.os_file = argv[optind];

	if (!data.os_file) {
		printf("no boot image given\n");
		goto err_out;
	}

	ret = bootm_boot(&data);
	if (ret) {
		printf("handler failed with: %pe\n", ERR_PTR(ret));
		goto err_out;
	}

err_out:
	return ret ? 1 : 0;
}

BAREBOX_CMD_HELP_START(bootm)
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-c\t",  "hash check image integrity")
BAREBOX_CMD_HELP_OPT ("-s\t",  "check signature of image")
BAREBOX_CMD_HELP_OPT ("-d\t",  "dry run: check data, but do not run")
BAREBOX_CMD_HELP_OPT ("-f\t",  "load images even if type is undetectable")
#ifdef CONFIG_BOOTM_INITRD
BAREBOX_CMD_HELP_OPT ("-r INITRD","specify an initrd image")
BAREBOX_CMD_HELP_OPT ("-L ADDR\t","specify initrd load address")
#endif
BAREBOX_CMD_HELP_OPT ("-a ADDR\t","specify os load address")
BAREBOX_CMD_HELP_OPT ("-e OFFS\t","entry point to the image relative to start (0)")
#ifdef CONFIG_OFTREE
BAREBOX_CMD_HELP_OPT ("-o DTB\t","specify open firmware device tree")
#endif
#ifdef CONFIG_BOOTM_OPTEE
BAREBOX_CMD_HELP_OPT ("-t TEE\t","specify TEE image")
#endif
#ifdef CONFIG_BOOTM_VERBOSE
BAREBOX_CMD_HELP_OPT ("-v\t","verbose")
#endif
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(bootm)
	.cmd		= do_bootm,
	BAREBOX_CMD_DESC("boot an application image")
	BAREBOX_CMD_OPTS("[-cdf"
#ifdef CONFIG_BOOTM_INITRD
					  "rL"
#endif
					  "ae"
#ifdef CONFIG_OFTREE
					  "o"
#endif
#ifdef CONFIG_BOOTM_VERBOSE
					  "v"
#endif
#ifdef CONFIG_BOOTM_OPTEE
					  "t"
#endif
					  "] IMAGE")
	BAREBOX_CMD_GROUP(CMD_GRP_BOOT)
	BAREBOX_CMD_HELP(cmd_bootm_help)
BAREBOX_CMD_END

static struct binfmt_hook binfmt_uimage_hook = {
	.type = filetype_uimage,
	.exec = "bootm",
};

static int binfmt_uimage_init(void)
{
	return binfmt_register(&binfmt_uimage_hook);
}
fs_initcall(binfmt_uimage_init);
