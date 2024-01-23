// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2015 Jason Cobham <cobham.jason@gmail.com>

/* Board specific file for the Digi ConnectCore ccxmx53 SoM */

#include <common.h>
#include <init.h>
#include <linux/sizes.h>
#include <i2c/i2c.h>
#include <gpio.h>

#include <asm/mach-types.h>
#include <mach/imx/imx5.h>
#include <mach/imx/generic.h>
#include <mach/imx/imx53-regs.h>
#include <mach/imx/esdctl.h>
#include <asm/armlinux.h>
#include <mach/imx/bbu.h>
#include <mach/imx/iim.h>


struct ccwmx53_ident {
	const char	*id_string;
	const int	mem_sz;
	const char	industrial;
	const char	eth0;
	const char	eth1;
	const char	wless;
};

static struct ccwmx53_ident ccwmx53_ids[] = {
	/* 0x00 - 5500xxxx-xx */
	{ "Unknown",						0,		0, 0, 0, 0},
	/* 0x01 - 5500xxxx-xx */
	{ "Not supported",					0,		0, 0, 0, 0},
	/* 0x02 - 55001604-01 */
	{ "i.MX535@1000MHz, Wireless, PHY, Ext. Eth, Accel",	SZ_512M,	0, 1, 1, 1},
	/* 0x03 - 55001605-01 */
	{ "i.MX535@1000MHz, PHY, Accel",			SZ_512M,	0, 1, 0, 0},
	/* 0x04 - 55001604-02 */
	{ "i.MX535@1000MHz, Wireless, PHY, Ext. Eth, Accel",	SZ_512M,	0, 1, 1, 1},
	/* 0x05 - 5500xxxx-xx */
	{ "i.MX535@1000MHz, PHY, Ext. Eth, Accel",		SZ_512M,	0, 1, 1, 0},
	/* 0x06 - 55001604-03 */
	{ "i.MX535@1000MHz, Wireless, PHY, Accel",		SZ_512M,	0, 1, 0, 1},
	/* 0x07 - 5500xxxx-xx */
	{ "i.MX535@1000MHz, PHY, Accel",			SZ_512M,	0, 1, 0, 0},
	/* 0x08 - 55001604-04 */
	{ "i.MX537@800MHz, Wireless, PHY, Accel",		SZ_512M,	1, 1, 0, 1},
	/* 0x09 - 55001605-02 */
	{ "i.MX537@800MHz, PHY, Accel",				SZ_512M,	1, 1, 0, 0},
	/* 0x0a - 5500xxxx-xx */
	{ "i.MX537@800MHz, Wireless, PHY, Ext. Eth, Accel",	SZ_512M,	1, 1, 1, 1},
	/* 0x0b - 55001605-03 */
	{ "i.MX537@800MHz, PHY, Ext. Eth, Accel",		SZ_1G,		1, 1, 1, 0},
	/* 0x0c - 5500xxxx-xx */
	{ "Reserved for future use",				0,		0, 0, 0, 0},
	/* 0x0d - 55001605-05 */
	{ "i.MX537@800MHz, PHY, Accel",				SZ_1G,		1, 1, 0, 0},
	/* 0x0e - 5500xxxx-xx */
	{ "Reserved for future use",				0,		0, 0, 0, 0},
	/* 0x0f - 5500xxxx-xx */
	{ "Reserved for future use",				0,		0, 0, 0, 0},
};

static struct ccwmx53_ident *ccwmx53_id;

/* On these boards, memory information is encoded in the MAC address.
 * Print device memory, and option info from lookup table.
 * */
static int ccwmx53_devices_init(void)
{
	u8 hwid[6] = {0};
	char manloc = 0;

	if (!of_machine_is_compatible("digi,imx53-ccxmx53"))
		return 0;

	if ((imx_iim_read(1, 9, hwid, sizeof(hwid)) != sizeof(hwid)) ||
	    (hwid[0] < 0x02) ||
	    (hwid[0] >= ARRAY_SIZE(ccwmx53_ids))) {
		printf("Module Variant: Unknown (0x%02x) (0x%02x) (0x%02x) (0x%02x) (0x%02x) (0x%02x)\n",
		       hwid[0], hwid[1], hwid[2], hwid[3], hwid[4], hwid[5]);
		memset(hwid, 0x00, sizeof(hwid));
	}

	ccwmx53_id = &ccwmx53_ids[hwid[0]];
	printf("Module Variant: %s (0x%02x)\n", ccwmx53_id->id_string, hwid[0]);

	if (hwid[0]) {
		printf("Module HW Rev : %02x\n", hwid[1] + 1);
		switch (hwid[2] & 0xc0) {
		case 0x00:
			manloc = 'B';
			break;
		case 0x40:
			manloc = 'W';
			break;
		case 0x80:
			manloc = 'S';
			break;
		default:
			manloc = 'N';
			break;
		}

		printf("Module Serial : %c%d\n", manloc,
		       ((hwid[2] & 0x3f) << 24) |
		       (hwid[3] << 16) |
		       (hwid[4] << 8) |
		       hwid[5]);

		printf("Module RAM    : %dK\n", (ccwmx53_id->mem_sz) / 1024);

	} else {
		return -ENOSYS;
	}

	armlinux_set_architecture(MACH_TYPE_CCWMX53);

	return 0;
}
device_initcall(ccwmx53_devices_init);

static int ccxmx53_init(void)
{
	unsigned char value = 0;
	struct i2c_adapter *adapter = NULL;
	struct i2c_client client;
	int addr = 0x68;	/* da9053 device address is 0x68 */
	int bus = 0;	/* I2C0 bus */

	if (!of_machine_is_compatible("digi,imx53-ccxmx53"))
			return 0;

	adapter = i2c_get_adapter(bus);
	if (adapter) {
		client.adapter = adapter;
		client.addr = addr;
		/* Enable 3.3V ext regulator. */
		value = 0xfa;
		if (i2c_write_reg(&client, 0x19, &value, 1) < 0) {
			printf("Can't set regulator. I2C write failed\n");
			return -ENOSYS;
		}
	} else {
		printf("Can't set regulator. No I2C Adapter\n");
		return -ENOSYS;
	}

	armlinux_set_architecture(MACH_TYPE_CCMX53);

	barebox_set_model("Digi CCMX53");
	barebox_set_hostname("ccxmx53");

	imx53_bbu_internal_nand_register_handler("nand",
		BBU_HANDLER_FLAG_DEFAULT, SZ_512K);

	return 0;
}
late_initcall(ccxmx53_init);

static int ccxmx53_postcore_init(void)
{
	if (!of_machine_is_compatible("digi,imx53-ccxmx53"))
		return 0;

	imx53_init_lowlevel(800);

	return 0;
}
postcore_initcall(ccxmx53_postcore_init);
