static int do_rm (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i = 1;

	if (argc < 2) {
		u_boot_cmd_usage(cmdtp);
		return 1;
	}

	while (i < argc) {
		if (unlink(argv[i])) {
			printf("could not remove %s: %s\n", argv[i], errno_str());
			return 1;
		}
		i++;
	}

	return 0;
}

static __maybe_unused char cmd_rm_help[] =
"Usage: rm [FILES]\n"
"Remove files\n";

U_BOOT_CMD_START(rm)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_rm,
	.usage		= "remove files",
	U_BOOT_CMD_HELP(cmd_rm_help)
U_BOOT_CMD_END
