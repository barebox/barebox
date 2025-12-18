/* SPDX-License-Identifier: GPL-2.0-only */
// SPDX-Comment: Origin-URL: https://github.com/u-boot/u-boot/blob/d786c6b69f537ed2a64950028d270d064970d29f/lib/efi_loader/efi_device_path.c

#define pr_fmt(fmt) "efi-loader: devicepath: " fmt

#include <efi/loader.h>
#include <efi/guid.h>
#include <efi/loader/object.h>
#include <efi/devicepath.h>
#include <efi/loader/devicepath.h>
#include <efi/error.h>
#include <string.h>
#include <xfuncs.h>
#include <driver.h>
#include <charset.h>
#include <block.h>
#include <asm/unaligned.h>
#include <linux/overflow.h>
#include <fs.h>

/*
 * END - Template end node for EFI device paths.
 *
 * Represents the terminating node of an EFI device path.
 * It has a type of DEVICE_PATH_TYPE_END and sub_type DEVICE_PATH_SUB_TYPE_END
 */
static const struct efi_device_path END = {
	.type     = DEVICE_PATH_TYPE_END,
	.sub_type = DEVICE_PATH_SUB_TYPE_END,
	.length   = sizeof(END),
};

/* template ROOT node: */
static const struct efi_device_path_vendor ROOT = {
	.header = {
		.type     = DEVICE_PATH_TYPE_HARDWARE_DEVICE,
		.sub_type = DEVICE_PATH_SUB_TYPE_VENDOR,
		.length   = sizeof(ROOT),
	},
	.Guid = BAREBOX_GUID,
};

static void *dp_alloc(size_t sz)
{
	return efi_alloc(sz, "DP");
}

/**
 * efi_dp_next() - Iterate to next block in device-path
 *
 * Advance to the next node in an EFI device path.
 *
 * @dp: Pointer to the current device path node.
 * Return: Pointer to the next device path node, or NULL if at the end
 * or if input is NULL.
 */
struct efi_device_path *efi_dp_next(const struct efi_device_path *dp)
{
	if (dp == NULL)
		return NULL;
	if (dp->type == DEVICE_PATH_TYPE_END)
		return NULL;
	dp = ((void *)dp) + dp->length;
	if (dp->type == DEVICE_PATH_TYPE_END)
		return NULL;
	return (struct efi_device_path *)dp;
}

/**
 * efi_dp_match() - Compare two device-paths
 *
 * Compare two device paths node by node. The comparison stops when an End
 * node is reached in the shorter of the two paths. This is useful, for example,
 * to compare a device-path representing a device with one representing a file
 * on that device, or a device with a parent device.
 *
 * @a: Pointer to the first device path.
 * @b: Pointer to the second device path.
 * Return: An integer less than, equal to, or greater than zero if the first
 * differing node in 'a' is found, respectively, to be less than,
 * to match, or be greater than the corresponding node in 'b'. Returns 0
 * if they match up to the end of the shorter path. Compares length first,
 * then content.
 */
int efi_dp_match(const struct efi_device_path *a,
		 const struct efi_device_path *b)
{
	while (1) {
		int ret;

		ret = memcmp(&a->length, &b->length, sizeof(a->length));
		if (ret)
			return ret;

		ret = memcmp(a, b, a->length);
		if (ret)
			return ret;

		a = efi_dp_next(a);
		b = efi_dp_next(b);

		if (!a || !b)
			return 0;
	}
}

/**
 * efi_dp_shorten() - shorten device-path
 *
 * When creating a short-boot option we want to use a device-path that is
 * independent of the location where the block device is plugged in.
 *
 * UsbWwi() nodes contain a serial number, hard drive paths a partition
 * UUID. Both should be unique.
 *
 * See UEFI spec, section 3.1.2 for "short-form device path".
 *
 * @dp:     original device-path
 * Return:  shortened device-path or NULL
 */
static struct efi_device_path *efi_dp_shorten(struct efi_device_path *dp)
{
	while (dp) {
		if (EFI_DP_TYPE(dp, MEDIA_DEVICE, HARD_DRIVE_PATH) ||
		    EFI_DP_TYPE(dp, MEDIA_DEVICE, FILE_PATH))
			return dp;

		dp = efi_dp_next(dp);
	}

	return dp;
}

/**
 * find_handle() - find handle by device path and installed protocol
 *
 * If @rem is provided, the handle with the longest partial match is returned.
 *
 * @dp:		device path to search
 * @guid:	GUID of protocol that must be installed on path or NULL
 * @short_path:	use short form device path for matching
 * @rem:	pointer to receive remaining device path
 * Return:	matching handle
 */
static efi_handle_t find_handle(struct efi_device_path *dp,
				const efi_guid_t *guid, bool short_path,
				struct efi_device_path **rem)
{
	efi_handle_t handle, best_handle = NULL;
	efi_uintn_t len, best_len = 0;

	len = efi_dp_instance_size(dp);

	list_for_each_entry(handle, &efi_obj_list, link) {
		struct efi_handler *handler;
		struct efi_device_path *dp_current;
		efi_uintn_t len_current;
		efi_status_t ret;

		if (guid) {
			ret = efi_search_protocol(handle, guid, &handler);
			if (ret != EFI_SUCCESS)
				continue;
		}
		ret = efi_search_protocol(handle, &efi_device_path_protocol_guid,
					  &handler);
		if (ret != EFI_SUCCESS)
			continue;
		dp_current = handler->protocol_interface;
		if (short_path) {
			dp_current = efi_dp_shorten(dp_current);
			if (!dp_current)
				continue;
		}
		len_current = efi_dp_instance_size(dp_current);
		if (rem) {
			if (len_current > len)
				continue;
		} else {
			if (len_current != len)
				continue;
		}
		if (memcmp(dp_current, dp, len_current))
			continue;
		if (!rem)
			return handle;
		if (len_current > best_len) {
			best_len = len_current;
			best_handle = handle;
			*rem = (void*)((u8 *)dp + len_current);
		}
	}
	return best_handle;
}

/**
 * efi_dp_find_obj() - find handle by device path
 *
 * If @rem is provided, the handle with the longest partial match is returned.
 *
 * @dp:     device path to search
 * @guid:   GUID of protocol that must be installed on path or NULL
 * @rem:    pointer to receive remaining device path
 * Return:  matching handle
 */
efi_handle_t efi_dp_find_obj(struct efi_device_path *dp,
			     const efi_guid_t *guid,
			     struct efi_device_path **rem)
{
	efi_handle_t handle;

	handle = find_handle(dp, guid, false, rem);
	if (!handle)
		/* Match short form device path */
		handle = find_handle(dp, guid, true, rem);

	return handle;
}

/**
 * efi_dp_last_node() - Determine the last device path node before the end node
 *
 * Iterate through the device path to find the very last node before
 * the terminating EFI_DP_END node.
 *
 * @dp: Pointer to the device path.
 * Return: Pointer to the last actual data node before the end node if it exists
 * otherwise NULL (e.g., if dp is NULL or only an EFI_DP_END node).
 */
const struct efi_device_path *efi_dp_last_node(const struct efi_device_path *dp)
{
	struct efi_device_path *ret;

	if (!dp || dp->type == DEVICE_PATH_TYPE_END)
		return NULL;
	while (dp) {
		ret = (struct efi_device_path *)dp;
		dp = efi_dp_next(dp);
	}
	return ret;
}

/**
 * efi_dp_instance_size() - Get size of the first device path instance
 *
 * Calculate the total length of all nodes in the first instance of a
 * (potentially multi-instance) device path. The size of the instance-specific
 * end node (if any) or the final device path. The end node is not included.
 *
 * @dp: Pointer to the device path.
 * Return: Size in bytes of the first instance, or 0 if dp is NULL or an
 * EFI_DP_END node
 */
efi_uintn_t efi_dp_instance_size(const struct efi_device_path *dp)
{
	efi_uintn_t sz = 0;

	if (!dp || dp->type == DEVICE_PATH_TYPE_END)
		return 0;
	while (dp) {
		sz += dp->length;
		dp = efi_dp_next(dp);
	}

	return sz;
}

/**
 * efi_dp_size() - Get size of multi-instance device path excluding end node
 *
 * Calculate the total size of the entire device path structure, traversing
 * through all instances, up to but not including the final
 * END_ENTIRE_DEVICE_PATH node.
 *
 * @dp: Pointer to the device path.
 * Return: Total size in bytes of all nodes in the device path (excluding the
 * final EFI_DP_END node), or 0 if dp is NULL.
 */
efi_uintn_t efi_dp_size(const struct efi_device_path *dp)
{
	const struct efi_device_path *p = dp;

	if (!p)
		return 0;
	while (p->type != DEVICE_PATH_TYPE_END ||
	       p->sub_type != DEVICE_PATH_SUB_TYPE_END)
		p = (void *)p + p->length;

	return (void *)p - (void *)dp;
}

/**
 * efi_dp_dup() - Copy multi-instance device path
 *
 * Duplicate the given device path, including its end node(s).
 * The caller is responsible for freeing the allocated memory (e.g.,
 * using efi_free()).
 *
 * @dp: Pointer to the device path to duplicate.
 * Return: Pointer to the newly allocated and copied device path, or NULL on
 * allocation failure or if dp is NULL.
 */
struct efi_device_path *efi_dp_dup(const struct efi_device_path *dp)
{
	struct efi_device_path *ndp;
	size_t sz = efi_dp_size(dp) + sizeof(END);

	if (!dp)
		return NULL;

	ndp = dp_alloc(sz);
	if (!ndp)
		return NULL;
	memcpy(ndp, dp, sz);

	return ndp;
}

/**
 * efi_dp_concat() - Concatenate two device paths and terminate the result
 *
 * @dp1:        First device path
 * @dp2:        Second device path
 * @split_end_node:
 * - 0 to concatenate (dp1 is assumed not to have an end node or it's ignored,
 *   dp2 is appended, then one EFI_DP_END node)
 * - 1 to concatenate with end node added as separator (dp1, END_THIS_INSTANCE,
 *   dp2, END_ENTIRE)
 *
 * Size of dp1 excluding last end node to concatenate with end node as
 * separator in case dp1 contains an end node (dp1 (partial), END_THIS_INSTANCE,
 * dp2, END_ENTIRE)
 *
 * Return:
 * concatenated device path or NULL. Caller must free the returned value.
 */
struct
efi_device_path *efi_dp_concat(const struct efi_device_path *dp1,
			       const struct efi_device_path *dp2,
			       size_t split_end_node)
{
	struct efi_device_path *ret;
	size_t end_size;

	if (!dp1 && !dp2) {
		/* return an end node */
		ret = efi_dp_dup(&END);
	} else if (!dp1) {
		ret = efi_dp_dup(dp2);
	} else if (!dp2) {
		ret = efi_dp_dup(dp1);
	} else {
		/* both dp1 and dp2 are non-null */
		size_t sz1;
		size_t sz2 = efi_dp_size(dp2);
		void *p;

		if (split_end_node < sizeof(struct efi_device_path))
			sz1 = efi_dp_size(dp1);
		else
			sz1 = split_end_node;

		if (split_end_node)
			end_size = 2 * sizeof(END);
		else
			end_size = sizeof(END);
		p = dp_alloc(sz1 + sz2 + end_size);
		if (!p)
			return NULL;
		ret = p;
		memcpy(p, dp1, sz1);
		p += sz1;

		if (split_end_node) {
			memcpy(p, &END, sizeof(END));
			p += sizeof(END);
		}

		/* the end node of the second device path has to be retained */
		memcpy(p, dp2, sz2);
		p += sz2;
		memcpy(p, &END, sizeof(END));
	}

	return ret;
}

/**
 * efi_dp_append_node() - Append a single node to a device path
 *
 * Create a new device path by appending a given node to an existing
 * device path.
 * If the original device path @dp is NULL, a new path is created
 * with the given @node followed by an EFI_DP_END node.
 * If the @node is NULL and @dp is not NULL, the original path @dp is
 * duplicated.
 * If both @dp and @node are NULL, a path with only an EFI_DP_END node is
 * returned.
 * The caller must free the returned path (e.g., using efi_free()).
 *
 * @dp:   Original device path (can be NULL).
 * @node: Node to append (can be NULL).
 * Return: New device path with the node appended, or NULL on allocation
 * failure.
 */
struct efi_device_path *efi_dp_append_node(const struct efi_device_path *dp,
					   const struct efi_device_path *node)
{
	struct efi_device_path *ret;

	if (!node && !dp) {
		ret = efi_dp_dup(&END);
	} else if (!node) {
		ret = efi_dp_dup(dp);
	} else if (!dp) {
		size_t sz = node->length;
		void *p = dp_alloc(sz + sizeof(END));
		if (!p)
			return NULL;
		memcpy(p, node, sz);
		memcpy(p + sz, &END, sizeof(END));
		ret = p;
	} else {
		/* both dp and node are non-null */
		size_t sz = efi_dp_size(dp);
		void *p = dp_alloc(sz + node->length + sizeof(END));
		if (!p)
			return NULL;
		memcpy(p, dp, sz);
		memcpy(p + sz, node, node->length);
		memcpy(p + sz + node->length, &END, sizeof(END));
		ret = p;
	}

	return ret;
}

/**
 * efi_dp_split_file_path() - split of relative file path from device path
 *
 * Given a device path indicating a file on a device, separate the device
 * path in two: the device path of the actual device and the file path
 * relative to this device.
 *
 * @full_path:      device path including device and file path
 * @device_path:    path of the device
 * @file_path:      relative path of the file or NULL if there is none
 * Return:      status code
 */
efi_status_t efi_dp_split_file_path(struct efi_device_path *full_path,
				    struct efi_device_path **device_path,
				    struct efi_device_path **file_path)
{
	struct efi_device_path *p, *dp, *fp = NULL;

	*device_path = NULL;
	*file_path = NULL;
	dp = efi_dp_dup(full_path);
	if (!dp)
		return EFI_OUT_OF_RESOURCES;
	p = dp;
	while (!EFI_DP_TYPE(p, MEDIA_DEVICE, FILE_PATH)) {
		p = efi_dp_next(p);
		if (!p)
			goto out;
	}
	fp = efi_dp_dup(p);
	if (!fp)
		return EFI_OUT_OF_RESOURCES;
	p->type = DEVICE_PATH_TYPE_END;
	p->sub_type = DEVICE_PATH_SUB_TYPE_END;
	p->length = sizeof(*p);

out:
	*device_path = dp;
	*file_path = fp;
	return EFI_SUCCESS;
}

static void efi_dp_end(struct efi_device_path *dp)
{
	*dp = END;
}

/**
 * efi_dp_from_mem() - Construct a device-path for a memory-mapped region
 *
 * Create an EFI device path representing a specific memory region, defined
 * by its type, start address, and size.
 * The caller is responsible for freeing the allocated memory (e.g.,
 * using efi_free()).
 *
 * @memory_type: EFI memory type (e.g., EFI_RESERVED_MEMORY_TYPE).
 * @start_address: Starting address of the memory region.
 * @size: Size of the memory region in bytes.
 * Return: Pointer to the new memory device path, or NULL on allocation failure
 */
struct efi_device_path *efi_dp_from_mem(uint32_t memory_type,
					uint64_t start_address,
					uint64_t end_address)
{
	struct efi_device_path_memory *mdp;
	void *buf, *start;

	start = buf = dp_alloc(sizeof(*mdp) + sizeof(END));
	if (!buf)
		return NULL;

	mdp = buf;
	mdp->header.type = DEVICE_PATH_TYPE_HARDWARE_DEVICE;
	mdp->header.sub_type = DEVICE_PATH_SUB_TYPE_MEMORY;
	mdp->header.length = sizeof(*mdp);
	mdp->memory_type = memory_type;
	mdp->starting_address = start_address;
	mdp->ending_address = end_address;

	efi_dp_end(&mdp[1].header);

	return start;
}

/**
 * efi_dp_create_device_node() - Create a new device path node
 *
 * Allocate and initialise the header of a new EFI device path node with the
 * given type, sub-type, and length. The content of the node beyond the basic
 * efi_device_path header is zeroed by efi_alloc.
 *
 * @type:     Device path type.
 * @sub_type: Device path sub-type.
 * @length:   Length of the node (must be >= sizeof(struct efi_device_path)).
 * Return: Pointer to the new device path node, or NULL on allocation failure
 * or if length is invalid.
 */
struct efi_device_path *efi_dp_create_device_node(const u8 type,
						  const u8 sub_type,
						  const u16 length)
{
	struct efi_device_path *ret;

	if (length < sizeof(struct efi_device_path))
		return NULL;

	ret = dp_alloc(length);
	if (!ret)
		return ret;
	ret->type = type;
	ret->sub_type = sub_type;
	ret->length = length;
	return ret;
}

/**
 * efi_dp_append_instance() - Append a device path instance to another
 *
 * Concatenate two device paths, treating the second path (@dpi) as a new
 * instance appended to the first path (@dp). An END_THIS_INSTANCE node is
 * inserted between @dp and @dpi if @dp is not NULL.
 * If @dp is NULL, @dpi is duplicated (and terminated appropriately).
 * @dpi must not be NULL.
 * The caller is responsible for freeing the returned path (e.g., using
 * efi_free()).
 *
 * @dp:  The base device path. If NULL, @dpi is duplicated.
 * @dpi: The device path instance to append. Must not be NULL.
 * Return: A new device path with @dpi appended as a new instance, or NULL on
 * error  (e.g. allocation failure, @dpi is NULL).
 */
struct efi_device_path *efi_dp_append_instance(
		const struct efi_device_path *dp,
		const struct efi_device_path *dpi)
{
	size_t sz, szi;
	struct efi_device_path *p, *ret;

	if (!dpi)
		return NULL;
	if (!dp)
		return efi_dp_dup(dpi);
	sz = efi_dp_size(dp);
	szi = efi_dp_instance_size(dpi);
	p = dp_alloc(sz + szi + 2 * sizeof(END));
	if (!p)
		return NULL;
	ret = p;
	memcpy(p, dp, sz + sizeof(END));
	p = (void *)p + sz;
	p->sub_type = DEVICE_PATH_SUB_TYPE_INSTANCE_END;
	p = (void *)p + sizeof(END);
	memcpy(p, dpi, szi);
	p = (void *)p + szi;
	memcpy(p, &END, sizeof(END));
	return ret;
}

/**
 * efi_dp_get_next_instance() - Extract the next dp instance
 *
 * Given a pointer to a pointer to a device path (@dp), this function extracts
 * the first instance from the path. It allocates a new path for this extracted
 * instance (including its instance-specific EFI_DP_END node). The input pointer
 * (*@dp) is then updated to point to the start of the next instance in the
 * original path, or set to NULL if no more instances remain.
 * The caller is responsible for freeing the returned instance path (e.g.,
 * using efi_free()).
 *
 * @dp:  On input, a pointer to a pointer to the multi-instance device path.
 * On output, *@dp is updated to point to the start of the next instance,
 * or NULL if no more instances.
 * @size: Optional pointer to an efi_uintn_t variable that will receive the size
 * of the extracted instance path (including its EFI_DP_END node).
 * Return: Pointer to a newly allocated device path for the extracted instance,
 * or NULL if no instance could be extracted or an error occurred (e.g.,
 * allocation failure).
 */
struct efi_device_path *efi_dp_get_next_instance(struct efi_device_path **dp,
						 efi_uintn_t *size)
{
	size_t sz;
	struct efi_device_path *p;

	if (size)
		*size = 0;
	if (!dp || !*dp)
		return NULL;
	sz = efi_dp_instance_size(*dp);
	p = dp_alloc(sz + sizeof(END));
	if (!p)
		return NULL;
	memcpy(p, *dp, sz + sizeof(END));
	*dp = (void *)*dp + sz;
	if ((*dp)->sub_type == DEVICE_PATH_SUB_TYPE_INSTANCE_END)
		*dp = (void *)*dp + sizeof(END);
	else
		*dp = NULL;
	if (size)
		*size = sz + sizeof(END);
	return p;
}

/**
 * efi_dp_is_multi_instance() - Check if a device path is multi-instance
 *
 * Traverse the device path to its end. It is considered multi-instance if an
 * END_THIS_INSTANCE_DEVICE_PATH node (type DEVICE_PATH_TYPE_END, sub-type
 * DEVICE_PATH_SUB_TYPE_INSTANCE_END) is encountered before the final
 * END_ENTIRE_DEVICE_PATH node.
 *
 * @dp: The device path to check.
 * Return: True if the device path contains multiple instances, false otherwise
 * (including if @dp is NULL).
 */
bool efi_dp_is_multi_instance(const struct efi_device_path *dp)
{
	const struct efi_device_path *p = dp;

	if (!p)
		return false;
	while (p->type != DEVICE_PATH_TYPE_END)
		p = (void *)p + p->length;
	return p->sub_type == DEVICE_PATH_SUB_TYPE_INSTANCE_END;
}

/**
 * path_to_uefi() - convert UTF-8 path to an UEFI style path
 *
 * Convert UTF-8 path to a UEFI style path (i.e. with backslashes as path
 * separators and UTF-16).
 *
 * @src:	source buffer
 * @uefi:	target buffer, possibly unaligned
 * Return: number of wide characters, including NUL terminator
 */
static size_t path_to_uefi(wchar_t *uefi, const char *src)
{
	wchar_t *pos = uefi;

	while (*src) {
		s32 code = utf8_get(&src);

		if (code < 0)
			code = '?';
		else if (code == '/')
			code = '\\';
		utf16_put(code, &pos);
	}
	*pos++ = 0;

	return pos - uefi;
}

/**
 * efi_fp_size() - Calculate file path EFI device node size
 *
 * Calculate the final size the node would take, so space can be allocated.
 *
 * @path:	file path inside device
 * Return: Size needed for the efi_device_path_file_path or -EOVERFLOW on error
 */
static ssize_t efi_fp_size(const char *path)
{
	size_t fpsize;

	fpsize = struct_size_t(struct efi_device_path_file_path, path_name,
			       utf8_utf16_strlen(path) + 1);
	if (fpsize > U16_MAX)
		return -EOVERFLOW;

	return fpsize;
}

/**
 * efi_fp_fill() - Fill file path EFI device node
 *
 * Initialize the node and fill out its path including NUL termination.
 *
 * @fp:		efi_device_path_file_path to be filled
 * @path:	file path inside device
 * Return: Pointer to next node after advancing by fp + copied path
 */
static struct efi_device_path *efi_fp_fill(struct efi_device_path_file_path *fp, const char *path)
{
	fp->header.type = DEVICE_PATH_TYPE_MEDIA_DEVICE;
	fp->header.sub_type = DEVICE_PATH_SUB_TYPE_FILE_PATH;
	fp->header.length = struct_size(fp, path_name, path_to_uefi(fp->path_name, path));

	return efi_dp_next(&fp->header);
}

/**
 * efi_bbs_size() - Calculate BiosBootSpecification EFI device node size
 *
 * Calculate the final size node would take, so space can be allocated.
 * To simplify things for users within this file that will want to prepend
 * and append strings, this is directly handled in the API.
 *
 * @prefix:	prefix to prepend in front of path, may be NULL
 * @path:	The string argument for the BBS node
 * @suffix:	suffix to append after path, may be NULL
 * Return: Size needed for the efi_device_path_bbs_bbs or -EOVERFLOW on error
 */
static ssize_t efi_bbs_size(const char *prefix, const char *path, const char *suffix)
{
	size_t fpsize;
	size_t len = 0;

	len += strlen(prefix ?: "");
	len += strlen(path);
	len += strlen(suffix ?: "");
	len += 1;

	fpsize = struct_size_t(struct efi_device_path_bbs_bbs, String, len);
	if (fpsize > U16_MAX)
		return -EOVERFLOW;

	return fpsize;
}

/**
 * efi_bbs_fill() - Fill BiosBootSpecification EFI device node
 *
 * Initialize the node and fill out its path including NUL termination.
 * To simplify things for users within this file that will want to prepend
 * and append strings, this is directly handled in the API.
 *
 * @bp:		efi_device_path_file_path to be filled
 * @prefix:	prefix to prepend in front of path, may be NULL
 * @path:	The string argument for the BBS node
 * @suffix:	suffix to append after path, may be NULL
 * Return: Pointer to next node after advancing by bp + copied path
 */
static struct efi_device_path *efi_bbs_fill(struct efi_device_path_bbs_bbs *bp,
					    const char *prefix,
					    const char *path,
					    const char *suffix)
{
	s8 *end = bp->String;

	bp->header.type = DEVICE_PATH_TYPE_BBS_DEVICE;
	bp->header.sub_type = 0x01;
	bp->device_type = BBS_TYPE_DEV;
	bp->status_flag = 0;

	end = stpcpy(end, prefix ?: "");
	end = stpcpy(end, path);
	end = stpcpy(end, suffix ?: "");
	end++;

	bp->header.length = struct_size(bp, String, end - bp->String);

	return efi_dp_next(&bp->header);
}

/**
 * __efi_dp_cdev_table_part() - Fill hard drive EFI device node from cdev
 *
 * Generates an EFI device node from a barebox cdev partition. The result
 * looks like this:
 *
 *   HD(Part2,Sig7963f4e7-618a-3b49-ac6b-10821a9385cc).
 *
 * @hddp:	The EFI device path node
 * @cdev:	The partition's cdev
 * Return: Pointer to next node after advancing by sizeof(*hddp)
 */
static struct efi_device_path *
__efi_dp_cdev_table_part(struct efi_device_path_hard_drive_path *hddp,
			 const struct cdev *cdev)
{
	struct cdev *master = cdev->master;
	struct block_device *blk = cdev_get_block_device(master);
	int ret;

	if (!(master->flags & (DEVFS_IS_MBR_PARTITIONED | DEVFS_IS_GPT_PARTITIONED))) {
		pr_debug("%s: cdev outside partition table\n", cdev->name);
		return NULL;
	}

	/*
	 * UEFI v2.10 documents Signature as u32 for MBR and GUID for GPT.
	 * As signature for partition on MBR ist ${DISKSIG}-${PARTID}, this
	 * would imply that GUID is also DISKUUID and not PARTUUID. EDK-II
	 * and U-Boot use partition GUID though, which is kind of redundant
	 * as there's already a partition number... We follow suit, even
	 * if doesn't make perfect sense.
	 */
	if (cdev_is_gpt_partitioned(master)) {
		guid_t sig;

		ret = guid_parse(cdev->partuuid, &sig);
		if (ret) {
			pr_debug("%s: failed to parse GPT uuid\n", cdev->name);
			return NULL;
		}

		hddp->mbr_type = MBR_TYPE_EFI_PARTITION_TABLE_HEADER;
		hddp->signature_type = SIGNATURE_TYPE_GUID;
		export_guid(hddp->signature, &sig);
	} else if (cdev_is_mbr_partitioned(master)) {
		u32 sig;

		// TODO Extended partitions should be children of the
		// extended raw partition, which should've an index of 0

		ret = kstrtou32(master->diskuuid, 16, &sig);
		if (ret) {
			pr_debug("%s: failed to parse MBR signature\n", cdev->name);
			return NULL;
		}

		hddp->mbr_type = MBR_TYPE_PCAT;
		hddp->signature_type = SIGNATURE_TYPE_MBR;
		put_unaligned_le32(sig, hddp->signature);
		memset(&hddp->signature[4], 0, 12);
	}

	hddp->header.type = DEVICE_PATH_TYPE_MEDIA_DEVICE;
	hddp->header.sub_type = DEVICE_PATH_SUB_TYPE_HARD_DRIVE_PATH;
	hddp->header.length = sizeof(*hddp);
	/* barebox starts counting partitions at 1 */
	hddp->partition_number = cdev->partition_table_index + 1;

	hddp->partition_start = cdev->offset >> blk->blockbits;
	hddp->partition_size = cdev->size >> blk->blockbits;

	return efi_dp_next(&hddp->header);
}

/**
 * efi_dp_from_cdev() - Generate EFI device node from any cdev
 *
 * A barebox cdev is represented as a BBS path, optionally followed
 * by a hard drive path specifying the partition.
 *
 * BBS is the BIOS Boot Specification, which seems long obsolete, but it lends
 * itself nicely to our purposes of wrapping barebox VFS paths. There's already
 * a FilePath device node sub type for media devices, but it's usually at the
 * end of the device path and buggy EFI payload may be not search beyond the
 * first file path they encounter. All other device path nodes are documented
 * as formatting their argument as hexadecimal with BBS being the only exception,
 * which takes a non-UTF-8 string. Our convention is:
 *
 *   virtioblk2 -> BBS(Dev,/dev/virtioblk2) FIXME
 *
 * @cdev:	A barebox cdev
 * @full:	return full path, including ROOT and END
 * Return: Pointer to efi_alloc()ated node or NULL on error
 */
struct efi_device_path *efi_dp_from_cdev(struct cdev *cdev, bool full)
{
	struct efi_device_path *dp = NULL;
	void *buf = NULL;
	struct cdev *cdevm = full ? cdev->master : NULL;
	ssize_t bbs_size;
	size_t dp_size = 0;

	bbs_size = efi_bbs_size("/dev/", cdev->name, NULL);
	if (bbs_size < 0)
		return NULL;

	if (full)
		dp_size += sizeof(ROOT) + sizeof(END);
	if (cdevm)
		dp_size += bbs_size;

	dp_size += max_t(size_t,
			 sizeof(struct efi_device_path_hard_drive_path), bbs_size);

	dp = buf = dp_alloc(dp_size);
	if (!dp)
		return NULL;

	if (full)
		buf = mempcpy(buf, &ROOT, sizeof(ROOT));

	if (cdevm)
		buf = efi_bbs_fill(buf, "/dev/", cdevm->name, NULL);

	if (cdev->flags & DEVFS_PARTITION_FROM_TABLE) {
		void *oldbuf = buf;

		buf = __efi_dp_cdev_table_part(buf, cdev);
		if (buf)
			goto out;

		buf = oldbuf;
	}

	buf = efi_bbs_fill(buf, "/dev/", cdev->name, NULL);

out:
	if (full)
		efi_dp_end(buf);
	return dp;
}

/**
 * efi_dp_from_fsdev() - Generate EFI device node from non-cdev-backed FS dev
 *
 * For file system devices with no backing cdev, this function returns
 * a BBS path wrapping the mount point with a trailing slash as to
 * differentiate from cdevs, which have no trailing /. Example:
 *
 *   tftpfs0 -> BBS(Dev,/mnt/tftp/)
 *
 * @fsdev:	File system device
 * Return: Pointer to efi_alloc()ated node or NULL on error
 */
static struct efi_device_path *efi_dp_from_fsdev(struct fs_device *fsdev)
{
	struct efi_device_path *dp;
	ssize_t bbs_size;
	const char *slash = "/";
	void *buf;

	if (*fsdev->path && fsdev->path[strlen(fsdev->path) - 1] == '/')
		slash = NULL;

	bbs_size = efi_bbs_size(NULL, fsdev->path, slash);
	if (bbs_size < 0)
		return NULL;

	dp = buf = dp_alloc(sizeof(ROOT) + bbs_size + sizeof(END));
	if (!dp)
		return NULL;

	buf = mempcpy(buf, &ROOT, sizeof(ROOT));
	buf = efi_bbs_fill(buf, NULL, fsdev->path, slash);
	efi_dp_end(buf);

	return dp;
}

/**
 * efi_dp_from_fsdev() - Generate EFI device node from barebox VFS path
 *
 * This function can map any barebox VFS path to an EFI device path.
 * It's not pretty and in future it's meant to be only the fallback,
 * with proper hierarchy, e.g.
 * MMIO -> PCI root -> USB controller -> USB Mass storage -> Block device,
 * taking its place.
 *
 * For now though, it's enough to allow Grub to find its config and to
 * represent arbitrary VFS paths.
 *
 * Examples (All start with VenHw(b9731de6-84a3-cc4a-aeab-82e828f362bb)):
 *
 *   /dev/hwrng0
 *	-> devfs0/hwrng0
 *   /mnt/virtioblk0.14/
 *      -> BBS(Dev,/dev/virtioblk0)/HD(Part15,Sig26e17b2d-45f0-4c67-9d42-0a5c4a9587fc)/\
 *   /mnt/virtioblk0.14/barebox.efivars
 *      ->
 *      BBS(Dev,/dev/virtioblk0)/HD(Part15,Sig26e17b2d-45f0-4c67-9d42-0a5c4a9587fc)/\barebox.efivars
 *   /dev/hwrng0
 *      -> BBS(Dev,/dev/hwrng0)
 *   /mnt/9p/
 *      -> BBS(Dev,/)/\mnt\9p
 *   /mnt/9p/fs0
 *      -> BBS(Dev,/mnt/9p/fs0/)/\
 *   /mnt/9p/fs0/
 *      -> BBS(Dev,/mnt/9p/fs0/)/\
 *
 * @dirfd:	directory fd to lookup a relative path in
 * @path:	relative or absolute VFS path
 * Return: Pointer to efi_alloc()ated node or NULL on error
 */
struct efi_device_path *efi_dp_from_file(int dirfd, const char *path)
{
	struct fs_device *fsdev;
	void *buf;
	ssize_t dp_size;
	struct efi_device_path *prefix_dp, *dp = NULL;
	struct cdev *cdev;
	char *filepath;

	cdev = cdev_by_name(devpath_to_name(path));
	if (cdev)
		return efi_dp_from_cdev(cdev, true);

	fsdev = resolve_fsdevice_path(dirfd, path, &filepath);
	if (!fsdev)
		return NULL;

	if (fsdev->cdev)
		prefix_dp = efi_dp_from_cdev(fsdev->cdev, true);
	else
		prefix_dp = efi_dp_from_fsdev(fsdev);

	if (!prefix_dp)
		goto out;

	/* both dp and node are non-null */
	dp_size = efi_dp_size(prefix_dp);
	buf = dp = dp_alloc(dp_size + efi_fp_size(filepath) + sizeof(END));
	if (!dp)
		goto out;

	buf = mempcpy(buf, prefix_dp, dp_size);
	buf = efi_fp_fill(buf, filepath);
	efi_dp_end(buf);

out:
	efi_free_pool(prefix_dp);
	free(filepath);

	return dp;
}

/**
 * efi_dp_from_file_tostr() - format barebox VFS path as EFI device path string
 *
 * Convert any barebox virtual file system path to a full UEFI device path
 * and then return its full string representation.
 *
 * @dirfd:	directory fd
 * @path:	barebox virtual file system path
 * Return: string representation of the UEFI device path to be freed with
 *         free() or NULL otherwise.
 */
char *efi_dp_from_file_tostr(int dirfd, const char *path)
{
	struct efi_device_path *dp;
	char *dpstr = NULL;

	dp = efi_dp_from_file(dirfd, path);
	if (dp)
		dpstr = device_path_to_str(dp, true);

	efi_free_pool(dp);

	return dpstr;
}
