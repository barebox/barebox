#include <command.h>
#include <stdio.h>
#include <crypto/public_key.h>

static int do_keys(int argc, char *argv[])
{
	const struct public_key *key;
	int id;

	for_each_public_key(key, id) {
		printf("KEY: %*phN\tTYPE: %s\tKEYRING: %s\tHINT: %s\n", key->hashlen,
		       key->hash, public_key_type_string(key->type), key->keyring, key->key_name_hint);
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
