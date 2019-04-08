// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <crc.h>
#include <soc/fsl/qe.h>

/*
 * qe_validate_firmware - Validate a QE firmware
 *
 * This function checks a QE firmware for validity. It checks the crc
 * and consistency of various fields. Returns 0 for success or a negative
 * error code otherwise.
 */
int qe_validate_firmware(const struct qe_firmware *firmware, int size)
{
	u32 crc;
	size_t calc_size;
	int i;
	const struct qe_header *hdr = &firmware->header;

	if (size < sizeof(*firmware)) {
		pr_err("firmware is too small\n");
		return -EINVAL;
	}

	if ((hdr->magic[0] != 'Q') || (hdr->magic[1] != 'E') ||
		(hdr->magic[2] != 'F')) {
		pr_err("Data at %p is not a QE firmware\n", firmware);
		return -EPERM;
	}

	if (size != be32_to_cpu(hdr->length)) {
		pr_err("Firmware size doesn't match size in header\n");
		return -EINVAL;
	}

	if (hdr->version != 1) {
		pr_err("Unsupported firmware version %u\n", hdr->version);
		return -EPERM;
	}

	calc_size = sizeof(*firmware) +
		    (firmware->count - 1) * sizeof(struct qe_microcode);

	if (size < calc_size)
		return -EINVAL;

	for (i = 0; i < firmware->count; i++)
		calc_size += sizeof(u32) *
			be32_to_cpu(firmware->microcode[i].count);

	if (size != calc_size + sizeof(u32)) {
		pr_err("Invalid length in firmware header\n");
		return -EPERM;
	}

	crc = be32_to_cpu(*(u32 *)((void *)firmware + calc_size));
	if (crc != (crc32_no_comp(0, (const void *)firmware, calc_size))) {
		pr_err("Firmware CRC is invalid\n");
		return -EIO;
	}

	return 0;
}
