// SPDX-License-Identifier: GPL-2.0-only

#include <bootm.h>
#include <bootm-overrides.h>
#include <bootm-fit.h>
#include <init.h>
#include <libfile.h>
#include <of.h>
#include <linux/pagemap.h>

/**
 * bootm_apply_image_override - apply bootm.image override
 * @data: image data context
 * @override: override path (may include @config suffix for FIT)
 *
 * Applies a bootm.image override according to these rules:
 * - If the override is not a FIT image, it replaces only data->os.
 * - If the override is a FIT image, it is re-opened via bootm_open_fit()
 *   using the default configuration or the @config suffix if given.
 *
 * Return: 0 on success, negative errno on failure
 */
static int bootm_apply_image_override(struct image_data *data,
				      const char *override)
{
	char *override_file, *override_part;
	void *override_header = NULL;
	int ret;

	ret = bootm_image_name_and_part(override, &override_file, &override_part);
	if (ret)
		return ret;

	/* Read header to detect file type */
	ret = file_read_and_detect_boot_image_type(override_file, &override_header);
	if (ret < 0) {
		free(override_file);
		return ret;
	}

	data->image_type = ret;

	/*
	 * bootm_image_name_and_part() returns os_part pointing inside the
	 * os_file allocation, so we must NULLify os_part before freeing
	 * os_file to avoid a use-after-free.
	 */
	data->os_part = NULL;
	free(data->os_file);
	data->os_file = override_file;
	if (override_part)
		data->os_part = override_part;

	/* Update the detected type for the handler */
	free(data->os_header);
	data->os_header = override_header;

	switch (data->image_type) {
	case filetype_fit:
		/* Restore, so bootm_open_fit looks for them in the new FIT */
		data->os_address = data->os_address_hint;
		data->os_entry = data->os_entry_hint;

		/* Re-open configuration and collect loadables */
		return bootm_open_fit(data, true);
	default:
		loadable_release(&data->os);
		data->os = loadable_from_file(override_file, LOADABLE_KERNEL);
		if (IS_ERR(data->os))
			return PTR_ERR(data->os);

		data->kernel_type = data->image_type;
		data->is_override.os = true;

		if (file_is_compressed_file(data->kernel_type))
			return bootm_open_os_compressed(data);

		return 0;
	}
}

static void report_active_override(const char *type_str, struct loadable *l)
{
	struct loadable *lc;
	char *buf = NULL;

	if (!l) {
		pr_notice("[bootm override active] removed %s\n", type_str);
		return;
	}

	buf = xrasprintf(buf, "[bootm override active] replacement %s: %s",
		      type_str, l->name);

	list_for_each_entry(lc, &l->chained_loadables, list)
		buf = xrasprintf(buf, ", %s", lc->name);

	pr_notice("%s\n", buf);
	free(buf);
}

static void report_active_overrides(const struct image_data *data)
{
	if (data->is_override.os)
		report_active_override("OS", data->os);
	if (data->is_override.initrd)
		report_active_override("initrd", data->initrd);
	if (data->is_override.oftree)
		report_active_override("FDT", data->oftree);
}

/**
 * bootm_apply_overrides - apply overrides
 * @data: image data context
 * @overrides: overrides to apply
 *
 * Applies bootm.image, bootm.initrd, and bootm.oftree overrides by translating
 * them into file-based loadables or FIT image replacements.
 *
 * Context: Called during boot preparation
 * Return: 0 on success, negative error code otherwise
 */
int bootm_apply_overrides(struct image_data *data,
			  const struct bootm_overrides *overrides)
{
	int ret;

	if (bootm_signed_images_are_forced())
		return 0;

	if (nonempty(overrides->os_file)) {
		ret = bootm_apply_image_override(data, overrides->os_file);
		if (ret)
			return ret;
	}

	if (IS_ENABLED(CONFIG_BOOTM_INITRD) && overrides->initrd_file) {
		/* loadables_from_files() will set data->initrd on empty initrd_file */
		int ret = loadables_from_files(&data->initrd, overrides->initrd_file, ":",
					       LOADABLE_INITRD);
		if (ret)
			return ret;
		data->is_override.initrd = true;
	}

	if (overrides->oftree_file) {
		int ret = loadables_from_files(&data->oftree,
					       overrides->oftree_file, ":",
					       LOADABLE_FDT);
		if (ret)
			return ret;
		data->is_override.oftree = true;
	}

	report_active_overrides(data);

	return 0;
}

static struct loadable *pending_oftree_overlays;

void bootm_set_pending_oftree_overlays(struct loadable *oftree)
{
	pending_oftree_overlays = oftree;
}

void bootm_clear_pending_oftree_overlays(void)
{
	pending_oftree_overlays = NULL;
}

static int bootm_override_overlay_fixup(struct device_node *root, void *ctx)
{
	struct loadable *ovl;

	if (!pending_oftree_overlays)
		return 0;

	list_for_each_entry(ovl, &pending_oftree_overlays->chained_loadables, list) {
		const void *dtbo;
		size_t size;
		int ret;

		dtbo = loadable_view(ovl, &size);
		if (IS_ERR(dtbo)) {
			pr_err("could not load overlay \"%s\": %pe\n",
			       ovl->name, dtbo);
			continue;
		}

		ret = of_overlay_apply_dtbo(root, dtbo);
		if (ret)
			pr_err("failed to apply overlay \"%s\": %pe\n",
			       ovl->name, ERR_PTR(ret));
		loadable_view_free(ovl, dtbo, size);
	}

	return 0;
}

static int bootm_register_override_overlay_fixup(void)
{
	if (!IS_ENABLED(CONFIG_OF_OVERLAY))
		return 0;
	return of_register_fixup(bootm_override_overlay_fixup, NULL);
}
of_populate_initcall(bootm_register_override_overlay_fixup);
