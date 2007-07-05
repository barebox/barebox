#include <common.h>
#include <command.h>
#include <fs.h>
#include <errno.h>

static int do_umount (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int ret = 0;

	if (argc != 2) {
		u_boot_cmd_usage(cmdtp);
		return 1;
	}

	if ((ret = umount(argv[1]))) {
		perror("umount");
		return 1;
	}
	return 0;
}

static __maybe_unused char cmd_umount_help[] =
"Usage: umount <mountpoint>\n"
"umount a filesystem mounted on a specific mountpoint\n";

U_BOOT_CMD_START(umount)
	.maxargs	= 2,
	.cmd		= do_umount,
	.usage		= "umount a filesystem",
	U_BOOT_CMD_HELP(cmd_umount_help)
U_BOOT_CMD_END
