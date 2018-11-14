#include <common.h>
#include <bootsource.h>
#include <partition.h>
#include <nand.h>
#include <init.h>
#include <driver.h>
#include <linux/mtd/mtd.h>
#include <libfile.h>
#include <fs.h>
#include <fcntl.h>
#include <linux/sizes.h>
#include <malloc.h>
#include <filetype.h>
#include <xymodem.h>
#include <mach/generic.h>
#include <mach/am33xx-generic.h>
#include <mach/omap3-generic.h>
#include <net.h>
#include <environment.h>
#include <dhcp.h>

struct omap_barebox_part *barebox_part;

static struct omap_barebox_part default_part = {
	.nand_offset = SZ_128K,
	.nand_size = SZ_1M,
	.nand_bkup_offset = 0,
	.nand_bkup_size = 0,
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
		printf("failed to open %s\n", name);
		return NULL;
	}

	ret = cdev_read(cdev, header, ARM_HEAD_SIZE, 0, 0);
	cdev_close(cdev);

	if (ret != ARM_HEAD_SIZE) {
		printf("failed to read from %s\n", name);
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

static void *read_mtd_barebox(const char *partition)
{
	int ret;
	int size;
	void *to, *header;
	struct cdev *cdev;

	header = read_image_head(partition);
	if (header == NULL)
		return NULL;

	size = get_image_size(header);
	if (!size) {
		printf("failed to get image size\n");
		return NULL;
	}

	to = xmalloc(size);

	cdev = cdev_open(partition, O_RDONLY);
	if (!cdev) {
		printf("failed to open partition\n");
		return NULL;
	}

	ret = cdev_read(cdev, to, size, 0, 0);
	if (ret != size) {
		printf("failed to read from partition\n");
		return NULL;
	}

	return to;
}

static void *omap_xload_boot_nand(struct omap_barebox_part *part)
{
	void *to;

	devfs_add_partition("nand0", part->nand_offset, part->nand_size,
					DEVFS_PARTITION_FIXED, "x");
	dev_add_bb_dev("x", "bbx");

	to = read_mtd_barebox("bbx");
	if (to == NULL && part->nand_bkup_size != 0) {
		printf("trying to load image from backup partition.\n");
		devfs_add_partition("nand0", part->nand_bkup_offset,
				part->nand_bkup_size,
				DEVFS_PARTITION_FIXED, "x_bkup");
		dev_add_bb_dev("x_bkup", "bbx_bkup");

		to = read_mtd_barebox("bbx_bkup");
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

	partname = basprintf("%s.0", diskdev);

	ret = mount(partname, NULL, "/", NULL);

	if (ret) {
		printf("Unable to mount %s (%d)\n", partname, ret);
		free(partname);
		return NULL;
	}

	free(partname);

	buf = read_file("/barebox.bin", &len);
	if (!buf)
		buf = read_file("/boot/barebox.bin", &len);
	if (!buf) {
		printf("could not read barebox.bin from sd card\n");
		return NULL;
	}

	return buf;
}

static void *omap_xload_boot_spi(struct omap_barebox_part *part)
{
	devfs_add_partition("m25p0", part->nor_offset, part->nor_size,
					DEVFS_PARTITION_FIXED, "x");

	return read_mtd_barebox("x");
}

static void *omap4_xload_boot_usb(void){
	int ret;
	void *buf;
	int len;

	ret = mount("omap4_usbboot", "omap4_usbbootfs", "/", NULL);
	if (ret) {
		printf("Unable to mount omap4_usbbootfs (%d)\n", ret);
		return NULL;
	}

	buf = read_file("/barebox.bin", &len);
	if (!buf)
		printf("could not read barebox.bin from omap4_usbbootfs\n");

	return buf;
}

static void *omap_serial_boot(void){
	struct console_device *cdev;
	int ret;
	void *buf;
	int len;
	int fd;

	/* need temporary place to store file */
	ret = mount("none", "ramfs", "/", NULL);
	if (ret < 0) {
		printf("failed to mount ramfs\n");
		return NULL;
	}

	cdev = console_get_first_active();
	if (!cdev) {
		printf("failed to get console\n");
		return NULL;
	}

	fd = open("/barebox.bin", O_WRONLY | O_CREAT);
	if (fd < 0) {
		printf("could not create barebox.bin\n");
		return NULL;
	}

	ret = do_load_serial_xmodem(cdev, fd);
	if (ret < 0) {
		printf("loadx failed\n");
		return NULL;
	}

	buf = read_file("/barebox.bin", &len);
	if (!buf)
		printf("could not read barebox.bin from serial\n");

	return buf;
}

#define TFTP_MOUNT "/.tftp"

static void *am33xx_net_boot(void)
{
	void *buf = NULL;
	int err;
	int len;
	struct dhcp_req_param dhcp_param;
	const char *bootfile;
	IPaddr_t ip;
	char *file;
	char ip4_str[sizeof("255.255.255.255")];
	struct eth_device *edev;
	struct dhcp_result *dhcp_res;

	am33xx_register_ethaddr(0, 0);

	memset(&dhcp_param, 0, sizeof(struct dhcp_req_param));
	dhcp_param.vendor_id = "am335x barebox-mlo";

	edev = eth_get_byname("eth0");
	if (!edev) {
		printf("eth0 not found\n");
		return NULL;
	}

	err = dhcp_request(edev, &dhcp_param, &dhcp_res);
	if (err) {
		printf("dhcp failed\n");
		return NULL;
	}

	dhcp_set_result(edev, dhcp_res);

	edev->ifup = true;

	/*
	 * Older tftp server don't send the file size.
	 * Then tftpfs needs temporary place to store the file.
	 */
	err = mount("none", "ramfs", "/", NULL);
	if (err < 0) {
		printf("failed to mount ramfs\n");
		return NULL;
	}

	err = make_directory(TFTP_MOUNT);
	if (err)
		return NULL;

	ip = net_get_serverip();
	sprintf(ip4_str, "%pI4", &ip);
	err = mount(ip4_str, "tftp", TFTP_MOUNT, NULL);
	if (err < 0) {
		printf("Unable to mount.\n");
		return NULL;
	}

	bootfile = dhcp_res->bootfile;
	if (!bootfile) {
		printf("bootfile not found.\n");
		return NULL;
	}

	file = basprintf("%s/%s", TFTP_MOUNT, bootfile);

	buf = read_file(file, &len);
	if (!buf)
		printf("could not read %s.\n", bootfile);

	free(file);

	umount(TFTP_MOUNT);

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
		if (IS_ENABLED(CONFIG_OMAP3_USBBOOT) && cpu_is_omap3()) {
			printf("booting from USB\n");
			func = omap3_xload_boot_usb();
		} else if (IS_ENABLED(CONFIG_FS_OMAP4_USBBOOT)) {
			printf("booting from USB\n");
			func = omap4_xload_boot_usb();
		} else {
			printf("booting from USB not enabled\n");
			func = NULL;
		}
		break;
	case BOOTSOURCE_NAND:
		printf("booting from NAND\n");
		func = omap_xload_boot_nand(barebox_part);
		break;
	case BOOTSOURCE_SPI:
		printf("booting from SPI\n");
		func = omap_xload_boot_spi(barebox_part);
		break;
	case BOOTSOURCE_SERIAL:
		if (IS_ENABLED(CONFIG_OMAP_SERIALBOOT)) {
			printf("booting from serial\n");
			func = omap_serial_boot();
			break;
		}
	case BOOTSOURCE_NET:
		if (IS_ENABLED(CONFIG_AM33XX_NET_BOOT)) {
			printf("booting from NET\n");
			func = am33xx_net_boot();
			break;
		} else {
			printf("booting from network not enabled\n");
		}
	default:
		printf("unknown boot source. Fall back to nand\n");
		func = omap_xload_boot_nand(barebox_part);
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
