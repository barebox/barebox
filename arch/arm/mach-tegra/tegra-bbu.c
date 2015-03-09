/*
 * Copyright (C) 2015 Lucas Stach <l.stach@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <fcntl.h>
#include <fs.h>
#include <mach/tegra-bbu.h>
#include <malloc.h>

static int tegra_bbu_emmc_handler(struct bbu_handler *handler,
				  struct bbu_data *data)
{

	int fd, ret;

	if (file_detect_type(data->image + 0x4000, data->len) !=
	    filetype_arm_barebox &&
	    !bbu_force(data, "Not an ARM barebox image"))
		return -EINVAL;

	ret = bbu_confirm(data);
	if (ret)
		return ret;

	fd = open(data->devicefile, O_WRONLY);
	if (fd < 0)
		return fd;

	ret = write(fd, data->image, data->len);
	if (ret < 0) {
		pr_err("writing update to %s failed with %s\n",
			data->devicefile, strerror(-ret));
		goto err_close;
	}

	ret = 0;

err_close:
	close(fd);

	return ret;
}

int tegra_bbu_register_emmc_handler(const char *name, char *devicefile,
				    unsigned long flags)
{
	struct bbu_handler *handler;
	int ret = 0;

	handler = xzalloc(sizeof(*handler));
	handler->name = name;
	handler->devicefile = devicefile;
	handler->flags = flags;
	handler->handler = tegra_bbu_emmc_handler;

	ret = bbu_register_handler(handler);
	if (ret)
		free(handler);

	return ret;
}
