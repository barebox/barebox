#include <common.h>
#include <bootsource.h>
#include <partition.h>
#include <nand.h>
#include <init.h>
#include <driver.h>
#include <linux/mtd/mtd.h>
#include <fs.h>
#include <fcntl.h>
#include <sizes.h>
#include <malloc.h>
#include <filetype.h>
#include <mach/generic.h>

struct omap_barebox_part *barebox_part;

static struct omap_barebox_part default_part = {
	.nand_offset = SZ_128K,
	.nand_size = SZ_1M,
	.nor_offset = SZ_128K,
	.nor_size = SZ_1M,
};

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

static void *omap_xload_boot_nand(int offset, int part_size)
{
	int ret;
	int size;
	void *to, *header;
	struct cdev *cdev;

	devfs_add_partition("nand0", offset, part_size,
					DEVFS_PARTITION_FIXED, "x");
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
	const char *diskdev;
	char *partname;

	diskdev = omap_get_bootmmc_devname();
	if (!diskdev)
		diskdev = "disk0";

	device_detect_by_name(diskdev);

	partname = asprintf("%s.0", diskdev);

	ret = mount(partname, "fat", "/");

	free(partname);

	if (ret) {
		printf("Unable to mount %s (%d)\n", partname, ret);
		return NULL;
	}

	buf = read_file("/barebox.bin", &len);
	if (!buf) {
		printf("could not read barebox.bin from sd card\n");
		return NULL;
	}

	return buf;
}

static void *omap_xload_boot_spi(int offset, int part_size)
{
	int ret;
	int size;
	void *to, *header;
	struct cdev *cdev;

	devfs_add_partition("m25p0", offset, part_size,
					DEVFS_PARTITION_FIXED, "x");

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
	void *func;

	if (!barebox_part)
		barebox_part = &default_part;

	switch (bootsource_get())
	{
	case BOOTSOURCE_MMC:
		printf("booting from MMC\n");
		func = omap_xload_boot_mmc();
		break;
	case BOOTSOURCE_USB:
		if (IS_ENABLED(CONFIG_FS_OMAP4_USBBOOT)) {
			printf("booting from USB\n");
			func = omap4_xload_boot_usb();
			break;
		} else {
			printf("booting from USB not enabled\n");
		}
	case BOOTSOURCE_NAND:
		printf("booting from NAND\n");
		func = omap_xload_boot_nand(barebox_part->nand_offset,
					barebox_part->nand_size);
		break;
	case BOOTSOURCE_SPI:
		printf("booting from SPI\n");
		func = omap_xload_boot_spi(barebox_part->nor_offset,
					barebox_part->nor_size);
		break;
	default:
		printf("unknown boot source. Fall back to nand\n");
		func = omap_xload_boot_nand(barebox_part->nand_offset,
					barebox_part->nand_size);
		break;
	}

	if (!func) {
		printf("booting failed\n");
		while (1);
	}

	omap_start_barebox(func);
}

int omap_set_barebox_part(struct omap_barebox_part *part)
{
	barebox_part = part;

	return 0;
}

static int omap_set_xload(void)
{
	barebox_main = omap_xload;

	return 0;
}
late_initcall(omap_set_xload);
