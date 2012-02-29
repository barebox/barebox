#include <common.h>
#include <command.h>
#include <linux/stat.h>
#include <malloc.h>
#include <fs.h>

#define MAGIC		0x19691228

static int do_alternate(int argc, char *argv[])
{
	void *buf;
	size_t size;
	ulong *ptr, val = 0, bitcount = 0;

	if (argc != 2)
		return COMMAND_ERROR_USAGE;

	buf = read_file(argv[1], &size);
	if (!buf)
		return 1;

	ptr = buf;
	if ((*ptr) != MAGIC) {
		printf("Wrong magic! Expected 0x%08x, got 0x%08lx.\n", MAGIC, *ptr);
		return 1;
	}

	ptr++;

	while ((ulong)ptr <= (ulong)buf + size && !(val = *ptr++))
		bitcount += 32;

	if (val) {
		do {
			if (val & 1)
				break;
			bitcount++;
		} while (val >>= 1);
	}

	printf("Bitcount : %ld\n", bitcount);

	free(buf);
	return (bitcount & 1) ? 3 : 2;
}

static const __maybe_unused char cmd_alternate_help[] =
"Usage: alternate <file>"
"\n";

BAREBOX_CMD_START(alternate)
	.cmd		= do_alternate,
	.usage		= "count zero bits in a file",
	BAREBOX_CMD_HELP(cmd_alternate_help)
BAREBOX_CMD_END

