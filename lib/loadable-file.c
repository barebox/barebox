// SPDX-License-Identifier: GPL-2.0-only

#include <fs.h>
#include <libfile.h>
#include <linux/ctype.h>
#include <linux/minmax.h>
#include <linux/sprintf.h>
#include <linux/stat.h>
#include <loadable.h>
#include <string.h>
#include <xfuncs.h>

struct file_loadable_priv {
	char *path;
	int fd;
	int read_fd;
};

static int file_loadable_get_info(struct loadable *l,
				  struct loadable_info *info)
{
	struct file_loadable_priv *priv = l->priv;
	struct stat s;
	int ret;

	ret = stat(priv->path, &s);
	if (ret)
		return ret;

	if (s.st_size == FILE_SIZE_STREAM)
		s.st_size = LOADABLE_SIZE_UNKNOWN;
	else if (FILE_SIZE_STREAM != LOADABLE_SIZE_UNKNOWN &&
		 s.st_size > LOADABLE_SIZE_UNKNOWN)
		return -EOVERFLOW;

	info->final_size = s.st_size;

	return 0;
}

static int file_loadable_open_and_lseek(struct file_loadable_priv *priv, loff_t pos)
{
	int ret, fd = priv->read_fd;

	if (fd < 0) {
		fd = open(priv->path, O_RDONLY);
		if (fd < 0)
			return fd;
	}

	if (lseek(fd, pos, SEEK_SET) != pos) {
		ret = -errno;
		priv->read_fd = -1;
		close(fd);
		return ret;
	}

	priv->read_fd = fd;
	return fd;
}

/**
 * file_loadable_extract_into_buf - load file data to target address
 * @l: loadable representing a file
 * @load_addr: virtual address to load data to
 * @size: size of buffer at load_addr
 * @offset: offset within the file to start reading from
 * @flags: A bitmask of OR-ed LOADABLE_EXTRACT_ flags
 *
 * Extracts the file to the specified memory address.
 *
 * File is read as-is from the filesystem.
 * The caller must provide a valid address range; this function does not allocate
 * memory.
 *
 * Return: actual number of bytes read on success, negative errno on error
 *         -ENOSPC if size is too small for the file (only when loading from start)
 */
static ssize_t file_loadable_extract_into_buf(struct loadable *l,
					      void *load_addr, size_t size,
					      loff_t offset, unsigned flags)
{
	struct file_loadable_priv *priv = l->priv;
	char buf;
	ssize_t ret;
	int fd;

	fd = file_loadable_open_and_lseek(priv, offset);
	if (fd < 0)
		return fd;

	ret = __read_full_anywhere(fd, load_addr, size);
	if (!(flags & LOADABLE_EXTRACT_PARTIAL) && ret == size &&
	    __read_full_anywhere(fd, &buf, 1) > 0)
		ret = -ENOSPC;

	return ret;
}

/**
 * file_loadable_extract - allocate buffer and load file data into it
 * @l: loadable representing a file
 * @size: on success, set to the number of bytes read
 *
 * Allocates a buffer and extracts the file data into it.
 *
 * No decompression is performed - the file is read as-is from the filesystem.
 * The caller is responsible for freeing the returned buffer with free().
 *
 * Return: allocated buffer, NULL for zero-size, or error pointer on failure
 */
static void *file_loadable_extract(struct loadable *l, size_t *size)
{
	struct file_loadable_priv *priv = l->priv;
	void *buf;
	int ret;

	ret = read_file_2(priv->path, size, &buf, FILESIZE_MAX);
	if (ret)
		return ERR_PTR(ret);

	if (*size == 0) {
		free(buf);
		buf = NULL;
	}

	return buf;
}

/**
 * file_loadable_mmap - memory map file data
 * @l: loadable representing a file
 * @size: on success, set to the size of the mapped region
 *
 * Memory maps the file without copying data into a separate buffer.
 *
 * No decompression is performed - the file is mapped as-is from the filesystem.
 *
 * Return: read-only pointer to mapped data, NULL for zero-size, or MAP_FAILED on error
 */
static const void *file_loadable_mmap(struct loadable *l, size_t *size)
{
	struct file_loadable_priv *priv = l->priv;
	struct stat st;
	void *buf;
	int fd, ret;

	if (unlikely(priv->fd >= 0))
		return MAP_FAILED;

	/* Reuse the read fd if already open, otherwise open a new one */
	if (priv->read_fd >= 0) {
		fd = priv->read_fd;
		priv->read_fd = -1;
	} else {
		fd = open(priv->path, O_RDONLY);
	}
	if (fd < 0)
		return MAP_FAILED;

	ret = fstat(fd, &st);
	if (ret || st.st_size == FILE_SIZE_STREAM)
		goto err;
	if (!st.st_size) {
		close(fd);
		*size = 0;
		return NULL;
	}

	buf = memmap(fd, PROT_READ);
	if (buf == MAP_FAILED)
		goto err;

	*size = min_t(u64, st.st_size, SIZE_MAX);
	priv->fd = fd;
	return buf;

err:
	close(fd);
	return MAP_FAILED;
}

static void file_loadable_munmap(struct loadable *l, const void *buf,
				 size_t size)
{
	struct file_loadable_priv *priv = l->priv;

	if (priv->fd >= 0) {
		close(priv->fd);
		priv->fd = -1;
	}
}

static void file_loadable_release(struct loadable *l)
{
	struct file_loadable_priv *priv = l->priv;

	if (priv->read_fd >= 0)
		close(priv->read_fd);
	if (priv->fd >= 0)
		close(priv->fd);
	free(priv->path);
	free(priv);
}

static const struct loadable_ops file_loadable_ops = {
	.get_info = file_loadable_get_info,
	.extract_into_buf = file_loadable_extract_into_buf,
	.extract = file_loadable_extract,
	.mmap = file_loadable_mmap,
	.munmap = file_loadable_munmap,
	.release = file_loadable_release,
};

/**
 * loadable_from_file - create a loadable from filesystem file
 * @path: filesystem path to the file
 * @type: type of loadable (LOADABLE_KERNEL, LOADABLE_INITRD, etc.)
 *
 * Creates a loadable structure that wraps access to a file in the filesystem.
 * The file is read directly during extract or via mmap with no decompression
 * - it is loaded as-is from the filesystem.
 *
 * The created loadable must be freed with loadable_release() when done.
 * The file path is copied internally, so the caller's string can be freed.
 *
 * Return: pointer to allocated loadable on success, ERR_PTR() on error
 */
struct loadable *loadable_from_file(const char *path, enum loadable_type type)
{
	char *cpath;
	struct loadable *l;
	struct file_loadable_priv *priv;

	cpath = canonicalize_path(AT_FDCWD, path);
	if (!cpath) {
		const char *typestr = loadable_type_tostr(type);
		int ret = -errno;

		pr_err("%s%simage \"%s\" not found: %m\n",
		       typestr, typestr ? " ": "", path);

		return ERR_PTR(ret);
	}

	l = xzalloc(sizeof(*l));
	priv = xzalloc(sizeof(*priv));

	priv->path = cpath;
	priv->fd = -1;
	priv->read_fd = -1;

	l->name = xasprintf("File(%s)", path);
	l->type = type;
	l->ops = &file_loadable_ops;
	l->priv = priv;
	loadable_init(l);

	return l;
}

/**
 * loadables_from_files - create loadables from a list of file paths
 * @l: pointer to loadable chain (updated on return)
 * @files: delimited list of file paths
 * @delimiters: each character on its own is a valid delimiter for @files
 * @type: type of loadable (LOADABLE_KERNEL, LOADABLE_INITRD, etc.)
 *
 * This helper splits up @files by any character in @delimiters and creates
 * loadables for every file it extracts. File extraction is done as follows:
 *
 * - Multiple whitespace is treated as if it were a single whitespace.
 * - Leading and trailing whitespace is ignored
 * - When a file name is empty, because a non-whitespace delimiter was placed
 *   at the beginning, the end or two delimiters were repeated, the original
 *   loadable from @l is spliced into the chain at that location. If @l is NULL,
 *   nothing happens
 * - Regular non-empty file names have loadables created for them
 *
 * This is basically the algorithm by which PATH is interpreted in
 * a POSIX-conformant shell (when delimiters=":" and the original is the current
 * working directory).
 *
 * Return: 0 on success, negative errno on failure
 */
int loadables_from_files(struct loadable **l,
			 const char *files, const char *delimiters,
			 enum loadable_type type)
{
	struct loadable *orig = *l;
	struct loadable *main = NULL;
	struct loadable *lf, *ltmp;
	char *file, *buf, *origbuf = xstrdup(files);
	LIST_HEAD(tmp);
	bool orig_included = false;
	char delim;

	if (isempty(files))
		goto out;

	buf = origbuf;
	while ((file = strsep_unescaped(&buf, delimiters, &delim))) {
		if (*file == '\0') {
			if (!isspace(delim) && !orig_included && orig) {
				list_add_tail(&orig->list, &tmp);
				orig_included = true;
			}
			continue;
		}

		lf = loadable_from_file(file, type);
		if (IS_ERR(lf)) {
			int ret = PTR_ERR(lf);

			if (orig_included)
				list_del_init(&orig->list);
			list_for_each_entry_safe(lf, ltmp, &tmp, list)
				loadable_release(&lf);
			free(origbuf);
			return ret;
		}

		list_add_tail(&lf->list, &tmp);
	}

	free(origbuf);

	/* Build the final chain from collected entries */
	list_for_each_entry_safe(lf, ltmp, &tmp, list) {
		list_del_init(&lf->list);
		loadable_chain(&main, lf);
	}

out:
	if (!orig_included)
		loadable_release(&orig);

	*l = main;
	return 0;
}
