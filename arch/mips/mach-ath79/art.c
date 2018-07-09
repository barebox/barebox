// SPDX-License-Identifier: GPL-2.
/*
 * Copyright (c) 2018 Oleksij Rempel <linux@rempel-privat.de>
 */

#include <common.h>
#include <fcntl.h>
#include <init.h>
#include <libfile.h>
#include <net.h>
#include <unistd.h>

#define AR93000_EPPROM_OFFSET	0x1000

struct ar9300_eeprom {
	u8 eeprom_version;
	u8 template_version;
	u8 mac_addr[6];
};

static int art_set_mac(struct device_d *dev, struct ar9300_eeprom *eeprom)
{
	struct device_node *node = dev->device_node;
	struct device_node *rnode;

	if (!node)
		return -ENOENT;

	rnode = of_parse_phandle_from(node, NULL,
				     "barebox,provide-mac-address", 0);
	if (!rnode)
		return -ENOENT;

	of_eth_register_ethaddr(rnode, &eeprom->mac_addr[0]);

	return 0;
}

static int art_read_mac(struct device_d *dev, const char *file)
{
	int fd, rbytes;
	struct ar9300_eeprom eeprom;

	fd = open_and_lseek(file, O_RDONLY, AR93000_EPPROM_OFFSET);
	if (fd < 0) {
		dev_err(dev, "Failed to open eeprom path %s %d\n",
		       file, fd);
		return fd;
	}

	rbytes = read_full(fd, &eeprom, sizeof(eeprom));
	close(fd);
	if (rbytes < sizeof(eeprom)) {
		dev_err(dev, "Failed to read %s\n", file);
		return rbytes < 0 ? rbytes : -EIO;
	}

	dev_dbg(dev, "ART version: %x.%x\n",
		 eeprom.eeprom_version, eeprom.template_version);
	dev_dbg(dev, "mac: %02x:%02x:%02x:%02x:%02x:%02x\n",
	       eeprom.mac_addr[0],
	       eeprom.mac_addr[1],
	       eeprom.mac_addr[2],
	       eeprom.mac_addr[3],
	       eeprom.mac_addr[4],
	       eeprom.mac_addr[5]);

	if (!is_valid_ether_addr(&eeprom.mac_addr[0])) {
		dev_err(dev, "bad MAC addr\n");
		return -EILSEQ;
	}

	return art_set_mac(dev, &eeprom);
}

static int art_probe(struct device_d *dev)
{
	char *path;
	int ret;

	dev_dbg(dev, "found ART partition\n");

	ret = of_find_path(dev->device_node, "device-path", &path, 0);
	if (ret) {
		dev_err(dev, "can't find path\n");
		return ret;
	}

	return art_read_mac(dev, path);
}

static struct of_device_id art_dt_ids[] = {
	{
		.compatible = "qca,art",
	}, {
		/* sentinel */
	}
};

static struct driver_d art_driver = {
	.name		= "qca-art",
	.probe		= art_probe,
	.of_compatible	= art_dt_ids,
};

static int art_of_driver_init(void)
{
	platform_driver_register(&art_driver);

	return 0;
}
late_initcall(art_of_driver_init);
