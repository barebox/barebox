// SPDX-License-Identifier: GPL-2.0-only

#include <mci.h>

struct omap_hsmmc {
	struct mci_host		mci;
	struct device		*dev;
	struct hsmmc		*base;
	void __iomem		*iobase;
};

int omap_hsmmc_init(struct omap_hsmmc *hsmmc);

int omap_hsmmc_send_cmd(struct omap_hsmmc *hsmmc, struct mci_cmd *cmd,
			struct mci_data *data);

void omap_hsmmc_set_ios(struct omap_hsmmc *hsmmc, struct mci_ios *ios);
