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

int envfs_save(char *path, char *env_path)
{
	struct dir *dir;
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

	envfd = open(path, O_WRONLY);
	if (envfd < 0) {
		perror("open");
		return -1;
	}

	super.magic = ENVFS_MAGIC;

	write(envfd, &super, sizeof(struct envfs_super));

	dir = opendir(env_path);
	if (!dir) {
		perror("opendir");
		close(envfd);
		return errno;
	}

	while ((d = readdir(dir))) {
		sprintf(tmp, "%s/%s", env_path, d->name);
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
		strcpy(inode->name, d->name); /* FIXME: strncpy */
		/* calc crc */
		if (write(envfd, inode, isize) < isize) {
			perror("write");
			goto out;
		}
	}

	*(unsigned long *)tmp = ENVFS_END_MAGIC;
	if (write(envfd, &tmp, 4) < 4) {
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

int do_envsave(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	char *env, *envpath;

	printf("saving environment\n");
	if (argc < 3)
		envpath = "/env";
	else
		envpath = argv[2];
	if (argc < 2)
		env = "/dev/env0";
	else
		env = argv[1];

	return envfs_save(env, envpath);
}

U_BOOT_CMD(
	envsave, 3, 0,	do_envsave,
	"saveenv - save environment variables to persistent storage\n",
	NULL
);

int envfs_load(char *path, char *env_path)
{
	int malloc_size = 0;
	struct envfs_super super;
	void *buf = NULL;
	char tmp[PATH_MAX];
	int envfd;
	struct envfs_inode inode;
	int fd;

	envfd = open(path, O_RDONLY);
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
			printf("envfs: wrong magic on %s\n", path);
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
		sprintf(tmp, "%s/%s", env_path, inode.name);

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

int do_envload(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	char *env, *envpath;

	if (argc < 3)
		envpath = "/env";
	else
		envpath = argv[2];
	if (argc < 2)
		env = "/dev/env0";
	else
		env = argv[1];
	printf("loading environment\n");
	return envfs_load(env, envpath);
}

U_BOOT_CMD(
	envload, 3, 0,	do_envload,
	"envload - bla\n",
	NULL
);

