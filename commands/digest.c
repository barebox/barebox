/*
 * Copyright (c) 2015 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * GPLv2 ONLY
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
#include <libfile.h>

#include "internal.h"

int __do_digest(struct digest *d, unsigned char *sig,
		       int argc, char *argv[])
{
	int ret = COMMAND_ERROR_USAGE;
	int i;
	unsigned char *hash;

	if (argc < 1)
		goto err;

	hash = calloc(digest_length(d), sizeof(unsigned char));
	if (!hash) {
		perror("calloc");
		goto err;
	}

	while (*argv) {
		char *filename = "/dev/mem";
		loff_t start = 0, size = ~0;
		int show_area = 1;

		/* arguments are either file, file+area or area */
		if (parse_area_spec(*argv, &start, &size)) {
			filename = *argv;
			show_area = 0;
			if (argv[1] && !parse_area_spec(argv[1], &start, &size)) {
				argv++;
				show_area = 1;
			}
		}

		ret = digest_file_window(d, filename,
					 hash, sig, start, size);
		if (ret < 0) {
			ret = 1;
		} else {
			if (!sig) {
				for (i = 0; i < digest_length(d); i++)
					printf("%02x", hash[i]);

				printf("  %s", filename);
				if (show_area)
					printf("\t0x%08llx ... 0x%08llx",
						start, start + size);

				puts("\n");
			}
		}

		argv++;
	}

	free(hash);
err:
	digest_free(d);

	return ret;
}

static void __maybe_unused prints_algo_help(void)
{
	puts("\navailable algo:\n");
	digest_algo_prints("\t");
}

static int do_digest(int argc, char *argv[])
{
	struct digest *d;
	unsigned char *tmp_key = NULL;
	unsigned char *tmp_sig = NULL;
	char *sig = NULL;
	char *sigfile = NULL;
	size_t siglen = 0;
	char *key = NULL;
	char *keyfile = NULL;
	size_t keylen = 0;
	size_t digestlen = 0;
	char *algo = NULL;
	int opt;
	int ret = COMMAND_ERROR;

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	while((opt = getopt(argc, argv, "a:k:K:s:S:")) > 0) {
		switch(opt) {
		case 'k':
			key = optarg;
			keylen = strlen(key);
			break;
		case 'K':
			keyfile = optarg;
			break;
		case 'a':
			algo = optarg;
			break;
		case 's':
			sig = optarg;
			siglen = strlen(sig);
			break;
		case 'S':
			sigfile = optarg;
			break;
		}
	}

	if (!algo)
		return COMMAND_ERROR_USAGE;

	d = digest_alloc(algo);
	if (!d) {
		eprintf("algo '%s' not found\n", algo);
		return COMMAND_ERROR_USAGE;
	}

	argc -= optind;
	argv += optind;

	if (keyfile) {
		tmp_key = key = read_file(keyfile, &keylen);
		if (!key) {
			eprintf("file '%s' not found\n", keyfile);
			goto err;
		}
	}

	if (key) {
		ret = digest_set_key(d, key, keylen);
		free(tmp_key);
		if (ret)
			goto err;
	} else if (digest_is_flags(d, DIGEST_ALGO_NEED_KEY)) {
		eprintf("%s need a key to be used\n", digest_name(d));
		goto err;
	}

	if (sigfile) {
		sig = tmp_sig = read_file(sigfile, &siglen);
		if (!tmp_sig) {
			eprintf("file '%s' not found\n", sigfile);
			goto err;
		}
	}

	if (sig) {
		digestlen = digest_length(d);
		if (siglen == 2 * digestlen) {
			if (!tmp_sig)
				tmp_sig = xmalloc(digestlen);

			ret = hex2bin(tmp_sig, sig, digestlen);
			if (ret)
				goto err;

			sig = tmp_sig;
		} else if (siglen != digestlen) {
			eprintf("%s wrong size %zu, expected %zu\n",
				sigfile, siglen, digestlen);
			ret = COMMAND_ERROR;
			goto err;
		}
	}

	ret = __do_digest(d, sig, argc, argv);
	free(tmp_sig);
	return ret;

err:
	digest_free(d);
	return ret;
}

BAREBOX_CMD_HELP_START(digest)
BAREBOX_CMD_HELP_TEXT("Calculate a digest over a FILE or a memory area.")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-a <algo>\t",  "hash or signature algorithm to use")
BAREBOX_CMD_HELP_OPT ("-k <key>\t",   "use supplied <key> (ASCII or hex) for MAC")
BAREBOX_CMD_HELP_OPT ("-K <file>\t",  "use key from <file> (binary) for MAC")
BAREBOX_CMD_HELP_OPT ("-s <hex>\t",   "verify data against supplied <hex> (hash, MAC or signature)")
BAREBOX_CMD_HELP_OPT ("-S <file>\t",  "verify data against <file> (hash, MAC or signature)")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(digest)
	.cmd		= do_digest,
	BAREBOX_CMD_DESC("calculate digest")
	BAREBOX_CMD_OPTS("-a <algo> [-k <key> | -K <file>] [-s <sig> | -S <file>] FILE|AREA")
	BAREBOX_CMD_GROUP(CMD_GRP_FILE)
	BAREBOX_CMD_HELP(cmd_digest_help)
	BAREBOX_CMD_USAGE(prints_algo_help)
BAREBOX_CMD_END
