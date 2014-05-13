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
BAREBOX_CMD_HELP_TEXT("Read a single line from FILE into a VARiable. Leading and trailing")
BAREBOX_CMD_HELP_TEXT("whitespaces are removed, nonvisible characters are stripped. Input is")
BAREBOX_CMD_HELP_TEXT("limited to 1024 characters.")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(readf)
	.cmd		= do_readf,
	BAREBOX_CMD_DESC("read file into variable")
	BAREBOX_CMD_OPTS("FILE VAR")
	BAREBOX_CMD_GROUP(CMD_GRP_SCRIPT)
	BAREBOX_CMD_HELP(cmd_readf_help)
BAREBOX_CMD_END
