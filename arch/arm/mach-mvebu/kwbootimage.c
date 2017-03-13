#include <bootm.h>
#include <common.h>
#include <fcntl.h>
#include <filetype.h>
#include <fs.h>
#include <init.h>
#include <libfile.h>
#include <restart.h>
#include <unistd.h>
#include <asm/unaligned.h>
#include <mach/common.h>

static int do_bootm_kwbimage_v1(struct image_data *data)
{
	int fd, ret;
	loff_t offset;
	char header[0x20];
	u32 image_size, image_source;
	void (*barebox)(void);

	fd = open(data->os_file, O_RDONLY);
	if (fd < 0)
		return fd;

	ret = read_full(fd, header, 0x20);
	if (ret < 0x20) {
		pr_err("Failed to read header\n");
		if (ret >= 0)
			return -EINVAL;
		return -errno;
	}

	image_size = header[4] | header[5] << 8 | header[6] << 16 | header[7] << 24;
	image_source = header[0xc] | header[0xd] << 8 |
		header[0xe] << 16 | header[0xf] << 24;

	if (data->verbose)
		pr_info("size: %u\noffset: %u\n", image_size, image_source);

	offset = lseek(fd, image_source, SEEK_SET);
	if (offset < 0) {
		pr_err("Failed to seek to image (%lld, %d)\n", offset, errno);
		return -errno;
	}

	barebox = xzalloc(image_size);

	ret = read_full(fd, barebox, image_size);
	if (ret < image_size) {
		pr_err("Failed to read image\n");
		if (ret >= 0)
			ret = -EINVAL;
		else
			ret = -errno;
		goto out_free;
	}

	if (is_barebox_arm_head((void *)barebox))
		put_unaligned_le32(MVEBU_REMAP_INT_REG_BASE, barebox + 0x30);

	shutdown_barebox();

	barebox();

	restart_machine();

out_free:
	free(barebox);
	return ret;
}

static struct image_handler image_handler_kwbimage_v1_handler = {
	.name = "MVEBU kwbimage v1",
	.bootm = do_bootm_kwbimage_v1,
	.filetype = filetype_kwbimage_v1,
};

static int mvebu_register_kwbimage_image_handler(void)
{
	register_image_handler(&image_handler_kwbimage_v1_handler);

	return 0;
}
late_initcall(mvebu_register_kwbimage_image_handler);
