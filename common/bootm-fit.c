// SPDX-License-Identifier: GPL-2.0-or-later

#include <bootm.h>
#include <image-fit.h>
#include <bootm-fit.h>
#include <memory.h>
#include <string.h>
#include <zero_page.h>
#include <filetype.h>
#include <fs.h>

static struct loadable *loadable_from_fit(struct fit_handle *fit,
					  void *config,
					  const char *image_name,
					  int index,
					  enum loadable_type type);

/*
 * loadable_from_fit_os() - create OS loadable from FIT
 * @data:		image data context
 * @fit:		handle of FIT image
 * @config:		config to look up kernel in
 *
 * This creates a loadable for the OS.
 */
static void loadable_from_fit_os(struct image_data *data,
				 struct fit_handle *fit,
				 void *config)
{
	loadable_release(&data->os);
	data->os = loadable_from_fit(fit, config, "kernel", 0, LOADABLE_KERNEL);
}

/*
 * loadable_from_fit_initrd() - create initrd loadable from FIT
 * @data:		image data context
 * @fit:		handle of FIT image
 * @config:		config to look up kernel in
 *
 * This creates loadables for all initial ram disks in the config and chains them.
 *
 * Return: true if initrd booting is supported and a ramdisk exists or
 *         false otherwise.
 */
static bool loadable_from_fit_initrd(struct image_data *data,
				struct fit_handle *fit,
				void *config)
{
	int nramdisks;

	if (!IS_ENABLED(CONFIG_BOOTM_INITRD))
		return false;

	nramdisks = fit_count_images(fit, config, "ramdisk");
	if (nramdisks < 0)
		return false;

	loadable_release(&data->initrd);

	for (int i = 0; i < nramdisks; i++)
		loadable_chain(&data->initrd, loadable_from_fit(fit, config, "ramdisk",
								i, LOADABLE_INITRD));

	return nramdisks > 0;
}

/*
 * loadable_from_fit_oftree() - create devicetree loadable from FIT
 * @data:		image data context
 * @fit:		handle of FIT image
 * @config:		config to look up kernel in
 *
 * This creates a loadable for the first fdt in the config.
 *
 * Return: true if a FDT exists or
 *         false otherwise.
 */
static bool loadable_from_fit_oftree(struct image_data *data,
				struct fit_handle *fit,
				void *config)
{
	if (!fit_has_image(fit, config, "fdt"))
		return false;

	loadable_release(&data->oftree);
	data->oftree = loadable_from_fit(fit, config, "fdt", 0, LOADABLE_FDT);
	return true;
}

/*
 * loadable_from_fit_tee() - create tee loadable from FIT
 * @data:		image data context
 * @fit:		handle of FIT image
 * @config:		config to look up kernel in
 *
 * This creates a loadable for the first trusted execution environment
 * in the config.
 *
 * Return: true if a TEE exists or
 *         false otherwise.
 */
static bool loadable_from_fit_tee(struct image_data *data,
				struct fit_handle *fit,
				void *config)
{
	if (!fit_has_image(fit, config, "tee"))
		return false;

	loadable_release(&data->tee);
	data->tee = loadable_from_fit(fit, config, "tee", 0, LOADABLE_TEE);
	return true;
}

static bool bootm_fit_config_valid(struct fit_handle *fit,
				   struct device_node *config)
{
	/*
	 * Consider only FIT configurations which do provide a loadable kernel
	 * image.
	 */
	return !!fit_has_image(fit, config, "kernel");
}

static enum filetype bootm_fit_update_os_header(struct image_data *data)
{
	size_t size;
	const void *header;
	enum filetype os_type;

	header = loadable_view(data->os, &size);
	if (IS_ERR(header))
		return filetype_unknown;

	if (size >= PAGE_SIZE)
		os_type = file_detect_type(header, size);
	else
		os_type = filetype_unknown;

	free(data->os_header);
	data->os_header = xmemdup(header, min_t(size_t, size, PAGE_SIZE));

	loadable_view_free(data->os, header, size);

	return os_type;
}

int bootm_open_fit(struct image_data *data, bool override)
{
	struct fit_handle *fit;
	void *fit_config;
	int ret;

	fit = fit_open(data->os_file, data->verbose, data->verify);
	if (IS_ERR(fit)) {
		pr_err("Loading FIT image %s failed with: %pe\n", data->os_file, fit);
		return PTR_ERR(fit);
	}

	fit_config = fit_open_configuration(fit, data->os_part, bootm_fit_config_valid);
	if (IS_ERR(fit_config)) {
		pr_err("Cannot open FIT image configuration '%s'\n",
		       data->os_part ?: "default");
		ret = PTR_ERR(fit_config);
		goto err;
	}

	loadable_from_fit_os(data, fit, fit_config);
	if (override)
		data->is_override.os = true;
	if (loadable_from_fit_initrd(data, fit, fit_config) && override)
		data->is_override.initrd = true;
	if (loadable_from_fit_oftree(data, fit, fit_config) && override)
		data->is_override.oftree = true;
	loadable_from_fit_tee(data, fit, fit_config);

	data->kernel_type = bootm_fit_update_os_header(data);

	if (data->os_address == UIMAGE_SOME_ADDRESS) {
		ret = fit_get_image_address(fit, fit_config, "kernel",
					    "load", &data->os_address);
		if (!ret)
			pr_info("Load address from FIT '%s': 0x%lx\n",
				"kernel", data->os_address);
		/* Note: Error case uses default value. */
	}
	if (data->os_entry == UIMAGE_SOME_ADDRESS) {
		unsigned long entry;
		ret = fit_get_image_address(fit, fit_config, "kernel",
					    "entry", &entry);
		if (!ret) {
			data->os_entry = entry - data->os_address;
			pr_info("Entry address from FIT '%s': 0x%lx\n",
				"kernel", entry);
		}
		/* Note: Error case uses default value. */
	}

	/* Each loadable now holds a reference to the FIT, so close our original
	 * reference, so the FIT is completely reclaimed if bootm fails.
	 */
	fit_close(fit);

	return 0;
err:
	fit_close(fit);
	return ret;
}

/* === Loadable implementation for FIT images === */

struct fit_loadable_priv {
	struct fit_handle *fit;
	struct device_node *config;
	const char *image_name;
	int index;
};

static int fit_loadable_get_info(struct loadable *l, struct loadable_info *info)
{
	struct fit_loadable_priv *priv = l->priv;
	const void *data;
	unsigned long size;
	int ret;

	/* Open image to get size */
	ret = fit_open_image(priv->fit, priv->config, priv->image_name,
			     priv->index, &data, &size);
	if (ret)
		return ret;

	/* TODO: This will trigger an uncompression currently.. */
	info->final_size = size;

	return 0;
}

static const void *fit_loadable_mmap(struct loadable *l, size_t *size)
{
	struct fit_loadable_priv *priv = l->priv;
	const void *data;
	unsigned long image_size;
	int ret;

	ret = fit_open_image(priv->fit, priv->config, priv->image_name,
			     priv->index, &data, &image_size);
	if (ret)
		return MAP_FAILED;

	*size = image_size;
	return data;
}

/**
 * fit_loadable_extract_into_buf - load FIT image data to target address
 * @l: loadable representing FIT image component
 * @load_addr: physical address to load data to
 * @buf_size: size of buffer at load_addr (0 = no limit check)
 * @offset: how many bytes to skip at the start of the uncompressed input
 * @flags: A bitmask of OR-ed LOADABLE_EXTRACT_ flags
 *
 * Commits the FIT image component to the specified memory address. This
 * involves:
 * 1. Opening the FIT image to get decompressed data
 * 2. Checking buffer size
 * 3. Copying data to target address
 *
 * The FIT data is already decompressed by fit_open_image(), so this just
 * performs a memcpy to the target address.
 *
 * Return: actual number of bytes written on success, negative errno on error
 *         -ENOSPC if buf_size is specified and too small
 *         -ENOMEM if failed to register SDRAM region
 */
static ssize_t fit_loadable_extract_into_buf(struct loadable *l, void *load_addr,
					     size_t buf_size, loff_t offset,
					     unsigned flags)
{
	struct fit_loadable_priv *priv = l->priv;
	const void *data;
	unsigned long size;
	int ret;

	/* TODO: optimize, so it decompresses directly to load address */

	/* Open image to get data */
	ret = fit_open_image(priv->fit, priv->config, priv->image_name,
			     priv->index, &data, &size);
	if (ret)
		return ret;

	/* Check if buffer is large enough */
	if (offset > size)
		return 0;

	if (!(flags & LOADABLE_EXTRACT_PARTIAL) && buf_size < size - offset)
		return -ENOSPC;

	size = min_t(size_t, size - offset, buf_size);

	if (unlikely(zero_page_contains((ulong)load_addr)))
		zero_page_memcpy(load_addr, data + offset, size);
	else
		memcpy(load_addr, data + offset, size);

	return size; /* Return actual bytes written */
}

static void fit_loadable_release(struct loadable *l)
{
	struct fit_loadable_priv *priv = l->priv;

	fit_close(priv->fit);
	free_const(priv->image_name);
	free(priv);
}

static const struct loadable_ops fit_loadable_ops = {
	.get_info = fit_loadable_get_info,
	.extract_into_buf = fit_loadable_extract_into_buf,
	.mmap = fit_loadable_mmap,
	.release = fit_loadable_release,
};

/**
 * loadable_from_fit - create a loadable from FIT image component
 * @fit: opened FIT image handle
 * @config: FIT configuration device node
 * @image_name: name of image in FIT (e.g., "kernel", "ramdisk", "fdt")
 * @index: index for multi-image types (e.g., ramdisk-0, ramdisk-1)
 * @type: type of loadable (LOADABLE_KERNEL, LOADABLE_INITRD, etc.)
 *
 * Creates a loadable structure that wraps access to a component within a
 * FIT image. The loadable uses the FIT handle to access decompressed image
 * data on demand during commit.
 *
 * The created loadable must be freed with loadable_release() when done.
 * The FIT handle itself is managed by the caller and must remain valid
 * until the loadable is released.
 *
 * Return: pointer to allocated loadable on success. Function never fails.
 */
static struct loadable *loadable_from_fit(struct fit_handle *fit,
					  void *config,
					  const char *image_name,
					  int index,
					  enum loadable_type type)
{
	struct loadable *l;
	struct fit_loadable_priv *priv;

	l = xzalloc(sizeof(*l));
	priv = xzalloc(sizeof(*priv));

	priv->fit = fit_open_handle(fit);
	priv->config = config;
	priv->image_name = xstrdup_const(image_name);
	priv->index = index;

	/* Create descriptive name */
	if (index)
		l->name = xasprintf("FIT(%s, %s/%s, %d)", fit->filename,
				    fit_config_get_name(fit, config),
				    image_name, index);
	else
		l->name = xasprintf("FIT(%s, %s/%s)", fit->filename,
				    fit_config_get_name(fit, config),
				    image_name);
	l->ops = &fit_loadable_ops;
	l->type = type;
	l->priv = priv;
	loadable_init(l);

	return l;
}
