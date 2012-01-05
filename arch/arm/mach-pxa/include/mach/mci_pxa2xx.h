
struct mci_host;
struct device_d;

struct pxamci_platform_data {
	int gpio_power;
	int gpio_power_invert;
	int (*init)(struct mci_host*, struct device_d*);
	int (*setpower)(struct mci_host*, int on);
};
