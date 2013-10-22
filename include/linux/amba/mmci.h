/*
 *  include/linux/amba/mmci.h
 */
#ifndef AMBA_MMCI_H
#define AMBA_MMCI_H

/*
 * These defines is places here due to access is needed from machine
 * configuration files. The ST Micro version does not have ROD and
 * reuse the voltage registers for direction settings.
 */
#define MCI_ST_DATA2DIREN	(1 << 2)
#define MCI_ST_CMDDIREN		(1 << 3)
#define MCI_ST_DATA0DIREN	(1 << 4)
#define MCI_ST_DATA31DIREN	(1 << 5)
#define MCI_ST_FBCLKEN		(1 << 7)
#define MCI_ST_DATA74DIREN	(1 << 8)

#define SDI_CLKCR_CLKDIV_INIT	0x000000FD

/**
 * struct mmci_platform_data - platform configuration for the MMCI
 * (also known as PL180) block.
 * @f_max: the maximum operational frequency for this host in this
 * platform configuration. When this is specified it takes precedence
 * over the module parameter for the same frequency.
 * @ocr_mask: available voltages on the 4 pins from the block, this
 * is ignored if a regulator is used, see the MMC_VDD_* masks in
 * mmc/host.h
 * @capabilities: the capabilities of the block as implemented in
 * this platform, signify anything MMC_CAP_* from mmc/host.h
 */
struct mmci_platform_data {
	unsigned long f_max;
	unsigned int ocr_mask;
	unsigned long capabilities;

	uint32_t sigdir;
	uint32_t clkdiv_init;
};

#endif
