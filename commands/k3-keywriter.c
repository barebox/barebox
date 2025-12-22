// SPDX-License-Identifier: GPL-2.0-only

#include <stdio.h>
#include <malloc.h>
#include <command.h>
#include <getopt.h>
#include <libfile.h>
#include <linux/kstrtox.h>
#include <soc/k3/keywriter-lite.h>

static void *read_sha512(const char *filename)
{
	size_t size;
	int ret;
	void *hash;

	ret = read_file_2(filename, &size, &hash, SHA512_DIGEST_SIZE + 1);
	if (ret) {
		printf("Cannot read %s: %pe\n", filename, ERR_PTR(ret));
		return NULL;
	}

	if (size != SHA512_DIGEST_SIZE) {
		printf("unexpected file size in hash file. Should be %u\n",
		       SHA512_DIGEST_SIZE);
		free(hash);
		return NULL;
	}

	return hash;
}

static int do_k3_keywriter(int argc, char *argv[])
{
	struct keywriter_lite_packet p;
	struct keywriter_lite_blob *blob;
	int opt, ret;
	const char *smpkh_file = NULL;
	const char *bmpkh_file = NULL;
	const char *outfile = NULL;
	int key_count = -1;
	int key_rev = -1;
	bool doit = false;

	blob = &p.blob;

	while ((opt = getopt(argc, argv, "s:b:o:c:r:x")) > 0) {
		switch (opt) {
		case 's':
			smpkh_file = optarg;
			break;
		case 'b':
			bmpkh_file = optarg;
			break;
		case 'o':
			outfile = optarg;
			break;
		case 'c':
			ret = kstrtouint(optarg, 0, &key_count);
			if (ret)
				goto err;
			break;
		case 'r':
			ret = kstrtouint(optarg, 0, &key_rev);
			if (ret)
				goto err;
			break;
		case 'x':
			doit = true;
			break;
		}
	}

	k3_kwl_prepare(&p);

	if (smpkh_file) {
		unsigned char *hash = read_sha512(smpkh_file);

		if (!hash) {
			ret = COMMAND_ERROR;
			goto err;
		}

		k3_kwl_set_smpkh(&blob->payload.smpkh, hash, FLAG_ACTIVE);
		free(hash);
	}

	if (bmpkh_file) {
		unsigned char *hash = read_sha512(bmpkh_file);

		if (!hash) {
			ret = COMMAND_ERROR;
			goto err;
		}

		k3_kwl_set_bmpkh(&blob->payload.bmpkh, hash, FLAG_ACTIVE);
		free(hash);
	}

	if (key_count >= 0) {
		ret = k3_kwl_set_keycnt(&blob->payload.key_count, key_count,
					FLAG_ACTIVE);
		if (ret)
			goto err;
	}

	if (key_rev >= 0) {
		ret = k3_kwl_set_key_revision(&blob->payload.key_revision, key_rev,
					      FLAG_ACTIVE);
		if (ret)
			goto err;
	}

	ret = k3_kwl_finalize(&p);
	if (ret) {
		ret = COMMAND_ERROR;
		printf("Cannot finalize keywriter lite structure: %pe\n", ERR_PTR(ret));
		goto err;
	}

	if (outfile) {
		ret = write_file(outfile, &p, sizeof(p));
		if (ret) {
			ret = COMMAND_ERROR;
			printf("Failed to write %s: %pe\n", outfile, ERR_PTR(ret));
			goto err;
		}
	}

	if (!smpkh_file && !bmpkh_file && key_count < 0 && key_rev < 0) {
		printf("Nothing to do\n");
		ret = COMMAND_ERROR_USAGE;
	}

	if (doit) {
		printf("Writing fuses...\n");
		ret = k3_kwl_fuse_writebuff(&p);
		if (ret) {
			printf("Failed to write fuses: %pe\n", ERR_PTR(ret));
			goto err;
		} else {
			printf("Fuses written successfully\n");
		}
	} else {
		printf("Not writing fuses as requested. "
		       "Pass -x to actually write fuses\n");
		ret = 0;
	}

err:
	return ret;
}

BAREBOX_CMD_HELP_START(k3_keywriter)
BAREBOX_CMD_HELP_TEXT("K3 keywriter support")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-s <FILE>", "Read SMPK hash from <FILE>")
BAREBOX_CMD_HELP_OPT ("-b <FILE>", "Read BMPK hash from <FILE>")
BAREBOX_CMD_HELP_OPT ("-o <FILE>", "write data structure for ROM to <FILE> (for debug)")
BAREBOX_CMD_HELP_OPT ("-c <NKEYS>","set key count to <NKEYS>")
BAREBOX_CMD_HELP_OPT ("-r <REV>",  "set key revision to <REV>")
BAREBOX_CMD_HELP_OPT ("-x",        "Do it! actually write fuses")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(k3_keywriter)
	.cmd = do_k3_keywriter,
	BAREBOX_CMD_DESC("K3 keywriter support")
	BAREBOX_CMD_OPTS("-sbocrx")
	BAREBOX_CMD_GROUP(CMD_GRP_HWMANIP)
	BAREBOX_CMD_HELP(cmd_k3_keywriter_help)
BAREBOX_CMD_END
