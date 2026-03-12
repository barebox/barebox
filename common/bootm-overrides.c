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

	/* TODO: As we haven't switched over everything to loadables yet,
	 * we need a special marker to mean override to empty.
	 * We do this via a 0-byte file (/dev/null) for now..
	 */

	if (overrides->initrd_file) {
		loadable_release(&data->initrd);

		/* Empty string means to mask the original initrd */
		if (nonempty(overrides->initrd_file))
			data->initrd = loadable_from_file(overrides->initrd_file,
							   LOADABLE_INITRD);
		else
			data->initrd = loadable_from_file("/dev/null",
							   LOADABLE_INITRD);
		if (IS_ERR(data->initrd))
			return PTR_ERR(data->initrd);
		data->is_override.initrd = true;
	}

	if (overrides->oftree_file) {
		loadable_release(&data->oftree);

		/* Empty string means to mask the original FDT */
		if (nonempty(overrides->oftree_file))
			data->oftree = loadable_from_file(overrides->oftree_file,
							   LOADABLE_FDT);
		else
			data->oftree = loadable_from_file("/dev/null",
							   LOADABLE_FDT);
		if (IS_ERR(data->oftree))
			return PTR_ERR(data->oftree);
		data->is_override.oftree = true;
	}

	return 0;
}
