#include <common.h>
#include <command.h>
#include <linux/stat.h>
#include <malloc.h>
#include <fs.h>

static int do_alternate (cmd_tbl_t *cmdtp, int argc, char *argv[])
{
	void *buf;
	size_t size;
	ulong *ptr, val = 0, bitcount = 0;

	if (argc != 2) {
		u_boot_cmd_usage(cmdtp);
		return 1;
	}

	buf = read_file(argv[1], &size);
	if (!buf)
		return 1;

	ptr = buf;

	while ((ulong)ptr <= (ulong)buf + size && !(val = *ptr++))
		bitcount += 32;

	if (val) {
		do {
			if (val & 1)
				break;
			bitcount++;
		} while (val >>= 1);
	}

	printf("bitcount : %d\n", bitcount);

	free(buf);
	return bitcount & 1 ? 2 : 3;
}

static __maybe_unused char cmd_alternate_help[] =
"Usage: alternate <file>\n"
"\n";

U_BOOT_CMD_START(alternate)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_alternate,
	.usage		= "count zero bits in a file",
	U_BOOT_CMD_HELP(cmd_alternate_help)
U_BOOT_CMD_END

