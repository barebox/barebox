#ifdef __U_BOOT__
#include <common.h>
#include <command.h>
#include <driver.h>
#include <malloc.h>
#include <errno.h>
#include <fs.h>
#include <fcntl.h>
#include <linux/stat.h>
#include <envfs.h>
#include <xfuncs.h>
#endif

int envfs_save(char *filename, char *dirname)
{
	DIR *dir;
	struct dirent *d;
	int malloc_size = 0;
	struct stat s;
	struct envfs_super super;
	void *buf = NULL;
	char tmp[PATH_MAX];
	int isize;
	int envfd;
	struct envfs_inode *inode = NULL;
	int fd;

	envfd = open(filename, O_WRONLY);
	if (envfd < 0) {
		perror("open");
		return -1;
	}

	super.magic = ENVFS_MAGIC;

	write(envfd, &super, sizeof(struct envfs_super));

	dir = opendir(dirname);
	if (!dir) {
		perror("opendir");
		close(envfd);
		return errno;
	}

	while ((d = readdir(dir))) {
		sprintf(tmp, "%s/%s", dirname, d->d_name);
		if (stat(tmp, &s)) {
			perror("stat");
			goto out;
		}
		if (s.st_mode & S_IFDIR)
			continue;

		isize = ((s.st_size + 3) & ~3) + sizeof(struct envfs_inode);
		if (isize > malloc_size) {
			if (buf)
				free(buf);
			inode = xmalloc(isize);
			buf = inode;
			malloc_size = isize;
		}
		fd = open(tmp, O_RDONLY);
		if (fd < 0) {
			perror("open");
			goto out;
		}
		if (read(fd, inode->data, s.st_size) < s.st_size) {
			perror("read");
			goto out;
		}

		close(fd);
		inode->magic = ENVFS_INODE_MAGIC;
		inode->size  = s.st_size;
		strcpy(inode->name, d->d_name); /* FIXME: strncpy */
		/* FIXME: calc crc */
		if (write(envfd, inode, isize) < isize) {
			perror("write");
			goto out;
		}
	}

	memset(inode, 0, sizeof(struct envfs_inode));
	inode->magic = ENVFS_END_MAGIC;
	if (write(envfd, inode, sizeof(struct envfs_inode)) < sizeof(struct envfs_inode)) {
		perror("write");
		goto out;
	}
out:
	if (buf)
		free(buf);
	close(envfd);
	closedir(dir);
	return 0;
}

#ifdef __U_BOOT__
int do_saveenv(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	char *filename, *dirname;

	printf("saving environment\n");
	if (argc < 3)
		dirname = "/env";
	else
		dirname = argv[2];
	if (argc < 2)
		filename = "/dev/env0";
	else
		filename = argv[1];

	return envfs_save(filename, dirname);
}

U_BOOT_CMD_START(saveenv)
	.maxargs	= 3,
	.cmd		= do_saveenv,
	.usage		= "saveenv - save environment to persistent storage\n",
U_BOOT_CMD_END
#endif /* __U_BOOT__ */

int envfs_load(char *filename, char *dirname)
{
	int malloc_size = 0;
	struct envfs_super super;
	void *buf = NULL;
	char tmp[PATH_MAX];
	int envfd;
	struct envfs_inode inode;
	int fd;

	envfd = open(filename, O_RDONLY);
	if (envfd < 0) {
		perror("open");
		return -1;
	}

	if (read(envfd, &super, sizeof(struct envfs_super)) < sizeof(struct envfs_super)) {
		perror("read");
		goto out;
	}

	while (1) {
		if (read(envfd, &inode, sizeof(struct envfs_inode)) < sizeof(struct envfs_inode)) {
			perror("read");
			goto out;
		}
		if (inode.magic == ENVFS_END_MAGIC)
			break;
		if (inode.magic != ENVFS_INODE_MAGIC) {
			printf("envfs: wrong magic on %s\n", filename);
			goto out;
		}
		if (inode.size > malloc_size) {
			if (buf)
				free(buf);
			buf = xmalloc(inode.size);
			malloc_size = inode.size;
		}
		if (read(envfd, buf, inode.size) < inode.size) {
			perror("read");
			goto out;
		}
		sprintf(tmp, "%s/%s", dirname, inode.name);

		fd = open(tmp, O_WRONLY | O_CREAT);
		if (fd < 0) {
			perror("open");
			goto out;
		}

		if (write(fd, buf, inode.size) < inode.size) {
			perror("write");
			close(fd);
			goto out;
		}
		close(fd);

		if (inode.size & 0x3) {
			if (read(envfd, buf, 4 - (inode.size & 0x3)) < 4 - (inode.size & 0x3)) {
				perror("read");
				goto out;
			}
		}
	}

out:
	close(envfd);
	if (buf)
		free(buf);
	return errno;
}

#ifdef __U_BOOT__
int do_loadenv(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	char *filename, *dirname;

	if (argc < 3)
		dirname = "/env";
	else
		dirname = argv[2];
	if (argc < 2)
		filename = "/dev/env0";
	else
		filename = argv[1];
	printf("loading environment from %s\n", filename);
	return envfs_load(filename, dirname);
}

U_BOOT_CMD_START(loadenv)
	.maxargs	= 3,
	.cmd		= do_loadenv,
	.usage		= "loadenv - load environment from persistent storage\n",
U_BOOT_CMD_END
#endif /* __U_BOOT__ */
