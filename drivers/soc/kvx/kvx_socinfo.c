// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 Kalray Inc., Clément Léger
 */

#define pr_fmt(fmt) "kvx_socinfo: " fmt

#include <init.h>
#include <driver.h>
#include <globalvar.h>
#include <magicvar.h>
#include <command.h>
#include <libfile.h>
#include <getopt.h>
#include <common.h>
#include <fs.h>

#include <asm/sfr.h>

#include <linux/kernel.h>
#include <linux/nvmem-consumer.h>

#define LOT_ID_STR_LEN	8

#define EWS_LOT_ID_MASK		0x1ffffffffffULL
#define EWS_WAFER_ID_SHIFT	42
#define EWS_WAFER_ID_MASK	0x1fULL

#define FT_COM_AP_SHIFT		16
#define FT_COM_AP_MASK		0x3f
#define FT_DEVICE_ID_SHIFT	22
#define FT_DEVICE_ID_MASK	0x1ff

static char *kvx_mppa_id;
static char *kvx_arch_rev;

BAREBOX_MAGICVAR(kvx.arch_rev, "KVX architecture revision");
BAREBOX_MAGICVAR(kvx.mppa_id, "KVX MPPA chip id");

static void kvx_soc_info_read_revision(void)
{
	u64 pcr = kvx_sfr_get(PCR);
	u8 sv = kvx_sfr_field_val(pcr, PCR, SV);
	u8 car = kvx_sfr_field_val(pcr, PCR, CAR);
	const char *car_str = "", *ver_str = "";

	switch (car) {
	case 0:
		car_str = "kv3";
		break;
	}

	switch (sv) {
	case 0:
		ver_str = "1";
		break;
	case 1:
		ver_str = "2";
		break;
	}

	kvx_arch_rev = basprintf("%s-%s", car_str, ver_str);

	globalvar_add_simple_string("kvx.arch_rev", &kvx_arch_rev);
}

static int base38_decode(char *s, u64 val, int nb_char)
{
	int i;
	const char *alphabet = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_?";
	const int base = strlen(alphabet);

	if (s == NULL)
		return -1;

	for (i = 0; i < nb_char; i++) {
		s[i] = alphabet[val % base];
		val /= base;
	}

	return 0;
}

static int kvx_read_serial(struct device_node *socinfo)
{
	char lot_id[LOT_ID_STR_LEN + 1] = "";
	char com_ap;
	u64 *cell_val64;
	u64 ews_val;
	u32 *cell_val32;
	u32 ft_val;
	u8 wafer_id;
	u16 device_id;

	cell_val64 = (u64 *) nvmem_cell_get_and_read(socinfo, "ews_fuse", 8);
	if (IS_ERR(cell_val64)) {
		pr_debug("Fail to read ews_fuse\n");
		return PTR_ERR(cell_val64);
	}

	ews_val = *cell_val64;
	ews_val = (ews_val >> 32) | (ews_val << 32);
	wafer_id = (ews_val >> EWS_WAFER_ID_SHIFT) & EWS_WAFER_ID_MASK;
	base38_decode(lot_id, ews_val & EWS_LOT_ID_MASK, LOT_ID_STR_LEN);

	cell_val32 = (u32 *) nvmem_cell_get_and_read(socinfo, "ft_fuse", 4);
	if (IS_ERR(cell_val32)) {
		pr_debug("Fail to read ft_fuse\n");
		return PTR_ERR(cell_val32);
	}

	ft_val = *cell_val32;
	device_id = (ft_val >> FT_DEVICE_ID_SHIFT) & FT_DEVICE_ID_MASK;
	base38_decode(&com_ap, (ft_val >> FT_COM_AP_SHIFT) & FT_COM_AP_MASK, 1);

	kvx_mppa_id = basprintf("%sA-%d%c-%03d", lot_id, wafer_id, com_ap,
			       device_id);

	globalvar_add_simple_string("kvx.mppa_id", &kvx_mppa_id);

	return 0;
}

static int kvx_socinfo_probe(struct device *dev)
{
	kvx_soc_info_read_revision();

	return kvx_read_serial(dev->of_node);
}

static const struct of_device_id kvx_socinfo_dt_ids[] = {
	{ .compatible = "kalray,kvx-socinfo" },
	{ }
};
MODULE_DEVICE_TABLE(of, kvx_socinfo_dt_ids);

static struct driver kvx_socinfo_driver = {
	.name = "kvx-socinfo",
	.probe = kvx_socinfo_probe,
	.of_compatible = DRV_OF_COMPAT(kvx_socinfo_dt_ids),
};
coredevice_platform_driver(kvx_socinfo_driver);
