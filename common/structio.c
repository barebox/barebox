/* SPDX-License-Identifier: GPL-2.0-only */

#include <structio.h>
#include <command.h>
#include <device.h>
#include <linux/export.h>

static struct bobject *active_capture;

struct bobject *structio_active(void)
{
	return active_capture;
}

int structio_run_command(struct bobject **bret, const char *cmd)
{
	struct bobject *bobj;
	int ret;

	if (!bret)
		return run_command(cmd);

	active_capture = bobj = bobject_alloc("capture");
	bobj->local = true;

	ret = run_command(cmd);

	active_capture = NULL;

	if (ret) {
		bobject_free(bobj);
		return ret;
	}

	*bret = bobj;
	return 0;
}
EXPORT_SYMBOL(structio_run_command);

int structio_devinfo(struct bobject **bret, struct device *dev)
{
	struct bobject *bobj;

	if (!bret) {
		devinfo(dev);
		return 0;
	}

	active_capture = bobj = bobject_alloc("devinfo");
	bobj->local = true;

	devinfo(dev);

	active_capture = NULL;

	*bret = bobj;
	return 0;
}
EXPORT_SYMBOL(structio_devinfo);
