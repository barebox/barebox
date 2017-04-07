#include <common.h>
#include <command.h>
#include <getopt.h>
#include <libfile.h>
#include <crypto/keystore.h>
#include <linux/kernel.h>
#include <fs.h>

static int do_keystore(int argc, char *argv[])
{
	int opt;
	int ret;
	int do_remove = 0;
	const char *name;
	const char *file = NULL;
	char *secret_str = NULL;
	void *secret;
	int s_len;

	while ((opt = getopt(argc, argv, "rs:f:")) > 0) {
		switch (opt) {
		case 'r':
			do_remove = 1;
			break;
		case 's':
			secret_str = optarg;
			break;
		case 'f':
			file = optarg;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	if (argc == optind)
		return COMMAND_ERROR_USAGE;

	if (!do_remove && !file && !secret_str)
		return COMMAND_ERROR_USAGE;

	if (file && secret_str)
		return COMMAND_ERROR_USAGE;

	name = argv[optind];

	if (do_remove) {
		keystore_forget_secret(name);
		printf("forgotten secret for key %s\n", name);
		return 0;
	}

	if (file) {
		ret = read_file_2(file, &s_len, (void *)&secret_str, FILESIZE_MAX);
		if (ret) {
			printf("Cannot open %s: %s\n", file, strerror(-ret));
			return 1;
		}
	} else if (secret_str) {
		s_len = strlen(secret_str);
	}

	if (s_len & 1) {
		printf("invalid secret len. Must be whole bytes\n");
		return 1;
	}

	secret = xzalloc(s_len / 2);
	ret = hex2bin(secret, secret_str, s_len / 2);
	if (ret) {
		printf("Cannot convert %s to binary: %s\n", secret_str, strerror(-ret));
		return 1;
	}

	ret = keystore_set_secret(name, secret, s_len / 2);
	if (ret)
		printf("cannot set secret for key %s: %s\n", name, strerror(-ret));
	else
		printf("Added secret for key %s\n", name);

	free(secret);

	return ret ? 1 : 0;
}

BAREBOX_CMD_HELP_START(keystore)
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-r", "remove a key from the keystore")
BAREBOX_CMD_HELP_OPT("-s <key>", "set a key in the keystore")
BAREBOX_CMD_HELP_OPT("-f <keyfile>", "set a key in the keystore, read secret from file")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(keystore)
	.cmd	= do_keystore,
	BAREBOX_CMD_DESC("manage keys")
	BAREBOX_CMD_OPTS("[-rsf] <keyname>")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_keystore_help)
BAREBOX_CMD_END
