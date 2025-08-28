#include <command.h>
#include <stdio.h>
#include <crypto/public_key.h>

static int do_keys(int argc, char *argv[])
{
	const struct public_key *key;

	for_each_public_key(key) {
		printf("KEY: %*phN", key->hashlen, key->hash);

		if (key->key_name_hint)
			printf(" (%s)\n", key->key_name_hint);
		else
			printf("\n");
	}

	return 0;
}

BAREBOX_CMD_HELP_START(keys)
BAREBOX_CMD_HELP_TEXT("Print informations about public keys")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(keys)
        .cmd            = do_keys,
        BAREBOX_CMD_DESC("Print informations about public keys")
        BAREBOX_CMD_GROUP(CMD_GRP_CONSOLE)
        BAREBOX_CMD_HELP(cmd_keys_help)
BAREBOX_CMD_END
