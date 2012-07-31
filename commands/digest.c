/*
 * digest.c - Calculate a md5/sha1/sha256 checksum of a memory area
 *
 * Copyright (c) 2011 Peter Korsgaard <jacmet@sunsite.dk>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
 */

#include <common.h>
#include <command.h>
#include <fs.h>
#include <fcntl.h>
#include <errno.h>
#include <xfuncs.h>
#include <malloc.h>
#include <digest.h>

static int do_digest(char *algorithm, int argc, char *argv[])
{
	struct digest *d;
	int ret = 0;
	int i;
	unsigned char *hash;

	d = digest_get_by_name(algorithm);
	BUG_ON(!d);

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	hash = calloc(d->length, sizeof(unsigned char));
	if (!hash) {
		perror("calloc");
		return COMMAND_ERROR_USAGE;
	}

	argv++;
	while (*argv) {
		char *filename = "/dev/mem";
		loff_t start = 0, size = ~0;

		/* arguments are either file, file+area or area */
		if (parse_area_spec(*argv, &start, &size)) {
			filename = *argv;
			if (argv[1] && !parse_area_spec(argv[1], &start, &size))
				argv++;
		}

		if (digest_file_window(d, filename, hash, start, size) < 0) {
			ret = 1;
		} else {
			for (i = 0; i < d->length; i++)
				printf("%02x", hash[i]);

			printf("  %s\t0x%08llx ... 0x%08llx\n",
				filename, start, start + size);
		}

		argv++;
	}

	free(hash);

	return ret;
}

#ifdef CONFIG_CMD_MD5SUM

static int do_md5(int argc, char *argv[])
{
	return do_digest("md5", argc, argv);
}

BAREBOX_CMD_HELP_START(md5sum)
BAREBOX_CMD_HELP_USAGE("md5sum [[FILE] [AREA]]...\n")
BAREBOX_CMD_HELP_SHORT("Calculate a md5 checksum of a memory area.\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(md5sum)
	.cmd		= do_md5,
	.usage		= "md5 checksum calculation",
	BAREBOX_CMD_HELP(cmd_md5sum_help)
BAREBOX_CMD_END

#endif /* CMD_CMD_MD5SUM */

#ifdef CONFIG_CMD_SHA1SUM

static int do_sha1(int argc, char *argv[])
{
	return do_digest("sha1", argc, argv);
}

BAREBOX_CMD_HELP_START(sha1sum)
BAREBOX_CMD_HELP_USAGE("sha1sum [[FILE] [AREA]]...\n")
BAREBOX_CMD_HELP_SHORT("Calculate a sha1 checksum of a memory area.\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(sha1sum)
	.cmd		= do_sha1,
	.usage		= "sha1 checksum calculation",
	BAREBOX_CMD_HELP(cmd_sha1sum_help)
BAREBOX_CMD_END

#endif /* CMD_CMD_SHA1SUM */

#ifdef CONFIG_CMD_SHA224SUM

static int do_sha224(int argc, char *argv[])
{
	return do_digest("sha224", argc, argv);
}

BAREBOX_CMD_HELP_START(sha224sum)
BAREBOX_CMD_HELP_USAGE("sha224sum [[FILE] [AREA]]...\n")
BAREBOX_CMD_HELP_SHORT("Calculate a sha224 checksum of a memory area.\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(sha224sum)
	.cmd		= do_sha224,
	.usage		= "sha224 checksum calculation",
	BAREBOX_CMD_HELP(cmd_sha224sum_help)
BAREBOX_CMD_END

#endif /* CMD_CMD_SHA224SUM */

#ifdef CONFIG_CMD_SHA256SUM

static int do_sha256(int argc, char *argv[])
{
	return do_digest("sha256", argc, argv);
}

BAREBOX_CMD_HELP_START(sha256sum)
BAREBOX_CMD_HELP_USAGE("sha256sum [[FILE] [AREA]]...\n")
BAREBOX_CMD_HELP_SHORT("Calculate a sha256 checksum of a memory area.\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(sha256sum)
	.cmd		= do_sha256,
	.usage		= "sha256 checksum calculation",
	BAREBOX_CMD_HELP(cmd_sha256sum_help)
BAREBOX_CMD_END

#endif /* CMD_CMD_SHA256SUM */
