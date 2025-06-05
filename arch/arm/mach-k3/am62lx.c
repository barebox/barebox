// SPDX-License-Identifier: GPL-2.0-only
#include <io.h>
#include <of.h>
#include <init.h>
#include <linux/bits.h>
#include <linux/bitfield.h>
#include <pm_domain.h>
#include <bootsource.h>
#include <mach/k3/common.h>

/* Primary BootMode devices */
#define BOOT_DEVICE_SPI_NAND		0x00
#define BOOT_DEVICE_RAM			0xFF
#define BOOT_DEVICE_OSPI		0x01
#define BOOT_DEVICE_QSPI		0x02
#define BOOT_DEVICE_SPI			0x03
#define BOOT_DEVICE_UART		0x07
#define BOOT_DEVICE_MMC			0x08
#define BOOT_DEVICE_EMMC		0x09
#define BOOT_DEVICE_USB			0x0A
#define BOOT_DEVICE_DFU			0x0A
#define BOOT_DEVICE_GPMC_NAND		0x0B
#define BOOT_DEVICE_XSPI_FAST		0x0D
#define BOOT_DEVICE_XSPI		0x0E
#define BOOT_DEVICE_NOBOOT		0x0F

/* U-Boot used aliases */
#define BOOT_DEVICE_SPINAND		0x10
#define BOOT_DEVICE_MMC2		BOOT_DEVICE_MMC
#define BOOT_DEVICE_MMC1		BOOT_DEVICE_EMMC
/* Invalid Choices for the AM62Lx */
#define BOOT_DEVICE_MMC2_2		0x1F
#define BOOT_DEVICE_ETHERNET		0x1F

/* Backup BootMode devices */
#define BACKUP_BOOT_DEVICE_USB		0x01
#define BACKUP_BOOT_DEVICE_DFU		0x01
#define BACKUP_BOOT_DEVICE_UART		0x03
#define BACKUP_BOOT_DEVICE_MMC		0x05
#define BACKUP_BOOT_DEVICE_SPI		0x06

#define K3_PRIMARY_BOOTMODE		0x0

#define MAIN_DEVSTAT_BACKUP_BOOTMODE		GENMASK(12, 10)
#define MAIN_DEVSTAT_BACKUP_BOOTMODE_CFG	BIT(13)
#define MAIN_DEVSTAT_BACKUP_USB_MODE		BIT(0)

static void am62lx_get_backup_bootsource(u32 devstat, enum bootsource *src, int *instance)
{
	u32 bkup_bootmode = FIELD_GET(MAIN_DEVSTAT_BACKUP_BOOTMODE, devstat);
	u32 bkup_bootmode_cfg = FIELD_GET(MAIN_DEVSTAT_BACKUP_BOOTMODE_CFG, devstat);

	*src = BOOTSOURCE_UNKNOWN;

	switch (bkup_bootmode) {
	case BACKUP_BOOT_DEVICE_UART:
		*src = BOOTSOURCE_SERIAL;
		return;
	case BACKUP_BOOT_DEVICE_MMC:
		if (bkup_bootmode_cfg) {
			*src = BOOTSOURCE_MMC;
			*instance = 1;
		} else {
			*src = BOOTSOURCE_MMC;
			*instance = 0;
		}
		return;
	case BACKUP_BOOT_DEVICE_SPI:
		*src = BOOTSOURCE_SPI;
		return;
	case BACKUP_BOOT_DEVICE_USB:
		if (bkup_bootmode_cfg & MAIN_DEVSTAT_BACKUP_USB_MODE)
			*src = BOOTSOURCE_USB;
		else
			*src = BOOTSOURCE_SERIAL;
		return;
	};
}

#define MAIN_DEVSTAT_PRIMARY_BOOTMODE		GENMASK(6, 3)
#define MAIN_DEVSTAT_PRIMARY_BOOTMODE_CFG	GENMASK(9, 7)
#define MAIN_DEVSTAT_PRIMARY_USB_MODE		BIT(1)
#define MAIN_DEVSTAT_PRIMARY_MMC_PORT		BIT(2)

static void am62lx_get_primary_bootsource(u32 devstat, enum bootsource *src, int *instance)
{
	u32 bootmode = FIELD_GET(MAIN_DEVSTAT_PRIMARY_BOOTMODE, devstat);
	u32 bootmode_cfg = FIELD_GET(MAIN_DEVSTAT_PRIMARY_BOOTMODE_CFG, devstat);

	switch (bootmode) {
	case BOOT_DEVICE_OSPI:
	case BOOT_DEVICE_QSPI:
	case BOOT_DEVICE_XSPI:
	case BOOT_DEVICE_SPI:
		*src = BOOTSOURCE_SPI;
		return;
	case BOOT_DEVICE_EMMC:
		*src = BOOTSOURCE_MMC;
		*instance = 0;
		return;
	case BOOT_DEVICE_MMC:
		if (bootmode_cfg & MAIN_DEVSTAT_PRIMARY_MMC_PORT) {
			*src = BOOTSOURCE_MMC;
			*instance = 1;
		} else {
			*src = BOOTSOURCE_MMC;
			*instance = 0;
		}
		return;
	case BOOT_DEVICE_USB:
		if (bootmode_cfg & MAIN_DEVSTAT_PRIMARY_USB_MODE)
			*src = BOOTSOURCE_USB;
		else
			*src = BOOTSOURCE_SERIAL;
		return;
	case BOOT_DEVICE_NOBOOT:
		*src = BOOTSOURCE_UNKNOWN;
		return;
	}
}

#define AM62LX_BOOT_PARAM_TABLE_INDEX_OCRAM		IOMEM(0x70816e70)

#define AM62LX_WKUP_CTRL_MMR0_BASE		IOMEM(0x43010000)
#define AM62LX_CTRLMMR_MAIN_DEVSTAT		(AM62LX_WKUP_CTRL_MMR0_BASE + 0x30)

void am62lx_get_bootsource(enum bootsource *src, int *instance)
{
	u32 bootmode = readl(AM62LX_BOOT_PARAM_TABLE_INDEX_OCRAM);
	u32 devstat;

	devstat = readl(AM62LX_CTRLMMR_MAIN_DEVSTAT);

	if (bootmode == K3_PRIMARY_BOOTMODE)
		am62lx_get_primary_bootsource(devstat, src, instance);
	else
		am62lx_get_backup_bootsource(devstat, src, instance);
}

static int am62lx_init(void)
{
	enum bootsource src = BOOTSOURCE_UNKNOWN;
	int instance = 0;

	if (!of_machine_is_compatible("ti,am62l3"))
		return 0;

	am62lx_get_bootsource(&src, &instance);
	bootsource_set(src, instance);

	genpd_activate();

	return 0;
}
postcore_initcall(am62lx_init);
