#include <common.h>
#include <command.h>
#include <getopt.h>
#include <linux/clk.h>
#include <linux/err.h>

static int do_clk_enable(int argc, char *argv[])
{
	struct clk *clk;

	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	clk = clk_lookup(argv[1]);
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	return clk_enable(clk);
}

BAREBOX_CMD_HELP_START(clk_enable)
BAREBOX_CMD_HELP_USAGE("clk_enable <clk>\n")
BAREBOX_CMD_HELP_SHORT("enable <clk>\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(clk_enable)
	.cmd		= do_clk_enable,
	.usage		= "enable a clock",
	BAREBOX_CMD_HELP(cmd_clk_enable_help)
BAREBOX_CMD_END

static int do_clk_disable(int argc, char *argv[])
{
	struct clk *clk;

	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	clk = clk_lookup(argv[1]);
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	clk_disable(clk);

	return 0;
}

BAREBOX_CMD_HELP_START(clk_disable)
BAREBOX_CMD_HELP_USAGE("clk_disable <clk>\n")
BAREBOX_CMD_HELP_SHORT("disable <clk>\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(clk_disable)
	.cmd		= do_clk_disable,
	.usage		= "disable a clock",
	BAREBOX_CMD_HELP(cmd_clk_disable_help)
BAREBOX_CMD_END

static int do_clk_set_rate(int argc, char *argv[])
{
	struct clk *clk;
	unsigned long rate;

	if (argc != 3)
		return COMMAND_ERROR_USAGE;

	clk = clk_lookup(argv[1]);
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	rate = simple_strtoul(argv[2], NULL, 0);

	return clk_set_rate(clk, rate);
}

BAREBOX_CMD_HELP_START(clk_set_rate)
BAREBOX_CMD_HELP_USAGE("clk_set_rate <clk> <rate_hz>\n")
BAREBOX_CMD_HELP_SHORT("Set clock <clk> to <rate>\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(clk_set_rate)
	.cmd		= do_clk_set_rate,
	.usage		= "set a clocks rate",
	BAREBOX_CMD_HELP(cmd_clk_set_rate_help)
BAREBOX_CMD_END

static int do_clk_dump(int argc, char *argv[])
{
	int opt, verbose = 0;

	while ((opt = getopt(argc, argv, "v")) > 0) {
		switch(opt) {
		case 'v':
			verbose = 1;
			break;
		default:
			return -EINVAL;

		}
	}

	clk_dump(verbose);

	return 0;
}

BAREBOX_CMD_HELP_START(clk_dump)
BAREBOX_CMD_HELP_USAGE("clk_dump [-v]\n")
BAREBOX_CMD_HELP_OPT  ("-v",  "verbose\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(clk_dump)
	.cmd		= do_clk_dump,
	.usage		= "show information about registered clocks",
	BAREBOX_CMD_HELP(cmd_clk_dump_help)
BAREBOX_CMD_END

static int do_clk_set_parent(int argc, char *argv[])
{
	struct clk *clk, *parent;

	if (argc != 3)
		return COMMAND_ERROR_USAGE;

	clk = clk_lookup(argv[1]);
	if (IS_ERR(clk))
		return PTR_ERR(clk);
	parent = clk_lookup(argv[2]);
	if (IS_ERR(parent))
		return PTR_ERR(parent);

	return clk_set_parent(clk, parent);
}

BAREBOX_CMD_HELP_START(clk_set_parent)
BAREBOX_CMD_HELP_USAGE("clk_set_parent <clk> <parent>\n")
BAREBOX_CMD_HELP_SHORT("Set parent of <clk> to <parent>\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(clk_set_parent)
	.cmd		= do_clk_set_parent,
	.usage		= "set a parent of a clock",
	BAREBOX_CMD_HELP(cmd_clk_set_parent_help)
BAREBOX_CMD_END
