#include <common.h>
#include <fb.h>
#include <errno.h>
#include <command.h>
#include <getopt.h>
#include <fcntl.h>
#include <fs.h>

static int fb_ioctl(struct cdev* cdev, int req, void *data)
{
	struct fb_info *info = cdev->priv;

	switch (req) {
	case FBIOGET_SCREENINFO:
		memcpy(data, info, sizeof(*info));
		break;
	case FBIO_ENABLE:
		info->fbops->fb_enable(info);
		break;
	case FBIO_DISABLE:
		info->fbops->fb_disable(info);
		break;
	default:
		return -ENOSYS;
	}

	return 0;
}

static int fb_enable_set(struct device_d *dev, struct param_d *param,
		const char *val)
{
	struct fb_info *info = dev->priv;
	int enable;

	enable = simple_strtoul(val, NULL, 0);

	if (enable)
		info->fbops->fb_enable(info);
	else
		info->fbops->fb_disable(info);

	sprintf(info->enable_string, "%d", !!enable);

	return 0;
}

static struct file_operations fb_ops = {
	.read	= mem_read,
	.write	= mem_write,
	.memmap	= generic_memmap_rw,
	.lseek	= dev_lseek_default,
	.ioctl	= fb_ioctl,
};

int register_framebuffer(struct fb_info *info)
{
	int id = get_free_deviceid("fb");
	struct device_d *dev;

	info->cdev.ops = &fb_ops;
	info->cdev.name = asprintf("fb%d", id);
	info->cdev.size = info->xres * info->yres * (info->bits_per_pixel >> 3);
	info->cdev.dev = &info->dev;
	info->cdev.priv = info;
	info->cdev.dev->map_base = (unsigned long)info->screen_base;
	info->cdev.dev->size = info->cdev.size;

	dev = &info->dev;
	dev->priv = info;

	sprintf(dev->name, "fb");

	info->param_enable.set = fb_enable_set;
	info->param_enable.name = "enable";
	sprintf(info->enable_string, "%d", 0);
	info->param_enable.value = info->enable_string;
	dev_add_param(dev, &info->param_enable);

	register_device(&info->dev);

	devfs_create(&info->cdev);

	return 0;
}

