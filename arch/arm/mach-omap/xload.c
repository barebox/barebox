#include <common.h>
#include <partition.h>
#include <nand.h>
#include <init.h>
#include <driver.h>
#include <linux/mtd/mtd.h>
#include <fs.h>
#include <fcntl.h>
#include <mach/generic.h>
#include <sizes.h>
#include <filetype.h>

static void *read_image_head(const char *name)
{
	void *header = xmalloc(ARM_HEAD_SIZE);
	struct cdev *cdev;
	int ret;

	cdev = cdev_open(name, O_RDONLY);
	if (!cdev) {
		printf("failed to open partition\n");
		return NULL;
	}

	ret = cdev_read(cdev, header, ARM_HEAD_SIZE, 0, 0);
	cdev_close(cdev);

	if (ret != ARM_HEAD_SIZE) {
		printf("failed to read from partition\n");
		return NULL;
	}

	return header;
}

static unsigned int get_image_size(void *head)
{
	unsigned int ret = 0;
	unsigned int *psize = head + ARM_HEAD_SIZE_OFFSET;

	if (is_barebox_arm_head(head))
		ret = *psize;
	debug("Detected barebox image size %u\n", ret);

	return ret;
}

static void *omap_xload_boot_nand(int offset)
{
	int ret;
	int size;
	void *to, *header;
	struct cdev *cdev;

	devfs_add_partition("nand0", offset, SZ_1M, DEVFS_PARTITION_FIXED, "x");
	dev_add_bb_dev("x", "bbx");

	header = read_image_head("bbx");
	if (header == NULL)
		return NULL;

	size = get_image_size(header);
	if (!size) {
		printf("failed to get image size\n");
		return NULL;
	}

	to = xmalloc(size);

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

static void *omap_xload_boot_mmc(void)
{
	int ret;
	void *buf;
	int len;
	const char *diskdev = "disk0.0";

	ret = mount(diskdev, "fat", "/");
	if (ret) {
		printf("Unable to mount %s (%d)\n", diskdev, ret);
		return NULL;
	}

	buf = read_file("/barebox.bin", &len);
	if (!buf) {
		printf("could not read barebox.bin from sd card\n");
		return NULL;
	}

	return buf;
}

static void *omap_xload_boot_spi(int offset)
{
	int ret;
	int size;
	void *to, *header;
	struct cdev *cdev;

	devfs_add_partition("m25p0", offset, SZ_1M, DEVFS_PARTITION_FIXED, "x");

	header = read_image_head("x");
	if (header == NULL)
		return NULL;

	size = get_image_size(header);
	if (!size) {
		printf("failed to get image size\n");
		return NULL;
	}

	to = xmalloc(size);

	cdev = cdev_open("x", O_RDONLY);
	if (!cdev) {
		printf("failed to open spi flash\n");
		return NULL;
	}

	ret = cdev_read(cdev, to, size, 0, 0);
	if (ret != size) {
		printf("failed to read from spi flash\n");
		return NULL;
	}

	return to;
}

static void *omap4_xload_boot_usb(void){
	int ret;
	void *buf;
	int len;

	ret = mount("omap4_usbboot", "omap4_usbbootfs", "/");
	if (ret) {
		printf("Unable to mount omap4_usbbootfs (%d)\n", ret);
		return NULL;
	}

	buf = read_file("/barebox.bin", &len);
	if (!buf)
		printf("could not read barebox.bin from omap4_usbbootfs\n");

	return buf;
}

/*
 * Replaces the default shell in xload configuration
 */
static __noreturn int omap_xload(void)
{
	int (*func)(void) = NULL;

	switch (omap_bootsrc())
	{
	case OMAP_BOOTSRC_MMC1:
		printf("booting from MMC1\n");
		func = omap_xload_boot_mmc();
		break;
	case OMAP_BOOTSRC_USB1:
		if (IS_ENABLED(CONFIG_FS_OMAP4_USBBOOT)) {
			printf("booting from USB1\n");
			func = omap4_xload_boot_usb();
			break;
		} else {
			printf("booting from usb1 not enabled\n");
		}
	case OMAP_BOOTSRC_UNKNOWN:
		printf("unknown boot source. Fall back to nand\n");
	case OMAP_BOOTSRC_NAND:
		printf("booting from NAND\n");
		func = omap_xload_boot_nand(SZ_128K);
		break;
	case OMAP_BOOTSRC_SPI1:
		printf("booting from SPI1\n");
		func = omap_xload_boot_spi(SZ_128K);
		break;
	}

	if (!func) {
		printf("booting failed\n");
		while (1);
	}

	shutdown_barebox();
	func();

	while (1);
}

static int omap_set_xload(void)
{
	barebox_main = omap_xload;

	return 0;
}
late_initcall(omap_set_xload);
