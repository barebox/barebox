// SPDX-License-Identifier: GPL-2.0+
// SPDX-Comment: Origin-URL: https://github.com/u-boot/u-boot/blob/9d95a35715fcb8e81ee423e31273489a47ed1563/lib/efi_loader/efi_file.c
/*
 * EFI_FILE_PROTOCOL
 *
 * Copyright (c) 2017 Rob Clark
 */

#define pr_fmt(fmt) "efi-loader: file: " fmt

#include <errno.h>
#include <linux/nls.h>
#include <linux/overflow.h>
#include <sys/stat.h>
#include <efi/loader.h>
#include <efi/error.h>
#include <efi/guid.h>
#include <efi/devicepath.h>
#include <efi/loader/devicepath.h>
#include <efi/loader/trace.h>
#include <efi/loader/event.h>
#include <efi/loader/file.h>
#include <efi/protocol/file.h>
#include <wchar.h>
#include <string.h>
#include <libfile.h>
#include <disks.h>
#include <charset.h>
#include <malloc.h>
#include <dirent.h>
#include <fs.h>

#define MAX_UTF8_PER_UTF16 3

struct file_system {
	struct efi_simple_file_system_protocol base;
	struct efi_device_path *dp;
	struct cdev *cdev;
	struct file_handle *fh;
};
#define to_fs(x) container_of(x, struct file_system, base)

struct file_handle {
	struct efi_file_handle base;
	struct file_handle *parent;
	struct file_system *fs;
	int fd;
	u64 open_mode;

	/* for reading a directory: */
	DIR *dir;

	char path[];
};
#define to_fh(x) container_of(x, struct file_handle, base)

static const struct efi_file_handle efi_file_handle_protocol;

static int efi_opendir(int dirfd, const char *path, struct file_handle *fh)
{
	fh->fd = openat(dirfd, path, O_DIRECTORY | O_CHROOT);
	if (fh->fd < 0) {
		pr_err("openat(%d, \"%s\") failed: %m\n", dirfd, path);
		return fh->fd;
	}

	fh->dir = fdopendir(fh->fd);
	if (!fh->dir) {
		close(fh->fd);
		return -errno;
	}

	return 0;
}

/**
 * file_open() - open a file handle
 *
 * @parent:		directory relative to which the file is to be opened
 * @file_name:		path of the file to be opened. '\', '.', or '..' may
 *			be used as modifiers. A leading backslash indicates an
 *			absolute path.
 * @open_mode:		bit mask indicating the access mode (read, write,
 *			create)
 * @attributes:		attributes for newly created file
 * Returns:		handle to the opened file or NULL
 */
static struct efi_file_handle *file_open(struct file_handle *parent,
					 efi_char16_t *file_name, u64 open_mode,
		u64 attributes)
{
	struct file_handle *fh;
	int dirfd;
	bool is_dir = false;
	struct stat s;
	int flags = 0;
	char *p;
	int flen;

	flen = wcslen(file_name);

	fh = xzalloc(struct_size(fh, path, (flen * MAX_UTF8_PER_UTF16) + 2));

	fh->open_mode = open_mode;
	fh->base = efi_file_handle_protocol;
	fh->parent = parent;
	fh->fs = parent->fs;

	p = fh->path;
	utf16_utf8_strcpy(&p, file_name);

	strreplace(fh->path, '\\', '/');

	dirfd = parent->dir ? parent->fd : parent->parent->fd;

	if (open_mode & (EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE))
		flags |= O_RDWR;
	else
		flags |= O_RDONLY;

	if (open_mode & EFI_FILE_MODE_CREATE) {
		flags |= O_CREAT;

		if (attributes & EFI_FILE_DIRECTORY)
			is_dir = true;
	}

	if (is_dir || (!statat(dirfd, fh->path, &s) && S_ISDIR(s.st_mode))) {
		if (flags & O_CREAT)
			mkdirat(dirfd, fh->path, 0);

		if (efi_opendir(dirfd, fh->path, fh))
			goto error;
	} else {
		fh->fd = openat(dirfd, fh->path, flags);
		if (fh->fd < 0)
			goto error;
	}

	return &fh->base;

error:
	free(fh);
	return NULL;
}

static efi_status_t efi_file_open_int(struct efi_file_handle *this,
				      struct efi_file_handle **new_handle,
				      efi_char16_t *file_name, u64 open_mode,
				      u64 attributes)
{
	struct file_handle *fh = to_fh(this);
	efi_status_t efiret;

	/* Check parameters */
	if (!this || !new_handle || !file_name) {
		efiret = EFI_INVALID_PARAMETER;
		goto out;
	}
	if (open_mode != EFI_FILE_MODE_READ &&
	    open_mode != (EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE) &&
	    open_mode != (EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE |
			 EFI_FILE_MODE_CREATE)) {
		efiret = EFI_INVALID_PARAMETER;
		goto out;
	}
	/*
	 * The UEFI spec requires that attributes are only set in create mode.
	 * The SCT does not care about this and sets EFI_FILE_DIRECTORY in
	 * read mode. EDK2 does not check that attributes are zero if not in
	 * create mode.
	 *
	 * So here we only check attributes in create mode and do not check
	 * that they are zero otherwise.
	 */
	if ((open_mode & EFI_FILE_MODE_CREATE) &&
	    (attributes & (EFI_FILE_READ_ONLY | ~EFI_FILE_VALID_ATTR))) {
		efiret = EFI_INVALID_PARAMETER;
		goto out;
	}

	/* Open file */
	*new_handle = file_open(fh, file_name, open_mode, attributes);
	if (*new_handle)
		efiret = EFI_SUCCESS;
	else
		efiret = EFI_NOT_FOUND;
out:
	return efiret;
}

/**
 * efi_file_open_()
 *
 * This function implements the Open service of the File Protocol.
 * See the UEFI spec for details.
 *
 * @this:	EFI_FILE_PROTOCOL instance
 * @new_handle:	on return pointer to file handle
 * @file_name:	file name
 * @open_mode:	mode to open the file (read, read/write, create/read/write)
 * @attributes:	attributes for newly created file
 */
static efi_status_t EFIAPI efi_file_open(struct efi_file_handle *this,
					 struct efi_file_handle **new_handle,
					 efi_char16_t *file_name, u64 open_mode,
					 u64 attributes)
{
	efi_status_t efiret;

	EFI_ENTRY("%p, %p, \"%ls\", %llx, %llu", this, new_handle,
		  file_name, open_mode, attributes);

	efiret = efi_file_open_int(this, new_handle, file_name, open_mode,
				attributes);

	return EFI_EXIT2(efiret, *new_handle);
}

/**
 * efi_file_open_ex() - open file asynchronously
 *
 * This function implements the OpenEx service of the File Protocol.
 * See the UEFI spec for details.
 *
 * @this:	EFI_FILE_PROTOCOL instance
 * @new_handle:	on return pointer to file handle
 * @file_name:	file name
 * @open_mode:	mode to open the file (read, read/write, create/read/write)
 * @attributes:	attributes for newly created file
 * @token:	transaction token
 */
static efi_status_t EFIAPI efi_file_open_ex(struct efi_file_handle *this,
					    struct efi_file_handle **new_handle,
					    efi_char16_t *file_name, u64 open_mode,
					    u64 attributes,
					    struct efi_file_io_token *token)
{
	efi_status_t efiret;

	EFI_ENTRY("%p, %p, \"%ls\", %llx, %llu, %p", this, new_handle,
		  file_name, open_mode, attributes, token);

	if (!token) {
		efiret = EFI_INVALID_PARAMETER;
		goto out;
	}

	efiret = efi_file_open_int(this, new_handle, file_name, open_mode,
				attributes);

	if (efiret == EFI_SUCCESS && token->event) {
		token->status = EFI_SUCCESS;
		efi_signal_event(token->event);
	}

out:
	return EFI_EXIT2(efiret, *new_handle);
}

static void file_close(struct file_handle *fh)
{
	if (fh->dir)
		closedir(fh->dir);
	else
		close(fh->fd);
}

static efi_status_t EFIAPI efi_file_close(struct efi_file_handle *file)
{
	struct file_handle *fh = to_fh(file);
	EFI_ENTRY("%p", file);
	file_close(fh);
	free(fh);
	return EFI_EXIT(EFI_SUCCESS);
}

static efi_status_t EFIAPI efi_file_delete(struct efi_file_handle *file)
{
	struct file_handle *fh = to_fh(file);
	int flags = fh->dir ? AT_REMOVEDIR : 0;
	int ret;

	EFI_ENTRY("%p", file);

	file_close(fh);

	ret = unlinkat(fh->parent->fd, fh->path, flags);

	free(fh);

	return EFI_EXIT(ret ? EFI_WARN_DELETE_FAILURE : EFI_SUCCESS);
}

/**
 * efi_get_file_size() - determine the size of a file
 *
 * @fh:		file handle
 * @file_size:	pointer to receive file size
 * Return:	status code
 */
static efi_status_t efi_get_file_size(struct file_handle *fh,
				      loff_t *file_size)
{
	struct stat s;
	int ret;

	if (fh->dir) {
		*file_size = 0;
		return EFI_SUCCESS;
	}

	ret = fstat(fh->fd, &s);
	if (ret)
		return EFI_DEVICE_ERROR;

	*file_size = s.st_size;

	return EFI_SUCCESS;
}

/**
 * efi_file_size() - Get the size of a file using an EFI file handle
 *
 * @fh:		EFI file handle
 * @size:	buffer to fill in the discovered size
 *
 * Return:	size of the file
 */
efi_status_t efi_file_size(struct efi_file_handle *fh, size_t *size)
{
	struct efi_file_info *info = NULL;
	size_t bs = 0;
	efi_status_t efiret;

	*size = 0;
	efiret = fh->get_info(fh, (efi_guid_t *)&efi_file_info_id, &bs, info);
	if (efiret != EFI_BUFFER_TOO_SMALL) {
		efiret = EFI_DEVICE_ERROR;
		goto out;
	}

	info = malloc(bs);
	if (!info) {
		efiret = EFI_OUT_OF_RESOURCES;
		goto out;
	}
	efiret = fh->get_info(fh, (efi_guid_t *)&efi_file_info_id, &bs, info);
	if (efiret != EFI_SUCCESS)
		goto out;

	*size = info->FileSize;

out:
	free(info);
	return efiret;
}

static efi_status_t file_read(struct file_handle *fh, u64 *buffer_size,
		void *buffer)
{
	ssize_t nbytes;

	if (!buffer)
		return EFI_INVALID_PARAMETER;

	nbytes = read_full(fh->fd, buffer, *buffer_size);
	if (nbytes < 0)
		return EFI_DEVICE_ERROR;

	*buffer_size = nbytes;

	return EFI_SUCCESS;
}

static efi_status_t dir_read(struct file_handle *fh, u64 *buffer_size,
		void *buffer)
{
	struct efi_file_info *info = buffer;
	struct dirent *d;
	u64 required_size;
	struct stat s;
	efi_char16_t *dst;
	int ret;

	do {
		d = readdir(fh->dir);
	} while (d && (!strcmp(d->d_name, ".") || !strcmp(d->d_name, "..")));

	if (!d) {
		/* no more files in directory */
		*buffer_size = 0;
		return EFI_SUCCESS;
	}

	/* check buffer size: */
	required_size = struct_size(info, FileName, utf8_utf16_strlen(d->d_name) + 1);
	if (*buffer_size < required_size) {
		*buffer_size = required_size;
		unreaddir(fh->dir, d);
		return EFI_BUFFER_TOO_SMALL;
	}
	if (!buffer)
		return EFI_INVALID_PARAMETER;

	ret = statat(fh->fd, d->d_name, &s);
	if (ret)
		return EFI_DEVICE_ERROR;

	*buffer_size = required_size;
	memset(info, 0, required_size);

	dst = info->FileName;
	utf8_utf16_strcpy(&dst, d->d_name);

	info->Size = required_size;
	info->FileSize = s.st_size;
	info->PhysicalSize = s.st_size;
	info->Attribute = 0;
	info->CreateTime = (struct efi_time) {0}; // TODO
	info->ModificationTime = (struct efi_time) {0};
	info->LastAccessTime = (struct efi_time) {0};

	if (S_ISDIR(s.st_mode))
		info->Attribute |= EFI_FILE_DIRECTORY;

	return EFI_SUCCESS;
}

static efi_status_t efi_file_read_int(struct efi_file_handle *this,
				      size_t *buffer_size, void *buffer)
{
	struct file_handle *fh = to_fh(this);
	efi_status_t efiret = EFI_SUCCESS;
	u64 bs;

	if (!this || !buffer_size)
		return EFI_INVALID_PARAMETER;

	bs = *buffer_size;
	if (fh->dir)
		efiret = dir_read(fh, &bs, buffer);
	else
		efiret = file_read(fh, &bs, buffer);
	if (bs <= SIZE_MAX)
		*buffer_size = bs;
	else
		*buffer_size = SIZE_MAX;

	return efiret;
}

/**
 * efi_file_read() - read file
 *
 * This function implements the Read() service of the EFI_FILE_PROTOCOL.
 *
 * See the Unified Extensible Firmware Interface (UEFI) specification for
 * details.
 *
 * @this:		file protocol instance
 * @buffer_size:	number of bytes to read
 * @buffer:		read buffer
 * Return:		status code
 */
static efi_status_t EFIAPI efi_file_read(struct efi_file_handle *this,
					 size_t *buffer_size, void *buffer)
{
	efi_status_t efiret;

	EFI_ENTRY("%p, *%p(%zu), %p", this, buffer_size,
		  buffer_size ? *buffer_size : 0, buffer);

	efiret = efi_file_read_int(this, buffer_size, buffer);

	return EFI_EXIT2(efiret, *buffer_size);
}

/**
 * efi_file_read_ex() - read file asynchonously
 *
 * This function implements the ReadEx() service of the EFI_FILE_PROTOCOL.
 *
 * See the Unified Extensible Firmware Interface (UEFI) specification for
 * details.
 *
 * @this:		file protocol instance
 * @token:		transaction token
 * Return:		status code
 */
static efi_status_t EFIAPI efi_file_read_ex(struct efi_file_handle *this,
					    struct efi_file_io_token *token)
{
	efi_status_t efiret;

	EFI_ENTRY("%p, %p", this, token);

	if (!token) {
		efiret = EFI_INVALID_PARAMETER;
		goto out;
	}

	efiret = efi_file_read_int(this, &token->buffer_size, token->buffer);

	if (efiret == EFI_SUCCESS && token->event) {
		token->status = EFI_SUCCESS;
		efi_signal_event(token->event);
	}

out:
	return EFI_EXIT(efiret);
}

static efi_status_t efi_file_write_int(struct efi_file_handle *this,
				       size_t *buffer_size, void *buffer)
{
	struct file_handle *fh = to_fh(this);
	efi_status_t efiret = EFI_SUCCESS;
	ssize_t nbytes;

	if (!this || !buffer_size || !buffer) {
		efiret = EFI_INVALID_PARAMETER;
		goto out;
	}
	if (fh->dir) {
		efiret = EFI_UNSUPPORTED;
		goto out;
	}
	if (!(fh->open_mode & EFI_FILE_MODE_WRITE)) {
		efiret = EFI_ACCESS_DENIED;
		goto out;
	}

	if (!*buffer_size)
		goto out;

	nbytes = write_full(fh->fd, buffer, *buffer_size);
	if (nbytes < 0) {
		efiret = errno == ENOSPC ? EFI_VOLUME_FULL : EFI_DEVICE_ERROR;
		goto out;
	}

	*buffer_size = nbytes;

out:
	return efiret;
}

/**
 * efi_file_write() - write to file
 *
 * This function implements the Write() service of the EFI_FILE_PROTOCOL.
 *
 * See the Unified Extensible Firmware Interface (UEFI) specification for
 * details.
 *
 * @this:		file protocol instance
 * @buffer_size:	number of bytes to write
 * @buffer:		buffer with the bytes to write
 * Return:		status code
 */
static efi_status_t EFIAPI efi_file_write(struct efi_file_handle *this,
					  size_t *buffer_size,
					  void *buffer)
{
	efi_status_t efiret;

	EFI_ENTRY("%p, %p, %p", this, buffer_size, buffer);

	efiret = efi_file_write_int(this, buffer_size, buffer);

	return EFI_EXIT2(efiret, *buffer_size);
}

/**
 * efi_file_write_ex() - write to file
 *
 * This function implements the WriteEx() service of the EFI_FILE_PROTOCOL.
 *
 * See the Unified Extensible Firmware Interface (UEFI) specification for
 * details.
 *
 * @this:		file protocol instance
 * @token:		transaction token
 * Return:		status code
 */
static efi_status_t EFIAPI efi_file_write_ex(struct efi_file_handle *this,
					     struct efi_file_io_token *token)
{
	efi_status_t efiret;

	EFI_ENTRY("%p, %p", this, token);

	if (!token) {
		efiret = EFI_INVALID_PARAMETER;
		goto out;
	}

	efiret = efi_file_write_int(this, &token->buffer_size, token->buffer);

	if (efiret == EFI_SUCCESS && token->event) {
		token->status = EFI_SUCCESS;
		efi_signal_event(token->event);
	}

out:
	return EFI_EXIT(efiret);
}

/**
 * efi_file_getpos() - get current position in file
 *
 * This function implements the GetPosition service of the EFI file protocol.
 * See the UEFI spec for details.
 *
 * @file:	file handle
 * @pos:	pointer to file position
 * Return:	status code
 */
static efi_status_t EFIAPI efi_file_getpos(struct efi_file_handle *file,
					   u64 *pos)
{
	struct file_handle *fh = to_fh(file);
	loff_t ret;

	EFI_ENTRY("%p, %p", file, pos);

	if (fh->dir)
		return EFI_EXIT(EFI_UNSUPPORTED);

	ret = lseek(fh->fd, 0, SEEK_CUR);
	if (ret < 0)
		return EFI_EXIT(EFI_DEVICE_ERROR);

	*pos = ret;

	return EFI_EXIT2(EFI_SUCCESS, pos);
}

/**
 * efi_file_setpos() - set current position in file
 *
 * This function implements the SetPosition service of the EFI file protocol.
 * See the UEFI spec for details.
 *
 * @file:	file handle
 * @pos:	new file position
 * Return:	status code
 */
static efi_status_t EFIAPI efi_file_setpos(struct efi_file_handle *file,
					   u64 pos)
{
	struct file_handle *fh = to_fh(file);
	loff_t ret;

	EFI_ENTRY("%p, %llu", file, pos);

	if (fh->dir) {
		if (pos)
			return EFI_EXIT(EFI_UNSUPPORTED);
		rewinddir(fh->dir);
		return EFI_EXIT(EFI_SUCCESS);
	}

	if (pos == ~0ULL)
		ret = lseek(fh->fd, 0, SEEK_END);
	else
		ret = lseek(fh->fd, pos, SEEK_SET);

	return EFI_EXIT(ret < 0 ? EFI_DEVICE_ERROR : EFI_SUCCESS);
}

static efi_status_t EFIAPI efi_file_get_info(struct efi_file_handle *file,
					    const efi_guid_t *info_type,
					    size_t *buffer_size,
					    void *buffer)
{
	struct file_handle *fh = to_fh(file);
	efi_status_t efiret = EFI_SUCCESS;

	EFI_ENTRY("%p, %pUs, %p, %p", file, info_type, buffer_size, buffer);

	if (!file || !info_type || !buffer_size ||
	    (*buffer_size && !buffer)) {
		efiret = EFI_INVALID_PARAMETER;
		goto error;
	}

	if (!efi_guidcmp(*info_type, efi_file_info_id)) {
		struct efi_file_info *info = buffer;
		const char *filename = kbasename(fh->path);
		unsigned int required_size;
		efi_char16_t *dst;
		loff_t file_size;

		/* check buffer size: */
		required_size = sizeof(*info) +
				2 * (utf8_utf16_strlen(filename) + 1);
		if (*buffer_size < required_size) {
			*buffer_size = required_size;
			efiret = EFI_BUFFER_TOO_SMALL;
			goto error;
		}

		efiret = efi_get_file_size(fh, &file_size);
		if (efiret != EFI_SUCCESS)
			goto error;

		memset(info, 0, required_size);

		info->Size = required_size;
		info->FileSize = file_size;
		info->PhysicalSize = file_size;

		if (fh->dir)
			info->Attribute |= EFI_FILE_DIRECTORY;

		dst = info->FileName;
		utf8_utf16_strcpy(&dst, filename);
	} else if (!efi_guidcmp(*info_type, efi_file_system_info_guid)) {
		struct efi_file_system_info *info = buffer;
		size_t required_size;

		required_size = sizeof(*info) + 2;
		if (*buffer_size < required_size) {
			*buffer_size = required_size;
			efiret = EFI_BUFFER_TOO_SMALL;
			goto error;
		}

		memset(info, 0, required_size);

		info->size = required_size;
		info->read_only = fh->fs->cdev->flags & DEVFS_PARTITION_READONLY;
		info->volume_size = fh->fs->cdev->size;
		/*
		 * TODO: We currently have no function to determine the free
		 * space. The volume size is the best upper bound we have.
		 */
		info->free_space = info->volume_size;
		info->block_size = SECTOR_SIZE;
		/*
		 * TODO: The volume label is not available in barebox.
		 */
		info->volume_label[0] = 0;
	} else if (!efi_guidcmp(*info_type, efi_system_volume_label_id)) {
		if (*buffer_size < 2) {
			*buffer_size = 2;
			efiret = EFI_BUFFER_TOO_SMALL;
			goto error;
		}
		*(efi_char16_t *)buffer = 0;
	} else {
		efiret = EFI_UNSUPPORTED;
	}

error:
	return EFI_EXIT2(efiret, *buffer_size);
}

static efi_status_t EFIAPI efi_file_set_info(struct efi_file_handle *file,
					    const efi_guid_t *info_type,
					    size_t buffer_size,
					    void *buffer)
{
	struct file_handle *fh = to_fh(file);
	efi_status_t efiret = EFI_UNSUPPORTED;

	EFI_ENTRY("%p, %pUs, %zu, %p", file, info_type, buffer_size, buffer);

	if (!efi_guidcmp(*info_type, efi_file_info_id)) {
		struct efi_file_info *info = (struct efi_file_info *)buffer;
		const char *filename = kbasename(fh->path);
		char *new_file_name, *pos;
		loff_t file_size;

		/* The buffer will always contain a file name. */
		if (buffer_size < sizeof(struct efi_file_info) + 2 ||
		    buffer_size < info->Size) {
			efiret = EFI_BAD_BUFFER_SIZE;
			goto out;
		}
		/* We cannot change the directory attribute */
		if (!!fh->dir != !!(info->Attribute & EFI_FILE_DIRECTORY)) {
			efiret = EFI_ACCESS_DENIED;
			goto out;
		}
		/* Check for renaming */
		new_file_name = malloc(utf16_utf8_strlen(info->FileName) + 1);
		if (!new_file_name) {
			efiret = EFI_OUT_OF_RESOURCES;
			goto out;
		}
		pos = new_file_name;
		utf16_utf8_strcpy(&pos, info->FileName);
		if (strcmp(new_file_name, filename)) {
			/* TODO: we do not support renaming */
			EFI_PRINT("Renaming not supported\n");
			free(new_file_name);
			efiret = EFI_ACCESS_DENIED;
			goto out;
		}
		free(new_file_name);
		/* Check for truncation */
		efiret = efi_get_file_size(fh, &file_size);
		if (efiret != EFI_SUCCESS)
			goto out;
		if (file_size != info->FileSize) {
			/* TODO: we do not support truncation */
			EFI_PRINT("Truncation not supported\n");
			efiret = EFI_ACCESS_DENIED;
			goto out;
		}
		/*
		 * We do not care for the other attributes
		 * TODO: Support read only
		 */
		efiret = EFI_SUCCESS;
	} else {
		/* TODO: We do not support changing the volume label */
		efiret = EFI_UNSUPPORTED;
	}
out:
	return EFI_EXIT(efiret);
}

/**
 * efi_file_flush_int() - flush file
 *
 * This is the internal implementation of the Flush() and FlushEx() services of
 * the EFI_FILE_PROTOCOL.
 *
 * @this:	file protocol instance
 * Return:	status code
 */
static efi_status_t efi_file_flush_int(struct efi_file_handle *this)
{
	struct file_handle *fh = to_fh(this);

	if (!this)
		return EFI_INVALID_PARAMETER;

	if (!(fh->open_mode & EFI_FILE_MODE_WRITE))
		return EFI_ACCESS_DENIED;

	/* TODO: flush for file position after end of file */
	return EFI_SUCCESS;
}

/**
 * efi_file_flush() - flush file
 *
 * This function implements the Flush() service of the EFI_FILE_PROTOCOL.
 *
 * See the Unified Extensible Firmware Interface (UEFI) specification for
 * details.
 *
 * @this:	file protocol instance
 * Return:	status code
 */
static efi_status_t EFIAPI efi_file_flush(struct efi_file_handle *this)
{
	efi_status_t efiret;

	EFI_ENTRY("%p", this);

	efiret = efi_file_flush_int(this);

	return EFI_EXIT(efiret);
}

/**
 * efi_file_flush_ex() - flush file
 *
 * This function implements the FlushEx() service of the EFI_FILE_PROTOCOL.
 *
 * See the Unified Extensible Firmware Interface (UEFI) specification for
 * details.
 *
 * @this:	file protocol instance
 * @token:	transaction token
 * Return:	status code
 */
static efi_status_t EFIAPI efi_file_flush_ex(struct efi_file_handle *this,
					     struct efi_file_io_token *token)
{
	efi_status_t efiret;

	EFI_ENTRY("%p, %p", this, token);

	if (!token) {
		efiret = EFI_INVALID_PARAMETER;
		goto out;
	}

	efiret = efi_file_flush_int(this);

	if (efiret == EFI_SUCCESS && token->event) {
		token->status = EFI_SUCCESS;
		efi_signal_event(token->event);
	}

out:
	return EFI_EXIT(efiret);
}

static const struct efi_file_handle efi_file_handle_protocol = {
	.Revision = EFI_FILE_HANDLE_REVISION2,
	.open = efi_file_open,
	.close = efi_file_close,
	.delete = efi_file_delete,
	.read = efi_file_read,
	.write = efi_file_write,
	.get_position = efi_file_getpos,
	.set_position = efi_file_setpos,
	.get_info = efi_file_get_info,
	.set_info = efi_file_set_info,
	.flush = efi_file_flush,
	.open_ex = efi_file_open_ex,
	.read_ex = efi_file_read_ex,
	.write_ex = efi_file_write_ex,
	.flush_ex = efi_file_flush_ex,
};

/**
 * efi_file_from_path() - open file via device path
 *
 * @fp:		device path
 * Return:	EFI_FILE_PROTOCOL for the file or NULL
 */
struct efi_file_handle *efi_file_from_path(struct efi_device_path *fp)
{
	struct efi_simple_file_system_protocol *v;
	struct efi_file_handle *f;
	efi_status_t efiret;

	v = efi_fs_from_path(fp);
	if (!v)
		return NULL;

	efiret = v->open_volume(v, &f);
	if (efiret != EFI_SUCCESS)
		return NULL;

	/* Skip over device-path nodes before the file path. */
	while (fp && !EFI_DP_TYPE(fp, MEDIA_DEVICE, FILE_PATH))
		fp = efi_dp_next(fp);

	/*
	 * Step through the nodes of the directory path until the actual file
	 * node is reached which is the final node in the device path.
	 */
	while (fp) {
		struct efi_device_path_file_path *fdp =
			container_of(fp, struct efi_device_path_file_path, header);
		struct efi_file_handle *f2;
		efi_char16_t *filename;
		size_t filename_sz;

		if (!EFI_DP_TYPE(fp, MEDIA_DEVICE, FILE_PATH)) {
			pr_warn("bad file path!\n");
			f->close(f);
			return NULL;
		}

		/*
		 * UEFI specification requires pointers that are passed to
		 * protocol member functions to be aligned.  So memcpy it
		 * unconditionally
		 */
		if (fdp->header.length <= offsetof(struct efi_device_path_file_path, path_name))
			return NULL;
		filename_sz = fdp->header.length -
			offsetof(struct efi_device_path_file_path, path_name);
		filename = memdup(fdp->path_name, filename_sz);
		if (!filename)
			return NULL;
		efiret = f->open(f, &f2, filename, EFI_FILE_MODE_READ, 0);
		free(filename);
		if (efiret != EFI_SUCCESS)
			return NULL;

		fp = efi_dp_next(fp);

		f->close(f);
		f = f2;
	}

	return f;
}

static efi_status_t EFIAPI
efi_open_volume(struct efi_simple_file_system_protocol *this,
		struct efi_file_handle **root)
{
	struct file_system *fs = to_fs(this);
	const char rootpath[] = "/";
	struct file_handle *fh;
	const char *path;

	EFI_ENTRY("%p, %p", this, root);

	path = cdev_mount(fs->cdev);
	if (IS_ERR(path))
		return EFI_EXIT(EFI_DEVICE_ERROR);

	fh = xzalloc(struct_size(fh, path, ARRAY_SIZE(rootpath)));

	fh->open_mode = EFI_FILE_MODE_READ;
	fh->base = efi_file_handle_protocol;
	fh->fs = fs;

	fs->fh = fh;

	memcpy(fh->path, rootpath, sizeof(rootpath));

	EFI_PRINT("Open volume %s\n", path);

	if (efi_opendir(AT_FDCWD, path, fh) < 0)
		goto error;

	*root = &fh->base;

	return EFI_EXIT2(EFI_SUCCESS, *root);

error:
	free(fh);
	return EFI_EXIT(EFI_DEVICE_ERROR);
}

struct efi_simple_file_system_protocol *
efi_simple_file_system(struct cdev *cdev,
		       struct efi_device_path *dp)
{
	struct file_system *fs;

	fs = xzalloc(sizeof(*fs));
	fs->base.Revision = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION;
	fs->base.open_volume = efi_open_volume;
	fs->cdev = cdev;
	fs->dp = dp;

	return &fs->base;
}
