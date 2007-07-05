#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <fs.h>
#include <command.h>
#include <errno.h>
#include <asm-generic/errno.h>
#include <linux/stat.h>

struct handle_d {
	int (*read)(struct handle_d *, ...);
	int (*write)(struct handle_d *, ...);

	struct device_d *dev;
};

struct data_d {
	char *data;
	ulong size;
	struct data_d *next;
};

struct node_d {
	char *name;
	struct node_d *next;
	struct node_d *child;
	ulong mode;

	struct handle_d *handle;

	struct data_d *data;
};

struct filesystem_d {
	int (*create)(struct device_d *dev, const char *pathname, mode_t mode);
	struct handle_d *(*open)(struct device_d *dev, const char *pathname, mode_t mode);
	int (*remove)(struct device_d *dev, const char *pathname);
	int (*mknod)(struct device_d *dev, const char *pathname, struct handle_d *handle);
	int (*ls)(struct device_d *dev, const char *pathname);
};

int create(const char *pathname, ulong mode);
struct handle_d *open(const char *pathname, ulong type);
int remove(const char *pathname);
int mknod(const char *pathname, struct handle_d *handle);

struct ramfs_priv {
	struct node_d root;
};

/* ---------------------------------------------------------------*/
struct node_d * __lookup(struct node_d *node, const char *name)
{
//	printf("__lookup: %s in %p\n",name, node);
	node = node->child;
	if (!node || node->mode != S_IFDIR)
		return NULL;

	while (node) {
		if (!strcmp(node->name, name))
			return node;
		node = node->next;
	}
        return NULL;
}

struct node_d* rlookup(struct node_d *node, const char *path)
{
        static char *buf;
        char *part;
//printf("rlookup %s in %p\n",path, node);
        buf = strdup(path);

        part = strtok(buf, "/");
        if (!part)
		goto out;

        do {
                node = __lookup(node, part);
                if (!node)
			goto out;
                part = strtok(NULL, "/");
        } while(part);

out:
	free(buf);
        return node;
}

/*
 * - Remove all multiple slashes
 * - Remove trailing slashes (except path consists of only
 *   a single slash)
 * - TODO: illegal characters?
 */
void normalise_path(char *path)
{
        char *out = path, *in = path;

        while(*in) {
                if(*in == '/') {
                        *out++ = *in++;
                        while(*in == '/')
                                in++;
                } else {
                        *out++ = *in++;
                }
        }

        /*
         * Remove trailing slash, but only if
         * we were given more than a single slash
         */
        if (out > path + 1 && *(out - 1) == '/')
                *(out - 1) = 0;

        *out = 0;
}

int node_add_child(struct node_d *node, const char *filename, ulong mode)
{
	struct node_d *new_node = malloc(sizeof(struct node_d));
	memset(new_node, 0, sizeof(struct node_d));
	new_node->name = strdup(filename);
	new_node->mode = mode;

//	printf("node_add_child: %p -> %p\n", node, new_node);
	if (!node->child) {
		node->child = new_node;
		return 0;
	}

	node = node->child;

	while (node->next)
		node = node->next;

	node->next = new_node;
	return 0;
}

/* ---------------------------------------------------------------*/

int ramfs_create(struct device_d *dev, const char *pathname, ulong mode)
{
	struct ramfs_priv *priv = dev->priv;
	char *path = strdup(pathname);
	char *file;
	struct node_d *node;

	normalise_path(path);
	if (*path == '/')
		path++;

//	printf("after normalise: %s\n",path);

	if ((file = strrchr(path, '/'))) {
		*file = 0;
		file++;
		node = rlookup(&priv->root, path);
		if (!node)
			return -ENOENT;
	} else {
		file = path;
		node = &priv->root;
	}

	if(__lookup(node, file))
		return -EEXIST;

	return node_add_child(node, file, mode);
}

int ramfs_mkdir(struct device_d *dev, const char *pathname)
{
	return ramfs_create(dev, pathname, S_IFDIR);
}

int ramfs_probe(struct device_d *dev)
{
	struct ramfs_priv *priv = malloc(sizeof(struct ramfs_priv));

	printf("ramfs_probe\n");
	memset(priv, 0, sizeof(struct ramfs_priv));

	dev->priv = priv;

	priv->root.name = "/";
	priv->root.mode = S_IFDIR;

	return 0;
}

static struct handle_d *ramfs_open(struct device_d *dev, const char *pathname)
{
	return NULL;
}

struct dir* ramfs_opendir(struct device_d *dev, const char *pathname)
{
	struct dir *dir;
	struct ramfs_priv *priv = dev->priv;
	struct node_d *node = rlookup(&priv->root, pathname);

	if (!node)
		return NULL;

	if (node->mode != S_IFDIR)
		return NULL;

	dir = malloc(sizeof(struct dir));
	if (!dir)
		return NULL;

	dir->node = node->child;

	return dir;
}

struct dirent* ramfs_readdir(struct device_d *dev, struct dir *dir)
{
	if (dir->node) {
		strcpy(dir->d.name, dir->node->name);
		dir->d.mode = dir->node->mode;
		dir->node = dir->node->next;
		return &dir->d;
	}
	return NULL;
}

int ramfs_closedir(struct device_d *dev, struct dir *dir)
{
	free(dir);
	return 0;
}

static struct fs_driver_d ramfs_driver = {
	.type   = FS_TYPE_RAMFS,
	.create = ramfs_create,
	.open   = ramfs_open,

	.mkdir    = ramfs_mkdir,

	.opendir  = ramfs_opendir,
	.readdir  = ramfs_readdir,
	.closedir = ramfs_closedir,

	.flags    = FS_DRIVER_NO_DEV,
	.drv = {
		.type   = DEVICE_TYPE_FS,
		.probe  = ramfs_probe,
		.name = "ramfs",
		.type_data = &ramfs_driver,
	}
};

int ramfs_init(void)
{
	return register_driver(&ramfs_driver.drv);
}

device_initcall(ramfs_init);

/* --------- Testing --------- */

static int do_create ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
//	int ret;

	printf("create %s\n",argv[1]);
//	ret = ramfs_create(&ramfs_device, argv[1], S_IFDIR);
//	perror("create", ret);
	return 0;
}

U_BOOT_CMD(
	create,     2,     0,      do_create,
	"ls      - list a file or directory\n",
	"<dev:path> list files on device"
);

