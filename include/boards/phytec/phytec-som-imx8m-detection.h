/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2020 PHYTEC Messtechnik GmbH
 * Author: Teresa Remmet <t.remmet@phytec.de>
 */

#ifndef _BOARDS_PHYTEC_PHYTEC_SOM_IMX8M_DETECTION_H
#define _BOARDS_PHYTEC_PHYTEC_SOM_IMX8M_DETECTION_H

#include <common.h>
#include <boards/phytec/phytec-som-detection.h>

enum phytec_imx8m_ddr_size {
	PHYTEC_IMX8M_DDR_AUTODETECT = 0,
	PHYTEC_IMX8M_DDR_1G = 1,
	PHYTEC_IMX8M_DDR_2G = 3,
	PHYTEC_IMX8M_DDR_4G = 5,
};

int phytec_imx8m_detect(u8 som, const char *opt, u8 cpu_type);
enum phytec_imx8m_ddr_size phytec_get_imx8m_ddr_size(const struct phytec_eeprom_data *data);
u8 phytec_get_imx8mp_rtc(const struct phytec_eeprom_data *data);
u8 phytec_get_imx8m_spi(const struct phytec_eeprom_data *data);
u8 phytec_get_imx8m_eth(const struct phytec_eeprom_data *data);

#endif /* _BOARDS_PHYTEC_PHYTEC_SOM_IMX8M_DETECTION_H */
