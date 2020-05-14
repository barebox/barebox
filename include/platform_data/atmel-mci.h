#ifndef PLATFORM_DATA_ATMEL_MCI_H
#define PLATFORM_DATA_ATMEL_MCI_H

/* Multimedia Card Interface */
struct atmel_mci_platform_data {
	unsigned slot_b;
	unsigned bus_width;
	int detect_pin;
	int wp_pin;
	char *devname;
};

void at91_add_device_mci(short mmc_id, struct atmel_mci_platform_data *data);

#endif /* PLATFORM_DATA_ATMEL_MCI_H */
