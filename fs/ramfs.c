#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <fs.h>
#include <command.h>
#include <errno.h>
#include <asm-generic/errno.h>
#include <linux/stat.h>

#define CHUNK_SIZE	512

struct ramfs_chunk {
	char *data;
	struct ramfs_chunk *next;
};

struct ramfs_inode {
	char *name;
	struct ramfs_inode *next;
	struct ramfs_inode *child;
	ulong mode;

	struct handle_d *handle;

	ulong size;
	struct ramfs_chunk *data;
};

struct ramfs_priv {
	struct ramfs_inode root;
};

/* ---------------------------------------------------------------*/
struct ramfs_inode * __lookup(struct ramfs_inode *node, const char *name)
{
//	printf("__lookup: %s in %p\n",name, node);
	if(node->mode != S_IFDIR)
		return NULL;

	node = node->child;
	if (!node)
		return NULL;

	while (node) {
		if (!strcmp(node->name, name))
			return node;
		node = node->next;
	}
        return NULL;
}

struct ramfs_inode* rlookup(struct ramfs_inode *node, const char *path)
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

struct ramfs_chunk *ramfs_get_chunk(void)
{
	struct ramfs_chunk *data = malloc(sizeof(struct ramfs_chunk));
	printf("get chunk\n");
	data->data = malloc(CHUNK_SIZE);
	data->next = NULL;
	return data;
}

void ramfs_put_chunk(struct ramfs_chunk *data)
{
	printf("put chunk\n");
	free(data->data);
	free(data);
}

struct ramfs_inode* node_add_child(struct ramfs_inode *node, const char *filename, ulong mode)
{
	struct ramfs_inode *new_node = malloc(sizeof(struct ramfs_inode));
	memset(new_node, 0, sizeof(struct ramfs_inode));
	new_node->name = strdup(filename);
	new_node->mode = mode;

	if (!(mode & S_IFDIR))
		new_node->data = ramfs_get_chunk();

	printf("node_add_child: %p -> %p\n", node, new_node);
	if (!node->child) {
		node->child = new_node;
		return new_node;
	}

	node = node->child;

	while (node->next)
		node = node->next;

	node->next = new_node;
	return new_node;
}

/* ---------------------------------------------------------------*/

int ramfs_create(struct device_d *dev, const char *pathname, mode_t mode)
{
	struct ramfs_priv *priv = dev->priv;
	char *path = strdup(pathname);
	char *file;
	struct ramfs_inode *node;

	normalise_path(path);
	if (*path == '/')
		path++;

	printf("ramfs create: %s\n",path);

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
printf("CREATE\n");
	node_add_child(node, file, mode);
	return 0;
}

int ramfs_mkdir(struct device_d *dev, const char *pathname)
{
	return ramfs_create(dev, pathname, S_IFDIR);
}

static int ramfs_open(struct device_d *dev, FILE *file, const char *filename)
{
	struct ramfs_priv *priv = dev->priv;
	struct ramfs_inode *node = rlookup(&priv->root, filename);

	if (!node)
		return -ENOENT;

	file->size = node->size;
	file->inode = node;
	return 0;
}

static int ramfs_close(struct device_d *dev, FILE *f)
{
	return 0;
}

static int ramfs_read(struct device_d *_dev, FILE *f, void *buf, size_t insize)
{
	struct ramfs_inode *node = (struct ramfs_inode *)f->inode;
	int chunk;
	struct ramfs_chunk *data;
	int ofs;
	int now;
	int pos = f->pos;
	int size = insize;

	chunk = f->pos / CHUNK_SIZE;
	printf("%s: reading from chunk %d\n", __FUNCTION__, chunk);

	/* Position ourself in stream */
	data = node->data;
	while (chunk) {
		data = data->next;
		chunk--;
	}
	ofs = f->pos % CHUNK_SIZE;

	/* Read till end of current chunk */
	if (ofs) {
		now = min(size, CHUNK_SIZE - ofs);
		printf("Reading till end of node. size: %d\n", size);
		memcpy(buf, data->data + ofs, now);
		size -= now;
		pos += now;
		buf += now;
		if (pos > node->size)
			node->size = now;
	}

	/* Do full chunks */
	while (size >= CHUNK_SIZE) {
		printf("do full chunk. size: %d\n", size);
		data = data->next;
		memcpy(buf, data->data, CHUNK_SIZE);
		size -= CHUNK_SIZE;
		pos += CHUNK_SIZE;
		buf += CHUNK_SIZE;
	}

	/* And the rest */
	if (size) {
		printf("do rest. size: %d\n", size);
		data = data->next;
		memcpy(buf, data->data, size);
	}

	return insize;
}

static int ramfs_write(struct device_d *_dev, FILE *f, const void *buf, size_t insize)
{
	struct ramfs_inode *node = (struct ramfs_inode *)f->inode;
	int chunk;
	struct ramfs_chunk *data;
	int ofs;
	int now;
	int pos = f->pos;
	int size = insize;

	chunk = f->pos / CHUNK_SIZE;
	printf("%s: writing to chunk %d\n", __FUNCTION__, chunk);

	/* Position ourself in stream */
	data = node->data;
	while (chunk) {
		data = data->next;
		chunk--;
	}
	ofs = f->pos % CHUNK_SIZE;

	/* Write till end of current chunk */
	if (ofs) {
		now = min(size, CHUNK_SIZE - ofs);
		printf("writing till end of node. size: %d\n", size);
		memcpy(data->data, buf, now);
		size -= now;
		pos += now;
		buf += now;
		if (pos > node->size)
			node->size = now;
	}

	/* Do full chunks */
	while (size >= CHUNK_SIZE) {
		printf("do full chunk. size: %d\n", size);
		data = data->next;
		memcpy(data->data, buf, CHUNK_SIZE);
		size -= CHUNK_SIZE;
		pos += CHUNK_SIZE;
		buf += CHUNK_SIZE;
	}

	/* And the rest */
	if (size) {
		printf("do rest. size: %d\n", size);
		data = data->next;
		memcpy(data->data, buf, size);
	}

	return insize;
}

int ramfs_truncate(struct device_d *dev, FILE *f, ulong size)
{
	struct ramfs_inode *node = (struct ramfs_inode *)f->inode;
	int oldchunks, newchunks;
	struct ramfs_chunk *data = node->data;

	newchunks = (size + CHUNK_SIZE - 1) / CHUNK_SIZE;
	oldchunks = (node->size + CHUNK_SIZE - 1) / CHUNK_SIZE;

	if (newchunks < oldchunks) {
		while (newchunks--)
			data = data->next;
		while (data->next) {
			struct ramfs_chunk *tmp;
			tmp = data->next;
			ramfs_put_chunk(data);
			data = tmp;
		}
	}

	if (newchunks > oldchunks) {
		while (data->next) {
			newchunks--;
			data = data->next;
		}

		while (newchunks--) {
			data->next = ramfs_get_chunk();
			data = data->next;
		}
	}
	node->size = size;
	return 0;
}

struct dir* ramfs_opendir(struct device_d *dev, const char *pathname)
{
	struct dir *dir;
	struct ramfs_priv *priv = dev->priv;
	struct ramfs_inode *node = rlookup(&priv->root, pathname);

	if (!node)
		return NULL;

	if (node->mode != S_IFDIR)
		return NULL;

	dir = malloc(sizeof(struct dir));
	if (!dir)
		return NULL;

	dir->priv = node->child;

	return dir;
}

struct dirent* ramfs_readdir(struct device_d *dev, struct dir *dir)
{
	struct ramfs_inode *node = dir->priv;

	if (node) {
		strcpy(dir->d.name, node->name);
		dir->priv = node->next;
		return &dir->d;
	}
	return NULL;
}

int ramfs_closedir(struct device_d *dev, struct dir *dir)
{
	free(dir);
	return 0;
}

int ramfs_stat(struct device_d *dev, const char *filename, struct stat *s)
{
	struct ramfs_priv *priv = dev->priv;
	struct ramfs_inode *node = rlookup(&priv->root, filename);

	if (!node)
		return -ENOENT;

	s->st_size = node->size;
	s->st_mode = node->mode;

	return 0;
}

int ramfs_probe(struct device_d *dev)
{
	struct ramfs_priv *priv = malloc(sizeof(struct ramfs_priv));

	printf("ramfs_probe\n");
	memset(priv, 0, sizeof(struct ramfs_priv));

	dev->priv = priv;

	priv->root.name = "/";
	priv->root.mode = S_IFDIR;

	printf("root node: %p\n",&priv->root);
	return 0;
}

static struct fs_driver_d ramfs_driver = {
	.type      = FS_TYPE_RAMFS,
	.create    = ramfs_create,
	.open      = ramfs_open,
	.close     = ramfs_close,
	.truncate  = ramfs_truncate,
	.read      = ramfs_read,
	.write     = ramfs_write,
	.mkdir     = ramfs_mkdir,
	.opendir   = ramfs_opendir,
	.readdir   = ramfs_readdir,
	.closedir  = ramfs_closedir,
	.stat      = ramfs_stat,
	.flags     = FS_DRIVER_NO_DEV,
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

