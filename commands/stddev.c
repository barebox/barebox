/*
 * Copyright (c) 2011 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * MA 02111-1307 USA
 */

#include <common.h>
#include <init.h>

static ssize_t zero_read(struct cdev *cdev, void *buf, size_t count, ulong offset, ulong flags)
{
	memset(buf, 0, count);
	return count;
}

static struct file_operations zeroops = {
	.read  = zero_read,
	.lseek = dev_lseek_default,
};

static int zero_init(void)
{
	struct cdev *cdev;

	cdev = xzalloc(sizeof (*cdev));

	cdev->name = "zero";
	cdev->size = ~0;
	cdev->ops = &zeroops;

	devfs_create(cdev);

	return 0;
}

device_initcall(zero_init);
