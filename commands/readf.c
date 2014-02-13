#include <common.h>
#include <command.h>
#include <fs.h>
#include <malloc.h>
#include <linux/stat.h>
#include <linux/ctype.h>
#include <environment.h>

static int do_readf(int argc, char *argv[])
{
	unsigned char *buf = NULL, *val;
	char *variable, *filename;
	struct stat s;
	size_t size;
	int ret, i;

	if (argc != 3)
		return COMMAND_ERROR_USAGE;

	filename = argv[1];
	variable = argv[2];

	ret = stat(filename, &s);
	if (ret)
		goto out;

	if (s.st_size > 1024) {
		ret = -EFBIG;
		goto out;
	}

	buf = read_file(filename, &size);
	if (!buf)
		goto out;

	for (i = 0; i < size; i++) {
		if (!isprint(buf[i])) {
			buf[i] = '\0';
			break;
		}
	}

	val = strim(buf);

	ret = setenv(variable, val);
out:
	free(buf);

	return ret;
}

BAREBOX_CMD_HELP_START(readf)
BAREBOX_CMD_HELP_USAGE("readf <file> <variable>\n")
BAREBOX_CMD_HELP_SHORT("Read a single line of a file into a shell variable. Leading and trailing whitespaces\n")
BAREBOX_CMD_HELP_SHORT("are removed, nonvisible characters are stripped. Input is limited to 1024\n")
BAREBOX_CMD_HELP_SHORT("characters.\n")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(readf)
	.cmd		= do_readf,
	.usage		= "read file into variable",
	BAREBOX_CMD_HELP(cmd_readf_help)
BAREBOX_CMD_END
