/*
 * Copyright (C) 2007 Atmel Corporation.
 * Copyright (C) 2011 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2
 */

#include <common.h>
#include <io.h>
#include <init.h>
#include <restart.h>

#include <mach/hardware.h>
#include <mach/cpu.h>
#include <mach/at91_dbgu.h>

#include "soc.h"
#include "generic.h"

struct at91_init_soc __initdata at91_boot_soc;

struct at91_socinfo at91_soc_initdata;
EXPORT_SYMBOL(at91_soc_initdata);

void __init at91rm9200_set_type(int type)
{
	if (type == ARCH_REVISON_9200_PQFP)
		at91_soc_initdata.subtype = AT91_SOC_RM9200_PQFP;
	else
		at91_soc_initdata.subtype = AT91_SOC_RM9200_BGA;

	pr_info("AT91: filled in soc subtype: %s\n",
		at91_get_soc_subtype(&at91_soc_initdata));
}

static void __init soc_detect(u32 dbgu_base)
{
	u32 cidr, socid;

	cidr = __raw_readl(dbgu_base + AT91_DBGU_CIDR);
	socid = cidr & ~AT91_CIDR_VERSION;

	/* sub version of soc */
	at91_soc_initdata.exid = __raw_readl(dbgu_base + AT91_DBGU_EXID);

	switch (socid) {
	case ARCH_ID_AT91RM9200:
		at91_soc_initdata.type = AT91_SOC_RM9200;
		if (at91_soc_initdata.subtype == AT91_SOC_SUBTYPE_NONE)
			at91_soc_initdata.subtype = AT91_SOC_RM9200_BGA;
		at91_boot_soc = at91rm9200_soc;
		break;

	case ARCH_ID_AT91SAM9260:
		at91_soc_initdata.type = AT91_SOC_SAM9260;
		at91_boot_soc = at91sam9260_soc;
		break;

	case ARCH_ID_AT91SAM9261:
		at91_soc_initdata.type = AT91_SOC_SAM9261;
		at91_boot_soc = at91sam9261_soc;
		break;

	case ARCH_ID_AT91SAM9263:
		at91_soc_initdata.type = AT91_SOC_SAM9263;
		at91_boot_soc = at91sam9263_soc;
		break;

	case ARCH_ID_AT91SAM9G20:
		at91_soc_initdata.type = AT91_SOC_SAM9G20;
		at91_boot_soc = at91sam9260_soc;
		break;

	case ARCH_ID_AT91SAM9G45:
		at91_soc_initdata.type = AT91_SOC_SAM9G45;
		if (cidr == ARCH_ID_AT91SAM9G45ES)
			at91_soc_initdata.subtype = AT91_SOC_SAM9G45ES;
		at91_boot_soc = at91sam9g45_soc;
		break;

	case ARCH_ID_AT91SAM9RL64:
		at91_soc_initdata.type = AT91_SOC_SAM9RL;
		at91_boot_soc = at91sam9rl_soc;
		break;

	case ARCH_ID_AT91SAM9X5:
		at91_soc_initdata.type = AT91_SOC_SAM9X5;
		break;

	case ARCH_ID_AT91SAM9N12:
		at91_soc_initdata.type = AT91_SOC_SAM9N12;
		at91_boot_soc = at91sam9n12_soc;
		break;

	case ARCH_ID_SAMA5:
		if (at91_soc_initdata.exid & ARCH_EXID_SAMA5D3) {
			at91_soc_initdata.type = AT91_SOC_SAMA5D3;
			at91_boot_soc = at91sama5d3_soc;
		} else {
			if (at91_soc_initdata.exid & ARCH_EXID_SAMA5D4) {
				at91_soc_initdata.type = AT91_SOC_SAMA5D4;
				at91_boot_soc = at91sama5d4_soc;
			}
		}
		break;
	}

	/* at91sam9g10 */
	if ((socid & ~AT91_CIDR_EXT) == ARCH_ID_AT91SAM9G10) {
		at91_soc_initdata.type = AT91_SOC_SAM9G10;
		at91_boot_soc = at91sam9261_soc;
	}
	/* at91sam9xe */
	else if ((cidr & AT91_CIDR_ARCH) == ARCH_FAMILY_AT91SAM9XE) {
		at91_soc_initdata.type = AT91_SOC_SAM9260;
		at91_soc_initdata.subtype = AT91_SOC_SAM9XE;
		at91_boot_soc = at91sam9260_soc;
	}

	if (!at91_soc_is_detected())
		return;

	at91_soc_initdata.cidr = cidr;

	if (at91_soc_initdata.type == AT91_SOC_SAM9G45) {
		switch (at91_soc_initdata.exid) {
		case ARCH_EXID_AT91SAM9M10:
			at91_soc_initdata.subtype = AT91_SOC_SAM9M10;
			break;
		case ARCH_EXID_AT91SAM9G46:
			at91_soc_initdata.subtype = AT91_SOC_SAM9G46;
			break;
		case ARCH_EXID_AT91SAM9M11:
			at91_soc_initdata.subtype = AT91_SOC_SAM9M11;
			break;
		}
	}

	if (at91_soc_initdata.type == AT91_SOC_SAM9X5) {
		switch (at91_soc_initdata.exid) {
		case ARCH_EXID_AT91SAM9G15:
			at91_soc_initdata.subtype = AT91_SOC_SAM9G15;
			break;
		case ARCH_EXID_AT91SAM9G35:
			at91_soc_initdata.subtype = AT91_SOC_SAM9G35;
			break;
		case ARCH_EXID_AT91SAM9X35:
			at91_soc_initdata.subtype = AT91_SOC_SAM9X35;
			break;
		case ARCH_EXID_AT91SAM9G25:
			at91_soc_initdata.subtype = AT91_SOC_SAM9G25;
			break;
		case ARCH_EXID_AT91SAM9X25:
			at91_soc_initdata.subtype = AT91_SOC_SAM9X25;
			break;
		}
	}

	if (at91_soc_initdata.type == AT91_SOC_SAM9N12) {
		switch (at91_soc_initdata.exid) {
		case ARCH_EXID_AT91SAM9N12:
			at91_soc_initdata.subtype = AT91_SOC_SAM9N12;
			break;
		case ARCH_EXID_AT91SAM9CN11:
			at91_soc_initdata.subtype = AT91_SOC_SAM9CN11;
			break;
		case ARCH_EXID_AT91SAM9CN12:
			at91_soc_initdata.subtype = AT91_SOC_SAM9CN12;
			break;
		}
	}

	if (at91_soc_initdata.type == AT91_SOC_SAMA5D3) {
		switch (at91_soc_initdata.exid) {
		case ARCH_EXID_SAMA5D31:
			at91_soc_initdata.subtype = AT91_SOC_SAMA5D31;
			break;
		case ARCH_EXID_SAMA5D33:
			at91_soc_initdata.subtype = AT91_SOC_SAMA5D33;
			break;
		case ARCH_EXID_SAMA5D34:
			at91_soc_initdata.subtype = AT91_SOC_SAMA5D34;
			break;
		case ARCH_EXID_SAMA5D35:
			at91_soc_initdata.subtype = AT91_SOC_SAMA5D35;
			break;
		case ARCH_EXID_SAMA5D36:
			at91_soc_initdata.subtype = AT91_SOC_SAMA5D36;
			break;
		}
	}

	if (at91_soc_initdata.type == AT91_SOC_SAMA5D4) {
		switch (at91_soc_initdata.exid) {
		case ARCH_EXID_SAMA5D41:
			at91_soc_initdata.subtype = AT91_SOC_SAMA5D41;
			break;
		case ARCH_EXID_SAMA5D42:
			at91_soc_initdata.subtype = AT91_SOC_SAMA5D42;
			break;
		case ARCH_EXID_SAMA5D43:
			at91_soc_initdata.subtype = AT91_SOC_SAMA5D43;
			break;
		case ARCH_EXID_SAMA5D44:
			at91_soc_initdata.subtype = AT91_SOC_SAMA5D44;
			break;
		}
	}
}

static const char *soc_name[] = {
	[AT91_SOC_RM9200]	= "at91rm9200",
	[AT91_SOC_SAM9260]	= "at91sam9260",
	[AT91_SOC_SAM9261]	= "at91sam9261",
	[AT91_SOC_SAM9263]	= "at91sam9263",
	[AT91_SOC_SAM9G10]	= "at91sam9g10",
	[AT91_SOC_SAM9G20]	= "at91sam9g20",
	[AT91_SOC_SAM9G45]	= "at91sam9g45",
	[AT91_SOC_SAM9RL]	= "at91sam9rl",
	[AT91_SOC_SAM9X5]	= "at91sam9x5",
	[AT91_SOC_SAM9N12]	= "at91sam9n12",
	[AT91_SOC_SAMA5D3]	= "sama5d3",
	[AT91_SOC_SAMA5D4]	= "sama5d4",
	[AT91_SOC_NONE]		= "Unknown"
};

const char *at91_get_soc_type(struct at91_socinfo *c)
{
	return soc_name[c->type];
}
EXPORT_SYMBOL(at91_get_soc_type);

static const char *soc_subtype_name[] = {
	[AT91_SOC_RM9200_BGA]	= "at91rm9200 BGA",
	[AT91_SOC_RM9200_PQFP]	= "at91rm9200 PQFP",
	[AT91_SOC_SAM9XE]	= "at91sam9xe",
	[AT91_SOC_SAM9G45ES]	= "at91sam9g45es",
	[AT91_SOC_SAM9M10]	= "at91sam9m10",
	[AT91_SOC_SAM9G46]	= "at91sam9g46",
	[AT91_SOC_SAM9M11]	= "at91sam9m11",
	[AT91_SOC_SAM9G15]	= "at91sam9g15",
	[AT91_SOC_SAM9G35]	= "at91sam9g35",
	[AT91_SOC_SAM9X35]	= "at91sam9x35",
	[AT91_SOC_SAM9G25]	= "at91sam9g25",
	[AT91_SOC_SAM9X25]	= "at91sam9x25",
	[AT91_SOC_SAM9N12]	= "at91sam9n12",
	[AT91_SOC_SAM9CN11]	= "at91sam9cn11",
	[AT91_SOC_SAM9CN12]	= "at91sam9cn12",
	[AT91_SOC_SAMA5D31]	= "sama5d31",
	[AT91_SOC_SAMA5D33]	= "sama5d33",
	[AT91_SOC_SAMA5D34]	= "sama5d34",
	[AT91_SOC_SAMA5D35]	= "sama5d35",
	[AT91_SOC_SAMA5D36]	= "sama5d36",
	[AT91_SOC_SAMA5D41]	= "sama5d41",
	[AT91_SOC_SAMA5D42]	= "sama5d42",
	[AT91_SOC_SAMA5D43]	= "sama5d43",
	[AT91_SOC_SAMA5D44]	= "sama5d44",
	[AT91_SOC_SUBTYPE_NONE]	= "Unknown"
};

const char *at91_get_soc_subtype(struct at91_socinfo *c)
{
	return soc_subtype_name[c->subtype];
}
EXPORT_SYMBOL(at91_get_soc_subtype);

static int at91_detect(void)
{
	at91_soc_initdata.type = AT91_SOC_NONE;
	at91_soc_initdata.subtype = AT91_SOC_SUBTYPE_NONE;

	soc_detect(AT91_BASE_DBGU0);
	if (!at91_soc_is_detected())
		soc_detect(AT91_BASE_DBGU1);
	if (!at91_soc_is_detected())
		soc_detect(AT91_BASE_DBGU2);

	if (!at91_soc_is_detected())
		panic("AT91: Impossible to detect the SOC type");

	pr_info("AT91: Detected soc type: %s\n",
		at91_get_soc_type(&at91_soc_initdata));
	pr_info("AT91: Detected soc subtype: %s\n",
		at91_get_soc_subtype(&at91_soc_initdata));

	if (IS_ENABLED(CONFIG_COMMON_CLK_OF_PROVIDER))
		return 0;

	if (!at91_soc_is_enabled())
		panic("AT91: Soc not enabled");

	/* Init clock subsystem */
	at91_clock_init();

	if (at91_boot_soc.init)
		at91_boot_soc.init();

	return 0;
}
postcore_initcall(at91_detect);

void restart_sam9(struct restart_handler *rst);
void restart_sam9g45(struct restart_handler *rst);

static int at91_soc_device(void)
{
	struct device_d *dev;

	dev = add_generic_device_res("soc", DEVICE_ID_SINGLE, NULL, 0, NULL);
	dev_add_param_fixed(dev, "name", (char*)at91_get_soc_type(&at91_soc_initdata));
	dev_add_param_fixed(dev, "subname", (char*)at91_get_soc_subtype(&at91_soc_initdata));

	if (IS_ENABLED(CONFIG_AT91SAM9_RESET))
		restart_handler_register_fn(restart_sam9);
	if (IS_ENABLED(CONFIG_AT91SAM9G45_RESET))
		restart_handler_register_fn(restart_sam9g45);

	return 0;
}
coredevice_initcall(at91_soc_device);
