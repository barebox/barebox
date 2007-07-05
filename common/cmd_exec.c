#include <common.h>
#include <command.h>
#include <fs.h>
#include <fcntl.h>
#include <linux/stat.h>
#include <errno.h>
#include <malloc.h>
#include <xfuncs.h>

#ifdef CONFIG_HUSH_PARSER
#include <hush.h>
#endif

static int do_exec(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	int i;
	char *script;

	if (argc < 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	for (i=1; i<argc; ++i) {
		script = read_file(argv[i]);
		if (!script)
			return 1;

		if (run_command (script, flag) == -1)
			goto out;
		free(script);
	}
	return 0;

out:
	free(script);
	return 1;
}

U_BOOT_CMD_START(exec)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_exec,
	.usage		= "execute a script",
U_BOOT_CMD_END
