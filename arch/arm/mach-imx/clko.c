#include <common.h>
#include <command.h>
#include <getopt.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/clock.h>

static int do_clko (cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	int opt, div = 0, src = -2, ret;

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

U_BOOT_CMD_START(clko)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_clko,
	.usage		= "Adjust CLKO setting",
	U_BOOT_CMD_HELP(cmd_clko_help)
U_BOOT_CMD_END


static int do_mcpy_test (cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	unsigned long src, dest, count, end;

	if (argc < 4)
		return 1;

	dest = simple_strtoul(argv[1], NULL, 0);
	src = simple_strtoul(argv[2], NULL, 0);
	count = simple_strtoul(argv[3], NULL, 0);

	printf("copying from 0x%08x to 0x%08x (size %d)\n", src, dest, count);

	end = src + count;

	__asm__ __volatile__(
		"copy_loop:\n"
		"ldmia	%1!, {r3-r10}\n"
		"stmia	%0!, {r3-r10}\n"
		"cmp	%1, %2\n"
		"ble	copy_loop\n"
		:
		: "r" (dest), "r" (src), "r" (end)
		: "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10");

	return 0;
}

U_BOOT_CMD_START(mcpy_test)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_mcpy_test,
	.usage		= "Adjust CLKO setting",
U_BOOT_CMD_END

