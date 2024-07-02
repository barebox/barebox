// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <command.h>
#include <net.h>

static int do_host(int argc, char *argv[])
{
	IPaddr_t ip;
	int ret;
	char *hostname, *varname = NULL;

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	hostname = argv[1];

	if (argc > 2)
		varname = argv[2];

	ret = resolv(argv[1], &ip);
	if (ret) {
		printf("unknown host %s\n", hostname);
		return 1;
	}

	if (varname)
		setenv_ip(varname, ip);
	else
		printf("%s is at %pI4\n", hostname, &ip);

	return 0;
}

BAREBOX_CMD_START(host)
	.cmd		= do_host,
	BAREBOX_CMD_DESC("resolve a hostname")
	BAREBOX_CMD_OPTS("<HOSTNAME> [VARIABLE]")
	BAREBOX_CMD_GROUP(CMD_GRP_NET)
BAREBOX_CMD_END
