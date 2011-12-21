#include <common.h>
#include <command.h>
#include <module.h>
#include <errno.h>
#include <fs.h>
#include <malloc.h>

static int do_insmod(int argc, char *argv[])
{
	struct module *module;
	void *buf;
	int len;

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	buf = read_file(argv[1], &len);
	if (!buf) {
		perror("insmod");
		return 1;
	}

	module = load_module(buf, len);

	free(buf);

	if (module) {
		if (module->init)
			module->init();
	}

	return 0;
}

static const __maybe_unused char cmd_insmod_help[] =
"Usage: insmod <module>\n"; 

BAREBOX_CMD_START(insmod)
	.cmd		= do_insmod,
	.usage		= "insert a module",
	BAREBOX_CMD_HELP(cmd_insmod_help)
BAREBOX_CMD_END
