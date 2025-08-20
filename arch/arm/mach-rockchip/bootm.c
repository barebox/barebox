// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "rockchip-bootm-barebox: " fmt

#include <bootm.h>
#include <common.h>
#include <init.h>
#include <memory.h>
#include <mach/rockchip/rockchip.h>

struct newidb_entry {
	uint32_t sector;
	uint32_t unknown_ffffffff;
	uint32_t unknown1;
	uint32_t image_number;
	unsigned char unknown2[8];
	unsigned char hash[64];
};

struct newidb {
	uint32_t magic;
	unsigned char unknown1[4];
	uint32_t n_files;
	uint32_t hashtype;
	unsigned char unknown2[8];
	unsigned char unknown3[8];
	unsigned char unknown4[88];
	struct newidb_entry entries[4];
	unsigned char unknown5[40];
	unsigned char unknown6[512];
	unsigned char unknown7[16];
	unsigned char unknown8[32];
	unsigned char unknown9[464];
	unsigned char hash[512];
};

#define SECTOR_SIZE 512

static int do_bootm_rkns_barebox_image(struct image_data *data)
{
	void (*barebox)(unsigned long x0, unsigned long x1, unsigned long x2,
			unsigned long x3);
	resource_size_t start, end;
	struct newidb *idb;
	int ret, i, n_files;
	void *buf;
	resource_size_t image_size;

	ret = memory_bank_first_find_space(&start, &end);
	if (ret)
		return ret;

	ret = bootm_load_os(data, start);
	if (ret)
		return ret;

	idb = (void *)data->os_res->start;
	buf = (void *)data->os_res->start;

	image_size = resource_size(data->os_res);

	if (image_size < SECTOR_SIZE)
		return -EINVAL;

	n_files = idb->n_files >> 16;
	if (n_files > 4)
		n_files = 4;

	if (data->verbose)
		printf("RKNS image contains %d binaries:\n", n_files);

	for (i = 0; i < n_files; i++) {
		struct newidb_entry *entry = &idb->entries[i];
		unsigned int entry_size = (entry->sector >> 16) * SECTOR_SIZE;
		unsigned int entry_start = (entry->sector & 0xffff) * SECTOR_SIZE;
		enum filetype filetype;

		filetype = file_detect_type(buf + entry_start, SECTOR_SIZE);

		if (entry_start + entry_size > image_size) {
			printf("image %d expands outside the image\n", i);
			continue;
		}

		if (data->verbose)
			printf("image %d: size: %d offset: %d type: %s\n",
				i, entry_size, entry_start,
				file_type_to_string(filetype));

		if (filetype == filetype_arm_barebox) {
			memmove(buf, buf + entry_start, image_size - entry_start);
			barebox = buf;
			goto found;
		}
	}

	return -EIO;

found:

	printf("Starting barebox image at 0x%p\n", barebox);

	shutdown_barebox();

	barebox(0, 0, 0, 0);

	return -EINVAL;
}

static struct image_handler image_handler_rkns_barebox_image = {
	.name = "Rockchip RKNS barebox image",
	.bootm = do_bootm_rkns_barebox_image,
	.filetype = filetype_rockchip_rkns_image,
};

static int rockchip_register_barebox_image_handler(void)
{
	if (rockchip_soc() < 0)
		return 0;

	register_image_handler(&image_handler_rkns_barebox_image);

	return 0;
}
late_initcall(rockchip_register_barebox_image_handler);
