#include <common.h>
#include <partition.h>
#include <nand.h>
#include <driver.h>
#include <linux/mtd/mtd.h>
#include <fs.h>
#include <fcntl.h>
#include <mach/xload.h>
#include <sizes.h>

void *omap_xload_boot_nand(int offset, int size)
{
	int ret;
	void *to = xmalloc(size);
	struct cdev *cdev;

	devfs_add_partition("nand0", offset, size, PARTITION_FIXED, "x");
	dev_add_bb_dev("x", "bbx");

	cdev = cdev_open("bbx", O_RDONLY);
	if (!cdev) {
		printf("failed to open nand\n");
		return NULL;
	}

	ret = cdev_read(cdev, to, size, 0, 0);
	if (ret != size) {
		printf("failed to read from nand\n");
		return NULL;
	}

	return to;
}

void *omap_xload_boot_mmc(void)
{
	int ret;
	void *buf;
	int len;

	ret = mount("disk0.0", "fat", "/");
	if (ret) {
		printf("mounting sd card failed with %d\n", ret);
		return NULL;
	}

	buf = read_file("/barebox.bin", &len);
	if (!buf) {
		printf("could not read barebox.bin from sd card\n");
		return NULL;
	}

	return buf;
}
