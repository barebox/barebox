// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2018 Sascha Hauer <s.hauer@pengutronix.de>

#define pr_fmt(fmt) "booti: " fmt

#include <common.h>
#include <filetype.h>
#include <memory.h>
#include <bootm.h>
#include <linux/sizes.h>

static unsigned long get_kernel_address(unsigned long os_address,
					unsigned long text_offset,
					resource_size_t *end)
{
	resource_size_t start;
	struct resource *sdram, gap;
	int ret;

	if (!UIMAGE_IS_ADDRESS_VALID(os_address)) {
		ret = memory_bank_first_find_space(&start, end);
		if (ret)
			return UIMAGE_INVALID_ADDRESS;

		return ALIGN(start, SZ_2M) + text_offset;
	}

	sdram = memory_bank_lookup_region(os_address, &gap);
	if (sdram != &gap)
		return UIMAGE_INVALID_ADDRESS;

	*end = gap.end;

	if (os_address >= text_offset && IS_ALIGNED(os_address - text_offset, SZ_2M))
		return os_address;

	return ALIGN(os_address, SZ_2M) + text_offset;
}

void *booti_load_image(struct image_data *data, phys_addr_t *oftree)
{
	const void *kernel_header = data->os_header;
	const struct resource *os_res;
	unsigned long text_offset, image_size, kernel;
	unsigned long image_end;
	resource_size_t end;
	void *fdt;

	print_hex_dump_bytes("header ", DUMP_PREFIX_OFFSET, kernel_header, 80);

	if ((IS_ENABLED(CONFIG_RISCV) && !is_riscv_linux_bootimage(kernel_header)) ||
	    (IS_ENABLED(CONFIG_ARM64) && !is_arm64_linux_bootimage(kernel_header))) {
		pr_err("Unexpected magic at offset 0x38!\n");
		return ERR_PTR(-EINVAL);
	}

	text_offset = le64_to_cpup(kernel_header + 8);
	image_size = le64_to_cpup(kernel_header + 16);

	kernel = get_kernel_address(data->os_address, text_offset, &end);

	pr_debug("Kernel (size: %lx) to be loaded into %lx+%llx\n",
		 image_size, kernel, end);

	if (kernel == UIMAGE_INVALID_ADDRESS)
		return ERR_PTR(-ENOENT);
	if (kernel + image_size - 1 > end)
		return ERR_PTR(-ENOSPC);

	os_res = bootm_load_os(data, kernel, end);
	if (IS_ERR(os_res))
		return ERR_CAST(os_res);

	image_end = PAGE_ALIGN(kernel + image_size);

	if (oftree) {
		unsigned long devicetree;
		const struct resource *initrd_res, *dt_res;

		initrd_res = bootm_load_initrd(data, image_end, end);
		if (IS_ERR(initrd_res)) {
			return ERR_CAST(initrd_res);
		} else if (initrd_res) {
			image_end += resource_size(data->initrd_res);
			image_end = PAGE_ALIGN(image_end);
		}

		devicetree = image_end;

		fdt = bootm_get_devicetree(data);
		if (IS_ERR(fdt))
			return fdt;
		if (!fdt) {
			if (initrd_res)
				pr_warn("initrd discarded due to missing devicetree.\n");
			goto out;
		}

		dt_res = bootm_load_devicetree(data, fdt, devicetree, end);
		free(fdt);

		if (IS_ERR(dt_res))
			return ERR_CAST(dt_res);

		*oftree = devicetree;
	}

out:
	printf("Loaded kernel to 0x%08lx", kernel);
	if (oftree && *oftree)
		printf(", devicetree at %pa", oftree);
	printf("\n");

	return (void *)kernel;
}
