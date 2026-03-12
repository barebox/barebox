// SPDX-License-Identifier: GPL-2.0-only

#include <bootm.h>
#include <bootm-overrides.h>

/**
 * bootm_apply_overrides - apply overrides
 * @data: image data context
 * @overrides: overrides to apply
 *
 * Applies bootm.initrd and bootm.oftree overrides by translating
 * them into file-based loadables.
 *
 * Context: Called during boot preparation
 * Return: 0 on success, negative error code otherwise
 */
int bootm_apply_overrides(struct image_data *data,
			  const struct bootm_overrides *overrides)
{
	if (bootm_signed_images_are_forced())
		return 0;

	if (IS_ENABLED(CONFIG_BOOTM_INITRD) && overrides->initrd_file) {
		/* loadables_from_files() will set data->initrd on empty initrd_file */
		int ret = loadables_from_files(&data->initrd, overrides->initrd_file, ":",
					       LOADABLE_INITRD);
		if (ret)
			return ret;
		data->is_override.initrd = true;
	}

	if (overrides->oftree_file) {
		loadable_release(&data->oftree);

		/* Empty string means to mask the original FDT */
		if (nonempty(overrides->oftree_file)) {
			data->oftree = loadable_from_file(overrides->oftree_file,
							   LOADABLE_FDT);
			if (IS_ERR(data->oftree))
				return PTR_ERR(data->oftree);
		}
		data->is_override.oftree = true;
	}

	return 0;
}

