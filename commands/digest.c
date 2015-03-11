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

static int do_digest(char *algorithm, int argc, char *argv[])
{
	struct digest *d;
	int ret = 0;
	int i;
	unsigned char *hash;
	unsigned char *key = NULL;
	size_t keylen = 0;
	int opt;

	while((opt = getopt(argc, argv, "h:")) > 0) {
		switch(opt) {
		case 'h':
			key = optarg;
			keylen = strlen(key);
			break;
		}
	}

	argc -= optind;
	argv += optind;

	if (key) {
		char *tmp = asprintf("hmac(%s)", algorithm);
		d = digest_alloc(tmp);
		free(tmp);
	} else {
		d = digest_alloc(algorithm);
	}
	BUG_ON(!d);

	if (argc < 1)
		return COMMAND_ERROR_USAGE;

	hash = calloc(digest_length(d), sizeof(unsigned char));
	if (!hash) {
		perror("calloc");
		return COMMAND_ERROR_USAGE;
	}

	while (*argv) {
		char *filename = "/dev/mem";
		loff_t start = 0, size = ~0;

		/* arguments are either file, file+area or area */
		if (parse_area_spec(*argv, &start, &size)) {
			filename = *argv;
			if (argv[0] && !parse_area_spec(argv[0], &start, &size))
				argv++;
		}

		ret = digest_file_window(d, filename,
					 key, keylen,
					 hash, start, size);
		if (ret < 0) {
			ret = 1;
		} else {
			for (i = 0; i < digest_length(d); i++)
				printf("%02x", hash[i]);

			printf("  %s\t0x%08llx ... 0x%08llx\n",
				filename, start, start + size);
		}

		argv++;
	}

	free(hash);
	digest_free(d);

	return ret;
}

#ifdef CONFIG_CMD_MD5SUM

static int do_md5(int argc, char *argv[])
{
	return do_digest("md5", argc, argv);
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
	return do_digest("sha1", argc, argv);
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
	return do_digest("sha224", argc, argv);
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
	return do_digest("sha256", argc, argv);
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
	return do_digest("sha384", argc, argv);
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
	return do_digest("sha512", argc, argv);
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
