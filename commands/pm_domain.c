// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <command.h>
#include <pm_domain.h>

static int do_pm_domain(int argc, char *argv[])
{
	pm_genpd_print();

	return 0;
}

BAREBOX_CMD_START(pm_domain)
	.cmd		= do_pm_domain,
	BAREBOX_CMD_DESC("list power domains")
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
BAREBOX_CMD_END
