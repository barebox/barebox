#include <common.h>
#include <command.h>
#include <getopt.h>
#include <mach/imx-regs.h>
#include <mach/clock.h>

static int do_clko(struct command *cmdtp, int argc, char *argv[])
{
	int opt, div = 0, src = -2, ret;

	while((opt = getopt(argc, argv, "d:s:")) > 0) {
		switch(opt) {
		case 'd':
			div = simple_strtoul(optarg, NULL, 0);
			break;
		case 's':
			src = simple_strtoul(optarg, NULL, 0);
			break;
		}
	}

	if (div == 0 && src == -2)
		return COMMAND_ERROR_USAGE;

	if (src == -1) {
		imx_clko_set_src(-1);
		return 0;
	}

	if (src != -2)
		imx_clko_set_src(src);

	if (div != 0) {
		ret = imx_clko_set_div(div);
		if (ret != div)
			printf("limited divider to %d\n", ret);
	}

	return 0;
}

static __maybe_unused char cmd_clko_help[] =
"Usage: clko [OPTION]...\n"
"Route different signals to the i.MX clko pin\n"
"  -d  <div>	Divider\n"
"  -s  <source> Clock select. See Ref. Manual for valid sources. Use -1\n"
"               for disabling clock output\n";

BAREBOX_CMD_START(clko)
	.cmd		= do_clko,
	.usage		= "Adjust CLKO setting",
	BAREBOX_CMD_HELP(cmd_clko_help)
BAREBOX_CMD_END

