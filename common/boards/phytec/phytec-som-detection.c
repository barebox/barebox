// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2020 PHYTEC Messtechnik GmbH
 * Author: Teresa Remmet <t.remmet@phytec.de>
 */

#include <boards/phytec/phytec-som-imx8m-detection.h>
#include <common.h>
#include <pbl/eeprom.h>

struct phytec_eeprom_data eeprom_data;

#define POLY (0x1070U << 3)

static u8 _crc8(u16 data)
{
	int i;

	for (i = 0; i < 8; i++) {
		if (data & 0x8000)
			data = data ^ POLY;
		data = data << 1;
	}

	return data >> 8;
}

static unsigned int crc8(unsigned int crc, const u8 *vptr, int len)
{
	int i;

	for (i = 0; i < len; i++)
		crc = _crc8((crc ^ vptr[i]) << 8);

	return crc;
}

const char *phytec_get_opt(const struct phytec_eeprom_data *data)
{
	const char *opt;

	if (!data)
		data = &eeprom_data;

	switch (data->api_rev) {
	case PHYTEC_API_REV0:
	case PHYTEC_API_REV1:
		opt = data->data.data_api0.opt;
		break;
	case PHYTEC_API_REV2:
		opt = data->data.data_api2.opt;
		break;
	default:
		opt = NULL;
		break;
	};

	return opt;
}

static int phytec_eeprom_data_init(struct pbl_i2c *i2c,
				   struct phytec_eeprom_data *data,
				   int addr, u8 phytec_som_type)
{
	unsigned int crc;
	const char *opt;
	int *ptr;
	int ret = -1, i;
	u8 som;

	if (!data)
		data = &eeprom_data;

	eeprom_read(i2c, addr, I2C_ADDR_16_BIT, data, sizeof(struct phytec_eeprom_data));

	if (data->api_rev == 0xff) {
		pr_err("%s: EEPROM is not flashed. Prototype?\n", __func__);
		return -EINVAL;
	}

	for (i = 0, ptr = (int *)data;
	     i < sizeof(struct phytec_eeprom_data);
	     i += sizeof(ptr), ptr++)
		if (*ptr != 0x0)
			break;

	if (i == sizeof(struct phytec_eeprom_data)) {
		pr_err("%s: EEPROM data is all zero. Erased?\n", __func__);
		return -EINVAL;
	}

	if (data->api_rev > PHYTEC_API_REV2) {
		pr_err("%s: EEPROM API revision %u not supported\n",
		       __func__, data->api_rev);
		return -EINVAL;
	}

	/* We are done here for early revisions */
	if (data->api_rev <= PHYTEC_API_REV1)
		return 0;

	crc = crc8(0, (const unsigned char *)data,
		   sizeof(struct phytec_eeprom_data));
	pr_debug("%s: crc: %x\n", __func__, crc);

	if (crc) {
		pr_err("%s: CRC mismatch. EEPROM data is not usable\n", __func__);
		return -EINVAL;
	}

	som = data->data.data_api2.som_no;
	pr_debug("%s: som id: %u\n", __func__, som);
	opt = phytec_get_opt(data);
	if (!opt)
		return -EINVAL;

	if (IS_ENABLED(CONFIG_BOARD_PHYTEC_SOM_IMX8M_DETECTION))
		ret = phytec_imx8m_detect(som, opt, phytec_som_type);

	if (ret) {
		pr_err("%s: SoM ID does not match. Wrong EEPROM data?\n", __func__);
		return -EINVAL;
	}

	return 0;
}

void phytec_print_som_info(const struct phytec_eeprom_data *data)
{
	const struct phytec_api2_data *api2;
	char pcb_sub_rev;
	unsigned int ksp_no, sub_som_type1 = -1, sub_som_type2 = -1;

	if (!data)
		data = &eeprom_data;

	if (data->api_rev < PHYTEC_API_REV2)
		return;

	api2 = &data->data.data_api2;

	/* Calculate PCB subrevision */
	pcb_sub_rev = api2->pcb_sub_opt_rev & 0x0f;
	pcb_sub_rev = pcb_sub_rev ? ((pcb_sub_rev - 1) + 'a') : ' ';

	/* print standard product string */
	if (api2->som_type <= 1) {
		pr_info("SoM: %s-%03u-%s.%s PCB rev: %u%c\n",
			phytec_som_type_str[api2->som_type], api2->som_no,
			api2->opt, api2->bom_rev, api2->pcb_rev, pcb_sub_rev);
		return;
	}
	/* print KSP/KSM string */
	if (api2->som_type <= 3) {
		ksp_no = (api2->ksp_no << 8) | api2->som_no;
		pr_info("SoM: %s-%u ",
			phytec_som_type_str[api2->som_type], ksp_no);
		/* print standard product based KSP/KSM strings */
	} else {
		switch (api2->som_type) {
		case 4:
			sub_som_type1 = 0;
			sub_som_type2 = 3;
			break;
		case 5:
			sub_som_type1 = 0;
			sub_som_type2 = 2;
			break;
		case 6:
			sub_som_type1 = 1;
			sub_som_type2 = 3;
			break;
		case 7:
			sub_som_type1 = 1;
			sub_som_type2 = 2;
			break;
		default:
			break;
		};

		pr_info("SoM: %s-%03u-%s-%03u ",
			phytec_som_type_str[sub_som_type1],
			api2->som_no, phytec_som_type_str[sub_som_type2],
			api2->ksp_no);
	}

	printf("Option: %s BOM rev: %s PCB rev: %u%c\n", api2->opt,
	       api2->bom_rev, api2->pcb_rev, pcb_sub_rev);
}

int phytec_eeprom_data_setup(struct pbl_i2c *i2c, struct phytec_eeprom_data *data,
			     int addr, int addr_fallback, u8 cpu_type)
{
	int ret;

	ret = phytec_eeprom_data_init(i2c, data, addr, cpu_type);
	if (ret) {
		pr_err("%s: init failed. Trying fall back address 0x%x\n",
		       __func__, addr_fallback);
		ret = phytec_eeprom_data_init(i2c, data, addr_fallback, cpu_type);
	}

	if (ret)
		pr_err("%s: EEPROM data init failed\n", __func__);
	else
		pr_debug("%s: init successful\n", __func__);

	return ret;
}
