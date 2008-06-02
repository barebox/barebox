#include <common.h>
#include <command.h>
#include <getopt.h>
#include <asm/arch/imx-regs.h>

static int do_clko (cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	int opt, div = 0, src = -2;

	getopt_reset();

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

	if (div == 0 && src == -2) {
		u_boot_cmd_usage(cmdtp);
		return 0;
	}

	if (src == -1) {
		PCDR0 &= ~(1 << 25);
		return 0;
	}

	if (src != -2) {
		ulong ccsr;
		ccsr = CCSR & ~0x1f;
		ccsr |= src & 0x1f;
		CCSR = ccsr;
	}

	if (div != 0) {
		ulong pcdr;
		div--;
		pcdr = PCDR0 & ~(7 << 22);
		pcdr |= (div & 0x7) << 22;
		PCDR0 = pcdr;
	}

	PCDR0 |= (1 << 25);
	return 0;
}

static __maybe_unused char cmd_clko_help[] =
"Usage: clko [OPTION]...\n"
"Route different signals to the i.MX clko pin\n"
"  -d  <div>	Divider (1..8)\n"
"  -s  <source> Clock select. See Ref. Manual for valid sources. Use -1\n"
"               for disabling clock output\n";

U_BOOT_CMD_START(clko)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_clko,
	.usage		= "Adjust CLKO setting",
	U_BOOT_CMD_HELP(cmd_clko_help)
U_BOOT_CMD_END

