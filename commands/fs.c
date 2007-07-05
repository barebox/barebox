#include <common.h>
#include <command.h>
#include <fs.h>
#include <errno.h>
#include <malloc.h>
#include <linux/ctype.h>
#include <getopt.h>
#include <linux/stat.h>
#include <xfuncs.h>

static void ls_one(const char *path, struct stat *s)
{
	char modestr[11];
	unsigned long namelen = strlen(path);

	mkmodestr(s->st_mode, modestr);
	printf("%s %8d %*.*s\n",modestr, s->st_size, namelen, namelen, path);
}

int ls(const char *path, ulong flags)
{
	DIR *dir;
	struct dirent *d;
	char tmp[PATH_MAX];
	struct stat s;

	if (flags & LS_SHOWARG)
		printf("%s:\n", path);

	if (stat(path, &s)) {
		perror("stat");
		return errno;
	}

	if (!(s.st_mode & S_IFDIR)) {
		ls_one(path, &s);
		return 0;
	}

	dir = opendir(path);
	if (!dir)
		return errno;

	while ((d = readdir(dir))) {
		sprintf(tmp, "%s/%s", path, d->d_name);
		if (stat(tmp, &s))
			goto out;
		ls_one(d->d_name, &s);
	}

	closedir(dir);

	if (!(flags & LS_RECURSIVE))
		return 0;

	dir = opendir(path);
	if (!dir) {
		errno = -ENOENT;
		return -ENOENT;
	}

	while ((d = readdir(dir))) {
		sprintf(tmp, "%s/%s", path, d->d_name);
		normalise_path(tmp);
		if (stat(tmp, &s))
			goto out;
		if (!strcmp(d->d_name, "."))
			continue;
		if (!strcmp(d->d_name, ".."))
			continue;
		if (s.st_mode & S_IFDIR)
			ls(tmp, flags);
	}
out:
	closedir(dir);

	return 0;
}

static int do_ls (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int ret, opt;
	ulong flags = 0;

	getopt_reset();

	while((opt = getopt(argc, argv, "R")) > 0) {
		switch(opt) {
		case 'R':
			flags |= LS_RECURSIVE | LS_SHOWARG;
			break;
		}
	}

	if (argc - optind > 1)
		flags |= LS_SHOWARG;

	if (optind == argc) {
		ret = ls(getcwd(), flags);
		return ret ? 1 : 0;
	}

	while (optind < argc) {
		ret = ls(argv[optind], flags);
		if (ret) {
			perror("ls");
			return 1;
		}
		optind++;
	}

	return 0;
}

static __maybe_unused char cmd_ls_help[] =
"Usage: ls [OPTION]... [FILE]...\n"
"List information about the FILEs (the current directory by default).\n"
"  -R  list subdirectories recursively\n";

U_BOOT_CMD_START(ls)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_ls,
	.usage		= "list a file or directory",
	U_BOOT_CMD_HELP(cmd_ls_help)
U_BOOT_CMD_END

static int do_cd (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int ret;

	if (argc == 1)
		ret = chdir("/");
	else
		ret = chdir(argv[1]);

	if (ret) {
		perror("chdir");
		return 1;
	}

	return 0;
}

static __maybe_unused char cmd_cd_help[] =
"Usage: cd [directory]\n"
"change to directory. If called without argument, change to /\n";

U_BOOT_CMD_START(cd)
	.maxargs	= 2,
	.cmd		= do_cd,
	.usage		= "change working directory",
	U_BOOT_CMD_HELP(cmd_cd_help)
U_BOOT_CMD_END

static int do_pwd (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	printf("%s\n", getcwd());
	return 0;
}

U_BOOT_CMD_START(pwd)
	.maxargs	= 2,
	.cmd		= do_pwd,
	.usage		= "print working directory",
U_BOOT_CMD_END

static int do_mkdir (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i = 1;

	if (argc < 2) {
		u_boot_cmd_usage(cmdtp);
		return 1;
	}

	while (i < argc) {
		if (mkdir(argv[i])) {
			printf("could not create %s: %s\n", argv[i], errno_str());
			return 1;
		}
		i++;
	}

	return 0;
}

static __maybe_unused char cmd_mkdir_help[] =
"Usage: mkdir [directories]\n"
"Create new directories\n";

U_BOOT_CMD_START(mkdir)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_mkdir,
	.usage		= "make directories",
	U_BOOT_CMD_HELP(cmd_mkdir_help)
U_BOOT_CMD_END

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

static int do_rmdir (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i = 1;

	if (argc < 2) {
		u_boot_cmd_usage(cmdtp);
		return 1;
	}

	while (i < argc) {
		if (rmdir(argv[i])) {
			printf("could not remove %s: %s\n", argv[i], errno_str());
			return 1;
		}
		i++;
	}

	return 0;
}

static __maybe_unused char cmd_rmdir_help[] =
"Usage: rmdir [directories]\n"
"Remove directories. The directories have to be empty.\n";

U_BOOT_CMD_START(rmdir)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_rmdir,
	.usage		= "remove directorie(s)",
	U_BOOT_CMD_HELP(cmd_rmdir_help)
U_BOOT_CMD_END

static int do_mount (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int ret = 0;
	struct mtab_entry *entry = NULL;

	if (argc == 1) {
		do {
			entry = mtab_next_entry(entry);
			if (entry) {
				printf("%s on %s type %s\n",
					entry->parent_device ? entry->parent_device->id : "none",
					entry->path,
					entry->dev->name);
			}
		} while (entry);
		return 0;
	}

	if (argc != 4) {
		u_boot_cmd_usage(cmdtp);
		return 1;
	}

	if ((ret = mount(argv[1], argv[2], argv[3]))) {
		perror("mount");
		return 1;
	}
	return 0;
}

static __maybe_unused char cmd_mount_help[] =
"Usage: mount:         list mounted filesystems\n"
"or:    mount <device> <fstype> <mountpoint>\n"
"\n"
"Mount a filesystem of a given type to a mountpoint.\n"
"<device> can be one of /dev/* or some arbitrary string if no\n"
"device is needed for this driver (for example ramfs).\n"
"<fstype> is the filesystem driver to use. Try the 'devinfo' command\n"
"for a list of available drivers.\n"
"<mountpoint> must be an empty directory descending directly from the\n"
"root directory.\n";

U_BOOT_CMD_START(mount)
	.maxargs	= 4,
	.cmd		= do_mount,
	.usage		= "mount a filesystem to a device",
	U_BOOT_CMD_HELP(cmd_mount_help)
U_BOOT_CMD_END

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

static int do_cat(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int ret;
	int fd, i;
	char *buf;
	int err = 0;
	int args = 1;

	if (argc < 2) {
		perror("cat");
		return 1;
	}

	buf = xmalloc(1024);

	while (args < argc) {
		fd = open(argv[args], 0);
		if (fd < 0) {
			printf("could not open %s: %s\n", argv[args], errno_str());
			goto out;
		}

		while((ret = read(fd, buf, 1024)) > 0) {
			for(i = 0; i < ret; i++) {
				if (isprint(buf[i]) || buf[i] == '\n' || buf[i] == '\t')
					putc(buf[i]);
				else
					putc('.');
			}
			if(ctrlc()) {
				err = 1;
				close(fd);
				goto out;
			}
		}
		close(fd);
		args++;
	}

out:
	free(buf);

	return err;
}

static __maybe_unused char cmd_cat_help[] =
"Usage: cat [FILES]\n"
"Concatenate files on stdout. Currently only printable characters\n"
"and \\n and \\t are printed, but this should be optional\n";

U_BOOT_CMD_START(cat)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_cat,
	.usage		= "concatenate file(s)",
	U_BOOT_CMD_HELP(cmd_cat_help)
U_BOOT_CMD_END
