#include <common.h>
#include <fb.h>
#include <errno.h>
#include <command.h>
#include <getopt.h>
#include <fcntl.h>
#include <fs.h>
#include <init.h>

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
	char *new;

	if (!val)
		return dev_param_set_generic(dev, param, NULL);

	enable = simple_strtoul(val, NULL, 0);

	if (enable) {
		if (!info->enabled)
			info->fbops->fb_enable(info);
		new = "1";
	} else {
		if (info->enabled)
			info->fbops->fb_disable(info);
		new = "0";
	}

	dev_param_set_generic(dev, param, new);

	info->enabled = !!enable;

	return 0;
}

static int fb_setup_mode(struct device_d *dev, struct param_d *param,
		const char *val)
{
	struct fb_info *info = dev->priv;
	int mode, ret;

	if (info->enabled != 0)
		return -EPERM;

	if (!val)
		return dev_param_set_generic(dev, param, NULL);

	for (mode = 0; mode < info->num_modes; mode++) {
		if (!strcmp(info->mode_list[mode].name, val))
			break;
	}
	if (mode >= info->num_modes)
		return -EINVAL;

	info->mode = &info->mode_list[mode];

	info->xres = info->mode->xres;
	info->yres = info->mode->yres;

	ret = info->fbops->fb_activate_var(info);

	if (!ret) {
		dev->resource[0].start = (resource_size_t)info->screen_base;
		info->cdev.size = info->xres * info->yres * (info->bits_per_pixel >> 3);
		dev->resource[0].end = dev->resource[0].start + info->cdev.size - 1;
		dev_param_set_generic(dev, param, val);
	} else
		info->cdev.size = 0;

	return ret;
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

	dev = &info->dev;

	info->cdev.ops = &fb_ops;
	info->cdev.name = asprintf("fb%d", id);
	info->cdev.size = info->xres * info->yres * (info->bits_per_pixel >> 3);
	info->cdev.dev = dev;
	info->cdev.priv = info;
	dev->resource = xzalloc(sizeof(struct resource));
	dev->resource[0].start = (resource_size_t)info->screen_base;
	dev->resource[0].end = dev->resource[0].start + info->cdev.size - 1;
	dev->resource[0].flags = IORESOURCE_MEM;
	dev->num_resources = 1;

	dev->priv = info;
	dev->id = id;

	sprintf(dev->name, "fb");

	info->dev.bus = &fb_bus;
	register_device(&info->dev);

	return 0;
}

static void fb_info(struct device_d *dev)
{
	struct fb_info *info = dev->priv;
	int i;

	if (!info->num_modes)
		return;

	printf("available modes:\n");

	for (i = 0; i < info->num_modes; i++) {
		struct fb_videomode *mode = &info->mode_list[i];

		printf("%-10s %dx%d@%d\n", mode->name,
				mode->xres, mode->yres, mode->refresh);
	}

	printf("\n");
}

static struct driver_d fb_driver = {
	.name  = "fb",
	.info = fb_info,
};

static int fb_match(struct device_d *dev, struct driver_d *drv)
{
	return 0;
}

static int fb_probe(struct device_d *dev)
{
	struct fb_info *info = dev->priv;

	dev_add_param(dev, "enable", fb_enable_set, NULL, 0);
	dev_set_param(dev, "enable", "0");

	if (info->num_modes && (info->mode_list != NULL) &&
			(info->fbops->fb_activate_var != NULL)) {
		dev_add_param(dev, "mode_name", fb_setup_mode, NULL, 0);
		dev_set_param(dev, "mode_name", info->mode_list[0].name);
	}

	return devfs_create(&info->cdev);
}

static void fb_remove(struct device_d *dev)
{
}

struct bus_type fb_bus = {
	.name = "fb",
	.match = fb_match,
	.probe = fb_probe,
	.remove = fb_remove,
};

static int fb_bus_init(void)
{
	return bus_register(&fb_bus);
}
pure_initcall(fb_bus_init);

static int fb_init_driver(void)
{
	fb_driver.bus = &fb_bus;
	register_driver(&fb_driver);
	return 0;
}
device_initcall(fb_init_driver);
