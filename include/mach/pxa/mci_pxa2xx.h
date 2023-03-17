/* SPDX-License-Identifier: GPL-2.0-only */


struct mci_host;
struct device;

struct pxamci_platform_data {
	int gpio_power;
	int gpio_power_invert;
	int (*init)(struct mci_host*, struct device*);
	int (*setpower)(struct mci_host*, int on);
};
