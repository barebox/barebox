#include <common.h>
#include <command.h>
#include <fs.h>
#include <errno.h>
#include <malloc.h>
#include <linux/ctype.h>
#include <getopt.h>
#include <linux/stat.h>
#include <xfuncs.h>

#define LS_RECURSIVE	1
#define LS_SHOWARG	2

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

U_BOOT_CMD_START(ls)
	.maxargs	= CONFIG_MAXARGS,
	.cmd		= do_ls,
	.usage		= "ls      - list a file or directory\n",
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

U_BOOT_CMD_START(cd)
	.maxargs	= 2,
	.cmd		= do_cd,
	.usage		= "cd      - change current directory\n",
U_BOOT_CMD_END

static int do_pwd (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	printf("%s\n", getcwd());
	return 0;
}

U_BOOT_CMD_START(pwd)
	.maxargs	= 2,
	.cmd		= do_pwd,
	.usage		= "pwd      - display current directory\n",
U_BOOT_CMD_END

static int do_mkdir (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int ret;

	ret = mkdir(argv[1]);
	if (ret) {
		perror("mkdir");
		return 1;
	}

	return 0;
}

U_BOOT_CMD_START(mkdir)
	.maxargs	= 2,
	.cmd		= do_mkdir,
	.usage		= "mkdir    - create a new directory\n",
U_BOOT_CMD_END

static int do_rm (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int ret;

	ret = unlink(argv[1]);
	if (ret) {
		perror("rm");
		return 1;
	}

	return 0;
}

U_BOOT_CMD_START(rm)
	.maxargs	= 2,
	.cmd		= do_rm,
	.usage		= "rm    - remove files\n",
U_BOOT_CMD_END

static int do_rmdir (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int ret;

	ret = rmdir(argv[1]);
	if (ret) {
		perror("rmdir");
		return 1;
	}

	return 0;
}

U_BOOT_CMD_START(rmdir)
	.maxargs	= 2,
	.cmd		= do_rmdir,
	.usage		= "rmdir <dir>   - remove directories\n",
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
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	if ((ret = mount(argv[1], argv[2], argv[3]))) {
		perror("mount");
		return 1;
	}
	return 0;
}

U_BOOT_CMD_START(mount)
	.maxargs	= 4,
	.cmd		= do_mount,
	.usage		= "mount   - mount a filesystem to a device\n",
	U_BOOT_CMD_HELP(" <device> <type> <path> add a filesystem of type 'type' on the given device")
U_BOOT_CMD_END

static int do_umount (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int ret = 0;

	if (argc != 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	if ((ret = umount(argv[1]))) {
		perror("umount");
		return 1;
	}
	return 0;

}

U_BOOT_CMD_START(umount)
	.maxargs	= 2,
	.cmd		= do_umount,
	.usage		= "umount <path>   - umount a filesystem mounted on <path>\n",
U_BOOT_CMD_END

static int do_cat ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int ret;
	int fd, i;
	char *buf;
	int err = 0;

	fd = open(argv[1], 0);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	buf = xmalloc(1024);

	while((ret = read(fd, buf, 1024)) > 0) {
		for(i = 0; i < ret; i++) {
			if (isprint(buf[i]) || buf[i] == '\n' || buf[i] == '\t')
				putc(buf[i]);
			else
				putc('.');
		}
		if(ctrlc()) {
			err = 1;
			goto out;
		}
	}
out:
	free(buf);
	close(fd);

	return err;
}

U_BOOT_CMD_START(cat)
	.maxargs	= 2,
	.cmd		= do_cat,
	.usage		= "cat      - bla bla\n",
U_BOOT_CMD_END
