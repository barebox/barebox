// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <asm/barebox-arm.h>
#include <filetype.h>
#include <linux/sizes.h>
#include <mach/socfpga/generic.h>
#include <pbl/bio.h>

static inline u32 barebox_arm_size(const char *head)
{
	return *(u32 *)(head + ARM_HEAD_SIZE_OFFSET);
}

const struct socfpga_barebox_part *barebox_part;

void agilex5_set_barebox_part(struct socfpga_barebox_part *part)
{
	barebox_part = part;
}

static ssize_t agilex5_bl33_read(struct pbl_bio *bio, void *bl33,
			     off_t offs, size_t max)
{
	const off_t step = ARM_HEAD_SIZE;
	u32 size = 0;
	int ret;

	ret = pbl_bio_read(bio, offs, bl33, step);
	if (ret < 0)
		return ret;
	if (is_barebox_arm_head(bl33))
		size = barebox_arm_size(bl33);
	if (size == 0) {
		pr_err("cannot find bl33 at offset 0x%08lx)\n", offs);
		return -EIO;
	}
	if (size > max) {
		pr_err("bl33 size (%u) larger than partition\n", size);
		return -EINVAL;
	}

	pr_info("bl33 found at 0x%08lx, size %u\n", offs, size);

	ret = pbl_bio_read(bio, offs + step, bl33 + step, size - step);
	if (ret < size - step) {
		pr_warn("failed to read bl33 from flash: %d\n", ret);
		return ret;
	}

	pr_debug("read bl33 to %p from %lx (%u bytes)\n", bl33, offs, size);

	return size;
}

int agilex5_xload_bl33(struct pbl_bio *bio, void *bl33)
{
	ssize_t size;

	if (!bio) {
		pr_warn("source device for SSBL is not configured\n");
		return -ENODEV;
	}
	if (!barebox_part) {
		pr_warn("source partition for SSBL is not configured\n");
		return -ENODEV;
	}
	size = agilex5_bl33_read(bio, bl33,
				 barebox_part->nor_offset,
				 barebox_part->nor_size);
	if (size < 0)
		return size;

	return 0;
}
