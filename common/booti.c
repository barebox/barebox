// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2018 Sascha Hauer <s.hauer@pengutronix.de>

#include <common.h>
#include <memory.h>
#include <bootm.h>
#include <linux/sizes.h>

void *booti_load_image(struct image_data *data, phys_addr_t *oftree)
{
	const void *kernel_header =
			data->os_fit ? data->fit_kernel : data->os_header;
	resource_size_t start, end;
	unsigned long text_offset, image_size, devicetree, kernel;
	unsigned long image_end;
	int ret;
	void *fdt;

	text_offset = le64_to_cpup(kernel_header + 8);
	image_size = le64_to_cpup(kernel_header + 16);

	ret = memory_bank_first_find_space(&start, &end);
	if (ret)
		return ERR_PTR(ret);

	kernel = ALIGN(start, SZ_2M) + text_offset;

	ret = bootm_load_os(data, kernel);
	if (ret)
		return ERR_PTR(ret);

	image_end = PAGE_ALIGN(kernel + image_size);

	if (oftree) {
		if (bootm_has_initrd(data)) {
			ret = bootm_load_initrd(data, image_end);
			if (ret)
				return ERR_PTR(ret);

			image_end += resource_size(data->initrd_res);
			image_end = PAGE_ALIGN(image_end);
		}

		devicetree = image_end;

		fdt = bootm_get_devicetree(data);
		if (IS_ERR(fdt))
			return fdt;

		ret = bootm_load_devicetree(data, fdt, devicetree);

		free(fdt);

		if (ret)
			return ERR_PTR(ret);

		*oftree = devicetree;
	}

	printf("Loaded kernel to 0x%08lx", kernel);
	if (oftree)
		printf(", devicetree at 0x%08lx", devicetree);
	printf("\n");

	return (void *)kernel;
}
