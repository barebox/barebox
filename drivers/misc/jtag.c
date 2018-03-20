/*
 * drivers/misc/jtag.c - More infos in include/jtag.h
 *
 * Written Aug 2009 by Davide Rizzo <elpa.rizzo@gmail.com>
 *
 * Ported to barebox Jul 2012 by
 *       Wjatscheslaw Stoljarski <wjatscheslaw.stoljarski@kiwigrid.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <common.h>
#include <errno.h>
#include <jtag.h>
#include <gpio.h>
#include <driver.h>
#include <malloc.h>
#include <init.h>
#include <ioctl.h>

#define VERSION_MAJ 1
#define VERSION_MIN 0

/* Max devices in the jtag chain */
#define MAX_DEVICES 16

struct jtag_info {
	struct jtag_platdata *pdata;
	struct cdev cdev;
	unsigned int devices; /* Number of devices found in the jtag chain */
	/* Instruction register length of every device in the chain */
	unsigned int ir_len[MAX_DEVICES];	/* [devices] */
};

static const unsigned long bypass = 0xFFFFFFFF;

static void pulse_tms0(const struct jtag_platdata *pdata)
{
	gpio_set_value(pdata->gpio_tms, 0);
	gpio_set_value(pdata->gpio_tclk, 0);
	gpio_set_value(pdata->gpio_tclk, 1);
}

static void pulse_tms1(const struct jtag_platdata *pdata)
{
	gpio_set_value(pdata->gpio_tms, 1);
	gpio_set_value(pdata->gpio_tclk, 0);
	gpio_set_value(pdata->gpio_tclk, 1);
}

static void jtag_reset(const struct jtag_platdata *pdata)
{
	gpio_set_value(pdata->gpio_tms, 1);
	gpio_set_value(pdata->gpio_tclk, 0);
	gpio_set_value(pdata->gpio_tclk, 1);
	gpio_set_value(pdata->gpio_tclk, 0);
	gpio_set_value(pdata->gpio_tclk, 1);
	gpio_set_value(pdata->gpio_tclk, 0);
	gpio_set_value(pdata->gpio_tclk, 1);
	gpio_set_value(pdata->gpio_tclk, 0);
	gpio_set_value(pdata->gpio_tclk, 1);
	gpio_set_value(pdata->gpio_tclk, 0);
	gpio_set_value(pdata->gpio_tclk, 1);
}

static void jtag_output(const struct jtag_platdata *pdata,
	const unsigned long *data, unsigned int bitlen, int notlast)
{
	unsigned int a;
	unsigned long mask;
	gpio_set_value(pdata->gpio_tms, 0);
	while (bitlen > 0) {
		for (a = *data++, mask = 0x00000001; mask != 0 && bitlen > 0;
		      mask <<= 1, bitlen--) {
			gpio_set_value(pdata->gpio_tdo, (a & mask) ? 1 : 0);
			if ((bitlen == 1) && !notlast)
				gpio_set_value(pdata->gpio_tms, 1);
			gpio_set_value(pdata->gpio_tclk, 0);
			gpio_set_value(pdata->gpio_tclk, 1);
		}
	}
}

static int jtag_ioctl(struct cdev *inode, int cmd, void *arg)
{
	int ret = 0;
	struct jtag_info *info = (struct jtag_info *)inode->priv;
	int devices = info->devices;
	struct jtag_cmd *jcmd = (struct jtag_cmd *)arg;
	struct jtag_platdata *pdata = info->pdata;

	if (_IOC_TYPE(cmd) != JTAG_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > JTAG_IOC_MAXNR) return -ENOTTY;

	switch (cmd) {

	case JTAG_GET_DEVICES:
		/* Returns how many devices found in the chain */
		ret = info->devices;
		break;

	case JTAG_GET_ID:
		/* Returns ID register of selected device */
		if ((((struct jtag_rd_id *)arg)->device < 0) ||
			(((struct jtag_rd_id *)arg)->device >= devices)) {
			ret = -EINVAL;
			break;
		}
		jtag_reset(pdata);
		pulse_tms0(pdata);
		pulse_tms1(pdata);
		pulse_tms0(pdata);
		pulse_tms0(pdata);
		while (devices-- > 0) {
			unsigned long id = 0;
			pulse_tms0(pdata);
			if (gpio_get_value(pdata->gpio_tdi)) {
				unsigned long mask;
				for (id = 1, mask = 0x00000002; (mask != 0);
				      mask <<= 1) {
					pulse_tms0(pdata);
					if (gpio_get_value(pdata->gpio_tdi))
						id |= mask;
				}
			}
			if (devices == ((struct jtag_rd_id *)arg)->device) {
				((struct jtag_rd_id *)arg)->id = id;
				ret = 0;
				break;
			}
		}
		jtag_reset(pdata);
		break;

	case JTAG_SET_IR_LENGTH:
		/* Sets up IR length of one device */
		if ((jcmd->device >= 0) && (jcmd->device < devices))
			info->ir_len[jcmd->device] = jcmd->bitlen;
		else
			ret = -EINVAL;
		break;

	case JTAG_RESET:
		/* Resets all JTAG states */
		jtag_reset(pdata);
		break;

	case JTAG_IR_WR:
		/*
		 * Writes Instruction Register
		 * If device == -1 writes same Instruction Register in
		 * all devices.
		 * If device >= 0 writes Instruction Register in selected
		 * device and loads BYPASS instruction in all others.
		 */
		if ((jcmd->device < -1) || (jcmd->device >= devices)) {
			ret = -EINVAL;
			break;
		}
		pulse_tms0(pdata);
		pulse_tms1(pdata);
		pulse_tms1(pdata);
		pulse_tms0(pdata);
		pulse_tms0(pdata);
		while (devices-- > 0) {
			if ((jcmd->device == -1) || (jcmd->device == devices))
				/* Loads desired instruction */
				jtag_output(pdata, jcmd->data,
					info->ir_len[devices], devices);
			else
				/* Loads BYPASS instruction */
				jtag_output(pdata, &bypass,
					info->ir_len[devices], devices);
		}
		pulse_tms1(pdata);
		pulse_tms0(pdata);
		break;

	case JTAG_DR_WR:
		/*
		 * Writes Data Register of all devices
		 * If device == -1 writes same Data Register in all devices.
		 * If device >= 0 writes Data Register in selected device
		 * and loads BYPASS instruction in all others.
		 */
		if ((jcmd->device < -1) || (jcmd->device >= devices)) {
			ret = -EINVAL;
			break;
		}
		pulse_tms0(pdata);
		pulse_tms1(pdata);
		pulse_tms0(pdata);
		pulse_tms0(pdata);
		while (devices-- > 0) {
			if ((jcmd->device == -1) || (devices == jcmd->device))
				/* Loads desired data */
				jtag_output(pdata, jcmd->data, jcmd->bitlen,
					devices);
			else
				/* Loads 1 dummy bit in BYPASS data register */
				jtag_output(pdata, &bypass, 1, devices);
		}
		pulse_tms1(pdata);
		pulse_tms0(pdata);
		break;

	case JTAG_DR_RD:
		/* Reads data register of selected device */
		if ((jcmd->device < 0) || (jcmd->device >= devices))
			ret = -EINVAL;
		else {
			unsigned long mask;
			int bitlen = jcmd->bitlen;
			unsigned long *data = jcmd->data;
			pulse_tms0(pdata);
			pulse_tms1(pdata);
			pulse_tms0(pdata);
			pulse_tms0(pdata);
			devices -= (jcmd->device + 1);
			while (devices-- > 0)
				pulse_tms0(pdata);
			while (bitlen > 0) {
				for (*data = 0, mask = 0x00000001;
				      (mask != 0) && (bitlen > 0);
				      mask <<= 1, bitlen--) {
					if (bitlen == 1)
						pulse_tms1(pdata);
					else
						pulse_tms0(pdata);
					if (gpio_get_value(pdata->gpio_tdi))
						*data |= mask;
				}
				data++;
			}
			pulse_tms1(pdata);
			pulse_tms0(pdata);
		}
		break;

	case JTAG_CLK:
		/* Generates arg clock pulses */
		gpio_set_value(pdata->gpio_tms, 0);
		while ((*(unsigned int *) arg)--) {
			gpio_set_value(pdata->gpio_tclk, 0);
			gpio_set_value(pdata->gpio_tclk, 1);
		}
		break;

	default:
		ret = -EFAULT;
	}

	return ret;
}

static struct cdev_operations jtag_operations = {
	.ioctl = jtag_ioctl,
};

static void jtag_info(struct device_d *pdev)
{
	int dn, ret;
	struct jtag_rd_id jid;
	struct jtag_info *info = pdev->priv;

	printf(" JTAG:\n");
	printf("  Devices found: %u\n", info->devices);
	for (dn = 0; dn < info->devices; dn++) {
		jid.device = dn;
		ret = jtag_ioctl(&info->cdev, JTAG_GET_ID, &jid);
		printf("  Device number: %d\n", dn);
		if (ret == -1)
			printf("   JTAG_GET_ID failed: %s\n", strerror(errno));
		else
			printf("   ID: 0x%lX\n", jid.id);
	}
}

static int jtag_probe(struct device_d *pdev)
{
	int i, ret;
	struct jtag_info *info;
	struct jtag_platdata *pdata = pdev->platform_data;

	/* Setup gpio pins */
	gpio_direction_output(pdata->gpio_tms, 0);
	gpio_direction_output(pdata->gpio_tclk, 1);
	gpio_direction_output(pdata->gpio_tdo, 0);
	gpio_direction_input(pdata->gpio_tdi);
	if (pdata->use_gpio_trst) {
		/*
		 * Keep fixed at 1 because some devices in the chain could
		 * not use it, to reset chain use jtag_reset()
		 */
		gpio_direction_output(pdata->gpio_trst, 1);
	}

	/* Find how many devices in chain */
	jtag_reset(pdata);
	pulse_tms0(pdata);
	pulse_tms1(pdata);
	pulse_tms1(pdata);
	pulse_tms0(pdata);
	pulse_tms0(pdata);
	gpio_set_value(pdata->gpio_tdo, 1);
	/* Fills all IR with bypass instruction */
	for (i = 0; i < 32 * MAX_DEVICES; i++)
		pulse_tms0(pdata);
	pulse_tms1(pdata);
	pulse_tms1(pdata);
	pulse_tms1(pdata);
	pulse_tms0(pdata);
	pulse_tms0(pdata);
	gpio_set_value(pdata->gpio_tdo, 0);
	/* Fills all 1-bit bypass register with 0 */
	for (i = 0; i < MAX_DEVICES + 2; i++)
		pulse_tms0(pdata);
	gpio_set_value(pdata->gpio_tdo, 1);
	/* Counts chain's bit length */
	for (i = 0; i < MAX_DEVICES + 1; i++) {
		pulse_tms0(pdata);
		if (gpio_get_value(pdata->gpio_tdi))
			break;
	}
	dev_notice(pdev, "%d devices found in chain\n", i);

	/* Allocate structure with chain specific infos */
	info = xzalloc(sizeof(struct jtag_info) + sizeof(info->ir_len[0]) * i);

	info->devices = i;
	info->pdata = pdata;
	pdev->priv = info;
	pdev->info = jtag_info;

	info->cdev.name = JTAG_NAME;
	info->cdev.dev = pdev;
	info->cdev.ops = &jtag_operations;
	info->cdev.priv = info;
	ret = devfs_create(&info->cdev);

	if (ret)
		goto fail_devfs_create;

	return 0;

fail_devfs_create:
	pdev->priv = NULL;
	free(info);
	return ret;
}

static void jtag_remove(struct device_d *pdev)
{
	struct jtag_info *info = (struct jtag_info *) pdev->priv;

	devfs_remove(&info->cdev);
	pdev->priv = NULL;
	free(info);
	dev_notice(pdev, "Device removed\n");
}

static struct driver_d jtag_driver = {
	.name = JTAG_NAME,
	.probe = jtag_probe,
	.remove = jtag_remove,
};
device_platform_driver(jtag_driver);

MODULE_AUTHOR("Davide Rizzo <elpa.rizzo@gmail.com>");
MODULE_AUTHOR("Wjatscheslaw Stoljarski <wjatscheslaw.stoljarski@kiwigrid.com>");
MODULE_DESCRIPTION("JTAG bitbang driver");
MODULE_LICENSE("GPL");
