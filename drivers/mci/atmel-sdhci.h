// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2020 Ahmad Fatoum, Pengutronix

#ifndef ATMEL_SDHCI_H_
#define ATMEL_SDHCI_H_

#include <linux/types.h>
#include <mci.h>

#include "sdhci.h"

struct at91_sdhci {
	struct sdhci	sdhci;
	struct device_d *dev;
	void __iomem	*base;
	u32		caps_max_clock;
};

int at91_sdhci_init(struct at91_sdhci *host, u32 maxclk,
		    bool force_cd, bool cal_always_on);
void at91_sdhci_mmio_init(struct at91_sdhci *host, void __iomem *base);
int at91_sdhci_send_command(struct at91_sdhci *host, struct mci_cmd *sd_cmd,
			    struct mci_data *data);
bool at91_sdhci_is_card_inserted(struct at91_sdhci *host);
void at91_sdhci_host_capability(struct at91_sdhci *host,
				unsigned int *voltages);
int at91_sdhci_set_ios(struct at91_sdhci *host, struct mci_ios *ios);

#endif
