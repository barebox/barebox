#include <common.h>
#include <command.h>
#include <fs.h>
#include <errno.h>
#include <asm-generic/errno.h>
#include <malloc.h>
#include <linux/ctype.h>

int do_ls (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int ret;

	ret = ls(argv[1]);
	if (ret) {
		perror("ls");
		return 1;
	}

	return 0;
}

U_BOOT_CMD(
	ls,     2,     0,      do_ls,
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

static int do_mount (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	struct device_d *dev;
	int ret = 0;

	if (argc != 4) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	dev = get_device_by_id(argv[1]);

	if ((ret = mount(dev, argv[2], argv[3]))) {
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
	char *buf = malloc(1024);

	fd = open(argv[1], 0);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	while((ret = read(fd, buf, 1024)) > 0) {
		for(i = 0; i < ret; i++) {
			if (isprint(buf[i]) || buf[i] == '\n' || buf[i] == '\t')
				putc(buf[i]);
			else
				putc('.');
		}
		if(ctrlc())
			return 1;
	}

	close(fd);

	return 0;
}

U_BOOT_CMD(
	cat,     2,     0,      do_cat,
	"cat      - bla bla\n",
	"<dev:path> list files on device"
);
/* --------- /Testing --------- */

