#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <asm/arch/linux.h>
#include <init.h>
#include <errno.h>
#include <asm-generic/errno.h>
#include <asm/arch/hostfile.h>
#include <xfuncs.h>

ssize_t hf_read(struct device_d *dev, void *buf, size_t count, ulong offset, ulong flags)
{
	struct hf_platform_data *hf = dev->platform_data;
	int fd = hf->fd;

	if (linux_lseek(fd, offset) != offset)
		return -EINVAL;

	return linux_read(fd, buf, count);
}

ssize_t hf_write(struct device_d *dev, const void *buf, size_t count, ulong offset, ulong flags)
{
	struct hf_platform_data *hf = dev->platform_data;
	int fd = hf->fd;

	if (linux_lseek(fd, offset) != offset)
		return -EINVAL;

	return linux_write(fd, buf, count);
}

static void hf_info(struct device_d *dev)
{
	struct hf_platform_data *hf = dev->platform_data;

	printf("file: %s\n", hf->filename);
}

static struct driver_d hf_drv = {
	.name  = "hostfile",
	.probe = dummy_probe,
	.read  = hf_read,
	.write = hf_write,
	.info  = hf_info,
	.type  = DEVICE_TYPE_BLOCK,
};

static int hf_init(void)
{
	return register_driver(&hf_drv);
}

device_initcall(hf_init);

int u_boot_register_filedev(struct hf_platform_data *hf, char *name_template)
{
	struct device_d *dev;

	dev = xzalloc(sizeof(struct device_d));

	dev->platform_data = hf;

	hf = dev->platform_data;

	strcpy(dev->name,"hostfile");
	get_free_deviceid(dev->id, name_template);
	dev->size = hf->size;
	dev->map_base = hf->map_base;
	dev->type = DEVICE_TYPE_BLOCK;

	return register_device(dev);
}

