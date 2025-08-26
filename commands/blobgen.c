// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <command.h>
#include <getopt.h>
#include <blobgen.h>
#include <environment.h>

static int do_blobgen(int argc, char *argv[])
{
	bool do_encrypt = false, do_decrypt = false;
	int opt;
	const char *varname = NULL;
	const char *modifier = NULL;
	const char *blobdev = NULL;
	struct blobgen *bg;
	int plainsize;
	int ret;
	const char *message = NULL;

	while ((opt = getopt(argc, argv, "edm:V:b:")) > 0) {
		switch (opt) {
		case 'e':
			do_encrypt = true;
			break;
		case 'd':
			do_decrypt = true;
			break;
		case 'm':
			modifier = optarg;
			break;
		case 'V':
			varname = optarg;
			break;
		case 'b':
			blobdev = optarg;
			break;
		}
	}

	if (!varname) {
		printf("varname not specified\n");
		return -EINVAL;
	}

	if (!modifier) {
		printf("Modifier not specified\n");
		return -EINVAL;
	}

	bg = blobgen_get(blobdev);
	if (!bg) {
		printf("blobdev \"%s\" not found\n", blobdev);
		return -ENOENT;
	}

	if (do_encrypt && do_decrypt) {
		printf("Both encrypt and decrypt given\n");
		return -EINVAL;
	}

	if (!do_encrypt && !do_decrypt) {
		printf("Specify either -e or -d option\n");
		return -EINVAL;
	}

	if (argc > optind) {
		message = argv[optind];
	} else {
		printf("No message to %scrypt provided\n",
		       do_encrypt ? "en" : "de");
		return -EINVAL;
	}

	if (do_encrypt) {
		ret = blob_encrypt_to_env(bg, modifier, message, strlen(message),
					  varname);
		if (ret)
			return ret;
	}

	if (do_decrypt) {
		void *plain;
		char *str;

		ret = blob_decrypt_from_base64(bg, modifier, message, &plain,
					    &plainsize);
		if (ret)
			return ret;

		str = malloc(plainsize + 1);
		if (!str)
			return -ENOMEM;

		memcpy(str, plain, plainsize);
		str[plainsize] = 0;

		setenv(varname, str);
		free(plain);
		free(str);
	}

	return 0;
}

BAREBOX_CMD_HELP_START(blobgen)
BAREBOX_CMD_HELP_TEXT("This command utilizes hardware crypto engines to en/decrypt")
BAREBOX_CMD_HELP_TEXT("data blobs.")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-e\t", "encrypt")
BAREBOX_CMD_HELP_OPT("-d\t", "decrypt")
BAREBOX_CMD_HELP_OPT("-m <modifier>", "Set modifier")
BAREBOX_CMD_HELP_OPT("-V <varname>", "specify variable name to set with the result")
BAREBOX_CMD_HELP_OPT("-b <blobdev>", "specify blob device to use")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(blobgen)
	.cmd	= do_blobgen,
	BAREBOX_CMD_DESC("en/decrypt blobs")
	BAREBOX_CMD_OPTS("[-edmVb] <plaintext/ciphertext>")
	BAREBOX_CMD_GROUP(CMD_GRP_SECURITY)
	BAREBOX_CMD_HELP(cmd_blobgen_help)
BAREBOX_CMD_END
