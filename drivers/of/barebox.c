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

/* If dev describes a file on a fs, mount the fs and return a pointer
 * to the file's path.  Otherwise return an error code or NULL if the
 * device path should be used.
 * Does nothing in env in a file support isn't enabled.
 */
static char *environment_check_mount(struct device *dev, const char *devpath)
{
	const char *filepath;
	int ret;

	if (!IS_ENABLED(CONFIG_OF_BAREBOX_ENV_IN_FS))
		return NULL;

	ret = of_property_read_string(dev->of_node, "file-path", &filepath);
	if (ret == -EINVAL) {
		/* No file-path so just use device-path */
		return NULL;
	} else if (ret) {
		/* file-path property exists, but has error */
		dev_err(dev, "Problem with file-path property\n");
		return ERR_PTR(ret);
	}

	/* Get device env is on and mount it */
	mkdir(ENV_MNT_DIR, 0777);
	ret = mount(devpath, "fat", ENV_MNT_DIR, NULL);
	if (ret) {
		dev_err(dev, "Failed to load environment: mount %s failed (%d)\n",
			devpath, ret);
		return ERR_PTR(ret);
	}

	/* Set env to be in a file on the now mounted device */
	dev_dbg(dev, "Loading default env from %s on device %s\n",
		filepath, devpath);

	return basprintf("%s/%s", ENV_MNT_DIR, filepath);
}

static int environment_probe(struct device *dev)
{
	char *devpath, *filepath;
	int ret;

	ret = of_find_path(dev->of_node, "device-path", &devpath,
			   OF_FIND_PATH_FLAGS_BB);
	if (ret)
		return ret;

	/* Do we need to mount a fs and find env there? */
	filepath = environment_check_mount(dev, devpath);
	if (IS_ERR(filepath)) {
		free(devpath);
		return ret;
	}

	if (filepath)
		free(devpath);
	else
		filepath = devpath;

	dev_dbg(dev, "Setting default environment path to %s\n", filepath);
	default_environment_path_set(filepath);

	free(filepath);

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
	if (!node)
		return 0;

	of_platform_populate(node, of_default_bus_match_table, NULL);

	platform_driver_register(&environment_driver);

	return 0;
}
late_initcall(barebox_of_driver_init);
