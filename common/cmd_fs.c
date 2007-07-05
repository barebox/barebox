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
	struct dir *dir;
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
		sprintf(tmp, "%s/%s", path, d->name);
		if (stat(tmp, &s))
			goto out;
		ls_one(d->name, &s);
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
		sprintf(tmp, "%s/%s", path, d->name);
		normalise_path(tmp);
		if (stat(tmp, &s))
			goto out;
		if (!strcmp(d->name, "."))
			continue;
		if (!strcmp(d->name, ".."))
			continue;
		if (s.st_mode & S_IFDIR)
			ls(tmp, flags);
	}
out:
	closedir(dir);

	return 0;
}

int do_ls (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
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

	while(optind < argc) {
		ret = ls(argv[optind], flags);
		if (ret) {
			perror("ls");
			return 1;
		}
		optind++;
	}

	return 0;
}

U_BOOT_CMD(
	ls,     10000,     0,      do_ls,
	"ls      - list a file or directory\n",
	"<path> list files on path"
);

int do_mkdir (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int ret;

	ret = mkdir(argv[1]);
	if (ret) {
		perror("mkdir");
		return 1;
	}

	return 0;
}

U_BOOT_CMD(
	mkdir,     2,     0,      do_mkdir,
	"mkdir    - create a new directory\n",
	""
);

int do_rm (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int ret;

	ret = unlink(argv[1]);
	if (ret) {
		perror("rm");
		return 1;
	}

	return 0;
}

U_BOOT_CMD(
	rm,     2,     0,      do_rm,
	"rm    - remove files\n",
	""
);

int do_rmdir (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int ret;

	ret = rmdir(argv[1]);
	if (ret) {
		perror("rmdir");
		return 1;
	}

	return 0;
}

U_BOOT_CMD(
	rmdir,     2,     0,      do_rmdir,
	"rm    - remove files\n",
	""
);

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

U_BOOT_CMD(
	mount,     4,     0,      do_mount,
	"mount   - mount a filesystem to a device\n",
	" <device> <type> <path> add a filesystem of type 'type' on the given device"
);

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

U_BOOT_CMD(
	umount,     2,     0,      do_umount,
	"umount   - umount a filesystem\n",
	" <device> <type> <path> add a filesystem of type 'type' on the given device"
);

/* --------- Testing --------- */

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

U_BOOT_CMD(
	cat,     2,     0,      do_cat,
	"cat      - bla bla\n",
	"<dev:path> list files on device"
);
/* --------- /Testing --------- */

