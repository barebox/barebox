// SPDX-License-Identifier: GPL-2.0-only
/*
 * barebox.c
 *
 * Copyright (c) 2013 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <malloc.h>
#include <envfs.h>
#include <fs.h>

#define ENV_MNT_DIR "/boot"	/* If env on filesystem, where to mount */

const char *of_env_get_device_alias_by_path(const char *of_path)
{
	return of_property_get_alias(of_path, "device-path");
}

static char *environment_probe_1node_binding(struct device *dev)
{
	struct cdev *cdev;

	cdev = cdev_by_device_node(dev->of_node);
	if (!cdev)
		return ERR_PTR(-EINVAL);

	return basprintf("/dev/%s", cdev_name(cdev));
}

static char *environment_probe_2node_binding(struct device *dev)
{
	const char *filepath;
	char *devpath = NULL;
	int ret;

	ret = of_find_path(dev->of_node, "device-path", &devpath,
			   OF_FIND_PATH_FLAGS_BB);
	if (ret)
		goto out;

	if (!of_property_present(dev->of_node, "file-path"))
		return devpath;

	if (!IS_ENABLED(CONFIG_OF_BAREBOX_ENV_IN_FS))
		return ERR_PTR(-ENOENT);

	ret = of_property_read_string(dev->of_node, "file-path", &filepath);
	if (ret) {
		/* file-path property exists, but has error */
		dev_err(dev, "Problem with file-path property\n");
		goto out;
	}

	/* Get device env is on and mount it */
	mkdir(ENV_MNT_DIR, 0777);
	ret = mount(devpath, "fat", ENV_MNT_DIR, NULL);
	if (ret) {
		dev_err(dev, "Failed to load environment: mount %s failed (%d)\n",
			devpath, ret);
		goto out;
	}

	/* Set env to be in a file on the now mounted device */
	dev_dbg(dev, "Loading default env from %s on device %s\n",
		filepath, devpath);

out:
	free(devpath);
	return ret ? ERR_PTR(ret) : basprintf("%s/%s", ENV_MNT_DIR, filepath);
}

static int environment_probe(struct device *dev)
{
	char *path;

	if (of_property_present(dev->of_node, "device-path"))
		path = environment_probe_2node_binding(dev);
	else
		path = environment_probe_1node_binding(dev);

	if (IS_ERR(path))
		return PTR_ERR(path);

	dev_dbg(dev, "Setting default environment path to %s\n", path);
	default_environment_path_set(path);

	free(path);

	return 0;
}

static struct of_device_id environment_dt_ids[] = {
	{
		.compatible = "barebox,environment",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, environment_dt_ids);

static struct driver environment_driver = {
	.name		= "barebox-environment",
	.probe		= environment_probe,
	.of_compatible	= environment_dt_ids,
};

static int barebox_of_driver_init(void)
{
	struct device_node *node;

	node = of_get_root_node();
	if (!node)
		return 0;

	node = of_find_node_by_path("/chosen");
	if (node)
		of_platform_populate(node, of_default_bus_match_table, NULL);

	return platform_driver_register(&environment_driver);
}
late_initcall(barebox_of_driver_init);
