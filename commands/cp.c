#include <common.h>
#include <command.h>
#include <fs.h>
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <xfuncs.h>

#define RW_BUF_SIZE	(ulong)4096

int do_cp ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int r, w, ret = 1;
	int src = 0, dst = 0;
	char *rw_buf = NULL;

	rw_buf = xmalloc(RW_BUF_SIZE);

	src = open(argv[1], O_RDONLY);
	if (src < 0) {
		printf("could not open %s: %s\n", src, errno_str());
		goto out;
	}

	dst = open(argv[2], O_WRONLY | O_CREAT);
	if ( dst < 0) {
		printf("could not open %s: %s\n", dst, errno_str());
		goto out;
	}

	while(1) {
		r = read(src, rw_buf, RW_BUF_SIZE);
		if (read < 0) {
			perror("read");
			goto out;
		}
		if (!r)
			break;
		w = write(dst, rw_buf, r);
		if (w < 0) {
			perror("write");
			goto out;
		}
	}
	ret = 0;
out:
	free(rw_buf);
	if (src)
		close(src);
	if (dst)
		close(dst);
	return ret;
}

static __maybe_unused char cmd_cp_help[] =
"Usage: cp <source> <destination>\n"
"cp copies file <source> to <destination>.\n"
"Currently only this form is supported and you have to specify the exact target\n"
"filename (not a target directory).\n"
"Note: This command was previously used as memory copy. Currently there is no\n"
"equivalent command for this. This must be fixed of course.\n";

U_BOOT_CMD_START(cp)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_cp,
	.usage		= "copy files",
	U_BOOT_CMD_HELP(cmd_cp_help)
U_BOOT_CMD_END
