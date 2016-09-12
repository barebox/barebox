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
#include <getopt.h>

#include "internal.h"

static int do_hash(char *algo, int argc, char *argv[])
{
	struct digest *d;
	unsigned char *key = NULL;
	size_t keylen = 0;
	int opt, ret;

	while ((opt = getopt(argc, argv, "h:")) > 0) {
		switch(opt) {
		case 'h':
			key = optarg;
			keylen = strlen(key);
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (key) {
		char *tmp = basprintf("hmac(%s)", algo);
		d = digest_alloc(tmp);
		free(tmp);
		BUG_ON(!d);

		ret = digest_set_key(d, key, keylen);
		if (ret) {
			perror("set_key");
			return ret;
		}
	} else {
		d = digest_alloc(algo);
		BUG_ON(!d);
	}

	argc -= optind;
	argv += optind;

	return __do_digest(d, NULL, argc, argv);
}

#ifdef CONFIG_CMD_MD5SUM

static int do_md5(int argc, char *argv[])
{
	return do_hash("md5", argc, argv);
}

BAREBOX_CMD_HELP_START(md5sum)
BAREBOX_CMD_HELP_TEXT("Calculate a MD5 digest over a FILE or a memory area.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(md5sum)
	.cmd		= do_md5,
	BAREBOX_CMD_DESC("calculate MD5 checksum")
	BAREBOX_CMD_OPTS("FILE|AREA...")
	BAREBOX_CMD_GROUP(CMD_GRP_FILE)
	BAREBOX_CMD_HELP(cmd_md5sum_help)
BAREBOX_CMD_END

#endif /* CMD_CMD_MD5SUM */

#ifdef CONFIG_CMD_SHA1SUM

static int do_sha1(int argc, char *argv[])
{
	return do_hash("sha1", argc, argv);
}

BAREBOX_CMD_HELP_START(sha1sum)
BAREBOX_CMD_HELP_TEXT("Calculate a SHA1 digest over a FILE or a memory area.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(sha1sum)
	.cmd		= do_sha1,
	BAREBOX_CMD_DESC("calculate SHA1 digest")
	BAREBOX_CMD_OPTS("FILE|AREA")
	BAREBOX_CMD_GROUP(CMD_GRP_FILE)
	BAREBOX_CMD_HELP(cmd_sha1sum_help)
BAREBOX_CMD_END

#endif /* CMD_CMD_SHA1SUM */

#ifdef CONFIG_CMD_SHA224SUM

static int do_sha224(int argc, char *argv[])
{
	return do_hash("sha224", argc, argv);
}

BAREBOX_CMD_HELP_START(sha224sum)
BAREBOX_CMD_HELP_TEXT("Calculate a SHA224 digest over a FILE or a memory area.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(sha224sum)
	.cmd		= do_sha224,
	BAREBOX_CMD_DESC("calculate SHA224 digest")
	BAREBOX_CMD_OPTS("FILE|AREA")
	BAREBOX_CMD_GROUP(CMD_GRP_FILE)
	BAREBOX_CMD_HELP(cmd_sha224sum_help)
BAREBOX_CMD_END

#endif /* CMD_CMD_SHA224SUM */

#ifdef CONFIG_CMD_SHA256SUM

static int do_sha256(int argc, char *argv[])
{
	return do_hash("sha256", argc, argv);
}

BAREBOX_CMD_HELP_START(sha256sum)
BAREBOX_CMD_HELP_TEXT("Calculate a SHA256 digest over a FILE or a memory area.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(sha256sum)
	.cmd		= do_sha256,
	BAREBOX_CMD_DESC("calculate SHA256 digest")
	BAREBOX_CMD_OPTS("FILE|AREA")
	BAREBOX_CMD_GROUP(CMD_GRP_FILE)
	BAREBOX_CMD_HELP(cmd_sha256sum_help)
BAREBOX_CMD_END

#endif /* CMD_CMD_SHA256SUM */

#ifdef CONFIG_CMD_SHA384SUM

static int do_sha384(int argc, char *argv[])
{
	return do_hash("sha384", argc, argv);
}

BAREBOX_CMD_HELP_START(sha384sum)
BAREBOX_CMD_HELP_TEXT("Calculate a SHA384 digest over a FILE or a memory area.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(sha384sum)
	.cmd		= do_sha384,
	BAREBOX_CMD_DESC("calculate SHA384 digest")
	BAREBOX_CMD_OPTS("FILE|AREA")
	BAREBOX_CMD_GROUP(CMD_GRP_FILE)
	BAREBOX_CMD_HELP(cmd_sha384sum_help)
BAREBOX_CMD_END

#endif /* CMD_CMD_SHA384SUM */

#ifdef CONFIG_CMD_SHA512SUM

static int do_sha512(int argc, char *argv[])
{
	return do_hash("sha512", argc, argv);
}

BAREBOX_CMD_HELP_START(sha512sum)
BAREBOX_CMD_HELP_TEXT("Calculate a SHA512 digest over a FILE or a memory area.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(sha512sum)
	.cmd		= do_sha512,
	BAREBOX_CMD_DESC("calculate SHA512 digest")
	BAREBOX_CMD_OPTS("FILE|AREA")
	BAREBOX_CMD_GROUP(CMD_GRP_FILE)
	BAREBOX_CMD_HELP(cmd_sha512sum_help)
BAREBOX_CMD_END

#endif /* CMD_CMD_SHA512SUM */
