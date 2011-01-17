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

	ret = info->fbops->fb_activate_var(info);

	dev->map_base = (unsigned long)info->screen_base;

	if (ret == 0)
		dev_param_set_generic(dev, param, val);

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

	info->cdev.ops = &fb_ops;
	info->cdev.name = asprintf("fb%d", id);
	info->cdev.size = info->xres * info->yres * (info->bits_per_pixel >> 3);
	info->cdev.dev = &info->dev;
	info->cdev.priv = info;
	info->cdev.dev->map_base = (unsigned long)info->screen_base;
	info->cdev.dev->size = info->cdev.size;

	dev = &info->dev;
	dev->priv = info;
	dev->id = id;

	sprintf(dev->name, "fb");

	register_device(&info->dev);
	dev_add_param(dev, "enable", fb_enable_set, NULL, 0);
	dev_set_param(dev, "enable", "0");

	if (info->num_modes && (info->mode_list != NULL) &&
			(info->fbops->fb_activate_var != NULL)) {
		dev_add_param(dev, "mode_name", fb_setup_mode, NULL, 0);
		dev_set_param(dev, "mode_name", info->mode_list[0].name);
	}

	devfs_create(&info->cdev);

	return 0;
}

