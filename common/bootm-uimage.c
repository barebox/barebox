// SPDX-License-Identifier: GPL-2.0-or-later

#include <bootm.h>
#include <bootm-uimage.h>
#include <linux/kstrtox.h>

static int uimage_part_num(const char *partname)
{
	if (!partname)
		return 0;
	return simple_strtoul(partname, NULL, 0);
}

static struct loadable *loadable_from_uimage(struct uimage_handle *uimage,
					     int part_num,
					     enum loadable_type type,
					     bool exclusive);

static void loadable_from_uimage_os(struct image_data *data)
{
	int num;

	num = uimage_part_num(data->os_part);

	loadable_release(&data->os);
	data->os = loadable_from_uimage(data->os_uimage, num, LOADABLE_KERNEL, true);
}

static int file_name_detect_type_is_uimage(const char *filename, const char *desc)
{
	enum filetype type;
	int ret;

	ret = file_name_detect_type(filename, &type);
	if (ret < 0) {
		pr_err("could not open %s \"%s\": %pe\n",
		       desc, filename, ERR_PTR(ret));
		return ret;
	}

	return type == filetype_uimage;
}

static int loadable_from_uimage_initrd(struct image_data *data)
{
	int ret, num;
	bool release = false;

	if (streq_ptr(data->os_file, data->initrd_file)) {
		data->initrd_uimage = data->os_uimage;
	} else {
		int is_uimage;

		is_uimage = file_name_detect_type_is_uimage(data->initrd_file, "initrd");
		if (is_uimage < 0)
			return is_uimage;
		if (!is_uimage) {
			loadable_release(&data->initrd);
			data->initrd = loadable_from_file(data->initrd_file,
							  LOADABLE_INITRD);
			return PTR_ERR_OR_ZERO(data->initrd);
		}

		data->initrd_uimage = uimage_open(data->initrd_file);
		if (!data->initrd_uimage)
			return -EINVAL;

		if (bootm_get_verify_mode() > BOOTM_VERIFY_NONE) {
			ret = uimage_verify(data->initrd_uimage);
			if (ret) {
				pr_err("Checking data crc failed with %pe\n",
					ERR_PTR(ret));
				uimage_close(data->initrd_uimage);
				data->initrd_uimage = NULL;
				return ret;
			}
		}
		uimage_print_contents(data->initrd_uimage);
		release = true;
	}

	num = uimage_part_num(data->initrd_part);

	loadable_release(&data->initrd);
	data->initrd = loadable_from_uimage(data->initrd_uimage, num,
					    LOADABLE_INITRD, release);

	return 0;
}

static int loadable_from_uimage_oftree(struct image_data *data)
{
	const char *oftree = data->oftree_file;
	int num;
	struct uimage_handle *of_handle;
	bool release = false;

	if (data->os_uimage && streq_ptr(data->os_file, oftree)) {
		of_handle = data->os_uimage;
	} else if (data->initrd_uimage && streq_ptr(data->initrd_file, oftree)) {
		of_handle = data->initrd_uimage;
	} else {
		int is_uimage;

		is_uimage = file_name_detect_type_is_uimage(data->oftree_file, "device tree");
		if (is_uimage < 0)
			return is_uimage;
		if (!is_uimage) {
			loadable_release(&data->oftree);
			data->oftree = loadable_from_file(data->oftree_file,
							  LOADABLE_FDT);
			return PTR_ERR_OR_ZERO(data->oftree);
		}

		of_handle = uimage_open(oftree);
		if (!of_handle)
			return -ENODEV;
		uimage_print_contents(of_handle);
		release = true;
	}

	num = uimage_part_num(data->oftree_part);
	pr_info("Loading devicetree from '%s'@%d\n", oftree, num);

	data->oftree_uimage = of_handle;

	loadable_release(&data->oftree);
	data->oftree = loadable_from_uimage(data->oftree_uimage, num, LOADABLE_FDT, release);

	return 0;
}

int bootm_open_uimage(struct image_data *data)
{
	int ret;

	data->os_uimage = uimage_open(data->os_file);
	if (!data->os_uimage)
		return -EINVAL;

	if (bootm_get_verify_mode() > BOOTM_VERIFY_NONE) {
		ret = uimage_verify(data->os_uimage);
		if (ret) {
			pr_err("Checking data crc failed with %pe\n",
					ERR_PTR(ret));
			goto err_close;
		}
	}

	uimage_print_contents(data->os_uimage);

	if (IH_ARCH == IH_ARCH_INVALID || data->os_uimage->header.ih_arch != IH_ARCH) {
		pr_err("Unsupported Architecture 0x%x\n",
		       data->os_uimage->header.ih_arch);
		ret = -EINVAL;
		goto err_close;
	}

	if (data->os_address == UIMAGE_SOME_ADDRESS)
		data->os_address = data->os_uimage->header.ih_load;

	return 0;

err_close:
	uimage_close(data->os_uimage);
	data->os_uimage = NULL;
	return ret;
}

/* === Loadable implementation for uImage === */

struct uimage_loadable_priv {
	struct uimage_handle *handle;
	int part_num;
	bool release_handle;
};

static int uimage_loadable_get_info(struct loadable *l, struct loadable_info *info)
{
	struct uimage_loadable_priv *priv = l->priv;
	struct uimage_handle *handle = priv->handle;
	ssize_t size;

	/* Get size from uImage header */
	size = uimage_get_size(handle, priv->part_num);
	if (size <= 0)
		return size < 0 ? size : -EINVAL;

	/*
	 * uimage_get_size() returns the compressed (stored) size.
	 * For compressed uImages, the decompressed size is unknown,
	 * so report LOADABLE_SIZE_UNKNOWN to let loadable_extract()
	 * fall through to the extract op which handles decompression.
	 */
	if (handle->header.ih_comp != IH_COMP_NONE)
		info->final_size = LOADABLE_SIZE_UNKNOWN;
	else
		info->final_size = size;

	return 0;
}

/**
 * uimage_loadable_extract_into_buf - load uImage data to target address
 * @l: loadable representing uImage component
 * @load_addr: physical address to load data to
 * @buf_size: size of buffer at load_addr (0 = no limit check)
 * @offset: how many bytes to skip at the start of the uncompressed input
 * @flags: A bitmask of OR-ed LOADABLE_EXTRACT_ flags
 *
 * Commits the uImage component to the specified memory address. This involves:
 * 1. Getting size information from loadable
 * 2. Checking buffer size if buf_size > 0
 * 3. Calling uimage_load_to_sdram() to decompress and load data
 * 4. Returning actual bytes written
 *
 * The uimage_load_to_sdram() function handles decompression (if needed),
 * memory allocation with request_sdram_region(), and copying data to the
 * target address.
 *
 * Return: actual number of bytes written on success, negative errno on error
 *         -ENOSPC if buf_size is specified and too small
 *         -ENOMEM if failed to load to SDRAM
 */
static ssize_t uimage_loadable_extract_into_buf(struct loadable *l, void *load_addr,
						size_t buf_size, loff_t offset,
						unsigned flags)
{
	struct uimage_loadable_priv *priv = l->priv;
	return uimage_load_into_fixed_buf(priv->handle, priv->part_num,
					  load_addr, buf_size, offset,
					  flags & LOADABLE_EXTRACT_PARTIAL);
}

static void *uimage_loadable_extract(struct loadable *l, size_t *size)
{
	struct uimage_loadable_priv *priv = l->priv;

	return uimage_load_to_buf(priv->handle, priv->part_num, size);
}

static void uimage_loadable_release(struct loadable *l)
{
	struct uimage_loadable_priv *priv = l->priv;

	if (priv->release_handle)
		uimage_close(priv->handle);
	free(priv);
}

static const struct loadable_ops uimage_loadable_ops = {
	.get_info = uimage_loadable_get_info,
	.extract_into_buf = uimage_loadable_extract_into_buf,
	.extract = uimage_loadable_extract,
	/* .mmap is implementable for uncompressed content if anyone cares enough */
	.release = uimage_loadable_release,
};

/**
 * loadable_from_uimage - create a loadable from uImage component
 * @uimage: opened uImage handle
 * @part_num: partition/part number within uImage (0 for single-part)
 * @type: type of loadable (LOADABLE_KERNEL, LOADABLE_INITRD, etc.)
 * @exclusive: whether the uimage will be owned (and released) by the loadable
 *
 * Creates a loadable structure that wraps access to a component within a
 * uImage. For multi-part uImages, part_num selects which part to load.
 * The loadable uses the uImage handle to access and potentially decompress
 * data on demand during commit.
 *
 * The created loadable must be freed with loadable_release() when done.
 * The uImage handle itself is managed by the caller and must remain valid
 * until the loadable is released.
 *
 * Return: pointer to allocated loadable on success, ERR_PTR() on error
 */
static struct loadable *loadable_from_uimage(struct uimage_handle *uimage,
					     int part_num,
					     enum loadable_type type,
					     bool exclusive)
{
	struct loadable *l;
	struct uimage_loadable_priv *priv;

	l = xzalloc(sizeof(*l));
	priv = xzalloc(sizeof(*priv));

	priv->handle = uimage;
	priv->release_handle = exclusive;
	priv->part_num = part_num;

	/* Create descriptive name */
	if (part_num > 0)
		l->name = xasprintf("uImage(%s, %d)", uimage->filename, part_num);
	else
		l->name = xasprintf("uImage(%s)", uimage->filename);

	l->type = type;
	l->ops = &uimage_loadable_ops;
	l->priv = priv;
	loadable_init(l);

	return l;
}

/**
 * bootm_collect_uimage_loadables - create loadables from opened uImage
 * @data: image data context with opened uImage handle
 *
 * Creates loadable structures for boot components from opened uImage handles.
 * This includes:
 * * Kernel from data->os_uimage (using data->os_part for multi-part selection)
 * * Initrd from data->initrd_files uImage if specified (opens it if needed)
 *
 * For initrd handling:
 * * If initrd_files matches os_file: uses same uImage handle (multi-part)
 * * Otherwise: opens separate uImage for initrd and verifies it
 *
 * Each loadable is added to data->loadables list and appropriate shortcuts
 * (data->kernel) are set. The loadables are not yet committed to memory - that
 * happens later during bootm_load_os/bootm_load_initrd.
 *
 * Note: FDT and TEE are not commonly used in uImage format and are not
 * collected here.
 *
 * Requires: data->os_uimage must be already opened by bootm_open_uimage()
 * Context: Called during boot preparation for uImage boots
 */
int bootm_collect_uimage_loadables(struct image_data *data)
{
	int ret;

	/* Create kernel loadable from opened uImage */
	if (data->os_uimage)
		loadable_from_uimage_os(data);

	if (IS_ENABLED(CONFIG_BOOTM_INITRD) && data->initrd_file) {
		ret = loadable_from_uimage_initrd(data);
		if (ret)
			return ret;
	}

	if (IS_ENABLED(CONFIG_BOOTM_OFTREE_UIMAGE) && data->oftree_file) {
		ret = loadable_from_uimage_oftree(data);
		if (ret)
			return ret;
	}

	return 0;
}
