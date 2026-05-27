#include <command.h>
#include <stdio.h>
#include <crypto/public_key.h>

static int do_keys(int argc, char *argv[])
{
	const struct keyring *kr;
	const struct keyring_link *link;

	for_each_keyring(kr) {
		printf("RING: %s\n", kr->name);
		for_each_link_in_keyring(link, kr) {
			if (link->type == KEYRING_LINK_KEY) {
				const struct public_key *key = link->key;

				printf("    KEY:    %*phN\tTYPE: %s\tHINT: %s\n",
				       key->hashlen, key->hash,
				       public_key_type_string(key->type),
				       key->key_name_hint ?: "");
			} else {
				printf("    RING:   %s\n", link->keyring->name);
			}
		}
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
