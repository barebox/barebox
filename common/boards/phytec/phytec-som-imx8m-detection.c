// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2020 PHYTEC Messtechnik GmbH
 * Author: Teresa Remmet <t.remmet@phytec.de>
 */

#include <boards/phytec/phytec-som-imx8m-detection.h>
#include <common.h>
#include <mach/imx/generic.h>

extern struct phytec_eeprom_data eeprom_data;

/* Check if the SoM is actually one of the following products:
 * - i.MX8MM
 * - i.MX8MN
 * - i.MX8MP
 * - i.MX8MQ
 *
 * Returns 0 in case it's a known SoM. Otherwise, returns -errno.
 */
int phytec_imx8m_detect(u8 som, const char *opt, u8 cpu_type)
{
	if (som == PHYTEC_IMX8MP_SOM && cpu_type == IMX_CPU_IMX8MP)
		return 0;

	if (som == PHYTEC_IMX8MM_SOM) {
		if (((opt[0] - '0') != 0) &&
		    ((opt[1] - '0') == 0) && cpu_type == IMX_CPU_IMX8MM)
			return 0;
		else if (((opt[0] - '0') == 0) &&
			 ((opt[1] - '0') != 0) && cpu_type == IMX_CPU_IMX8MN)
			return 0;
		return -EINVAL;
	}

	if (som == PHYTEC_IMX8MQ_SOM && cpu_type == IMX_CPU_IMX8MQ)
		return 0;

	return -EINVAL;
}

/*
 * So far all PHYTEC i.MX8M boards have RAM size definition at the
 * same location.
 */
enum phytec_imx8m_ddr_size phytec_get_imx8m_ddr_size(const struct phytec_eeprom_data *data)
{
	const char *opt;
	u8 ddr_id;

	if (!data)
		data = &eeprom_data;

	opt = phytec_get_opt(data);
	if (opt)
		ddr_id = opt[2] - '0';
	else
		ddr_id = PHYTEC_IMX8M_DDR_AUTODETECT;

	pr_debug("%s: ddr id: %u\n", __func__, ddr_id);

	return ddr_id;
}

/*
 * Filter SPI-NOR flash information. All i.MX8M boards have this at
 * the same location.
 * returns: 0x0 if no SPI is populated. Otherwise a board depended
 * code for the size. PHYTEC_EEPROM_INVAL when the data is invalid.
 */
u8 phytec_get_imx8m_spi(const struct phytec_eeprom_data *data)
{
	const char *opt;
	u8 spi;

	if (!data)
		data = &eeprom_data;

	if (data->api_rev < PHYTEC_API_REV2)
		return PHYTEC_EEPROM_INVAL;

	opt = phytec_get_opt(data);
	if (opt)
		spi = opt[4] - '0';
	else
		spi = PHYTEC_EEPROM_INVAL;

	pr_debug("%s: spi: %u\n", __func__, spi);

	return spi;
}

/*
 * Filter ethernet phy information. All i.MX8M boards have this at
 * the same location.
 * returns: 0x0 if no ethernet phy is poulated. 0x1 if it is populated.
 * PHYTEC_EEPROM_INVAL when the data is invalid.
 */
u8 phytec_get_imx8m_eth(const struct phytec_eeprom_data *data)
{
	const char *opt;
	u8 eth;

	if (!data)
		data = &eeprom_data;

	if (data->api_rev < PHYTEC_API_REV2)
		return PHYTEC_EEPROM_INVAL;

	opt = phytec_get_opt(data);
	if (opt) {
		eth = opt[5] - '0';
		eth &= 0x1;
	} else {
		eth = PHYTEC_EEPROM_INVAL;
	}

	pr_debug("%s: eth: %u\n", __func__, eth);

	return eth;
}

/*
 * Filter RTC information.
 * returns: 0 if no RTC is poulated. 1 if it is populated.
 * PHYTEC_EEPROM_INVAL when the data is invalid.
 */
u8 phytec_get_imx8mp_rtc(const struct phytec_eeprom_data *data)
{
	const char *opt;
	u8 rtc;

	if (!data)
		data = &eeprom_data;

	if (data->api_rev < PHYTEC_API_REV2)
		return PHYTEC_EEPROM_INVAL;

	opt = phytec_get_opt(data);
	if (opt) {
		rtc = opt[5] - '0';
		rtc &= 0x4;
		rtc = !(rtc >> 2);
	} else {
		rtc = PHYTEC_EEPROM_INVAL;
	}

	pr_debug("%s: rtc: %u\n", __func__, rtc);

	return rtc;
}
