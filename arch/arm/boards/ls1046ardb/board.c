// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <init.h>
#include <bbu.h>
#include <net.h>
#include <crc.h>
#include <fs.h>
#include <envfs.h>
#include <libfile.h>
#include <asm/memory.h>
#include <linux/sizes.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <asm/system.h>
#include <mach/layerscape.h>
#include <mach/bbu.h>

#define MAX_NUM_PORTS 16
struct nxid {
	u8 id[4];         /* 0x00 - 0x03 EEPROM Tag 'NXID' */
	u8 sn[12];        /* 0x04 - 0x0F Serial Number */
	u8 errata[5];     /* 0x10 - 0x14 Errata Level */
	u8 date[6];       /* 0x15 - 0x1a Build Date */
	u8 res_0;         /* 0x1b        Reserved */
	u32 version;      /* 0x1c - 0x1f NXID Version */
	u8 tempcal[8];    /* 0x20 - 0x27 Temperature Calibration Factors */
	u8 tempcalsys[2]; /* 0x28 - 0x29 System Temperature Calibration Factors */
	u8 tempcalflags;  /* 0x2a        Temperature Calibration Flags */
	u8 res_1[21];     /* 0x2b - 0x3f Reserved */
	u8 mac_count;     /* 0x40        Number of MAC addresses */
	u8 mac_flag;      /* 0x41        MAC table flags */
	u8 mac[MAX_NUM_PORTS][6];     /* 0x42 - 0xa1 MAC addresses */
	u8 res_2[90];     /* 0xa2 - 0xfb Reserved */	
	u32 crc;          /* 0xfc - 0xff CRC32 checksum */
} __packed;

static int nxid_is_valid(struct nxid *nxid)
{
	unsigned char id[] = { 'N', 'X', 'I', 'D' };
	int i;

	for (i = 0; i < ARRAY_SIZE(id); i++)
		if (nxid->id[i] != id[i])
			return false;
	return true;
}

static struct nxid *nxp_nxid_read(const char *filename, unsigned int offset)
{
	struct nxid *nxid;
	int fd, ret, i, n;
	struct device_node *root;
	u32 crc, crc_expected;

	nxid = xzalloc(sizeof(*nxid));

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		ret = -errno;
		goto out;
	}

	ret = pread(fd, nxid, sizeof(*nxid), offset);
	if (ret < 0) {
		close(fd);
		goto out;
	}

	if (!nxid_is_valid(nxid)) {
		ret = -ENODEV;
		goto out;
	}

	crc = crc32(0, nxid, 256 - 4);
	crc_expected = be32_to_cpu(nxid->crc);
	if (crc != crc_expected) {
		pr_err("CRC mismatch (%08x != %08x)\n", crc, crc_expected);
		ret = -EINVAL;
		goto out;
	}

	root = of_get_root_node();

	i = n = 0;

	/*
	 * The MAC addresses in the nxid structure are in the order of enabled
	 * network interfaces. We have to find the network interfaces by their
	 * aliases and skip assigning MAC addresses to disabled devices.
	 */
	while (1) {
		struct device_node *np;
		char alias[sizeof("ethernetxxx")];

		sprintf(alias, "ethernet%d", n);

		np = of_find_node_by_alias(root, alias);
		if (!np)
			goto out;

		n++;
		if (!of_device_is_available(np))
			continue;

		of_eth_register_ethaddr(np, (char *)nxid->mac[i]);

		i++;

		if (i == nxid->mac_count)
			break;
	}

	ret = 0;
out:
	if (ret) {
		free(nxid);
		nxid = ERR_PTR(ret);
	}

	return nxid;
}

static int rdb_late_init(void)
{
	nxp_nxid_read("/dev/eeprom", 256);

	return 0;
}
late_initcall(rdb_late_init);

static int rdb_mem_init(void)
{
	int ret;

	if (!of_machine_is_compatible("fsl,ls1046a-rdb"))
		return 0;

	arm_add_mem_device("ram0", 0x80000000, 0x80000000);
	arm_add_mem_device("ram1", 0x880000000, 3ULL * SZ_2G);

	ret = ls1046a_ppa_init(0x100000000 - SZ_64M, SZ_64M);
	if (ret)
		pr_err("Failed to initialize PPA firmware: %s\n", strerror(-ret));

	return 0;
}
mem_initcall(rdb_mem_init);

static int rdb_postcore_init(void)
{
	if (!of_machine_is_compatible("fsl,ls1046a-rdb"))
		return 0;

	defaultenv_append_directory(defaultenv_ls1046ardb);

	ls1046a_bbu_mmc_register_handler("sd", "/dev/mmc0.barebox",
					 BBU_HANDLER_FLAG_DEFAULT);

	return 0;
}

postcore_initcall(rdb_postcore_init);
