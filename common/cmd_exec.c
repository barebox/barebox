#include <common.h>
#include <command.h>
#include <fs.h>
#include <fcntl.h>
#include <linux/stat.h>
#include <errno.h>
#include <malloc.h>

#ifdef CONFIG_HUSH_PARSER
#include <hush.h>
#endif

static void *read_file(const char *file)
{
	struct stat s;
	void *buf = NULL;
	int fd = 0;

	if (stat(file, &s)) {
		perror("stat");
		return NULL;
	}

	fd = open(file, O_RDONLY);
	if (fd < 0) {
		perror("open");
		return NULL;
	}

	buf = malloc(s.st_size + 1);

	if (read(fd, buf, s.st_size) < s.st_size) {
		perror("read");
		goto out;
	}

	*(char *)(buf + s.st_size) = 0;
	close(fd);
	return buf;

out:
	free(buf);
	close(fd);
	return NULL;
}

int do_exec(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
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

int do_exec (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
U_BOOT_CMD(
	exec,	CONFIG_MAXARGS,	1,	do_exec,
	"exec     - execute a script\n",
	"<filename> - execute <filename>\n"
);

