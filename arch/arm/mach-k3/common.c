// SPDX-License-Identifier: GPL-2.0-only
#include <of.h>
#include <io.h>
#include <deep-probe.h>
#include <init.h>
#include <envfs.h>
#include <fs.h>
#include <stdio.h>
#include <xfuncs.h>
#include <malloc.h>
#include <pm_domain.h>
#include <bootsource.h>
#include <linux/bits.h>
#include <linux/bitfield.h>
#include <mach/k3/common.h>

static const struct of_device_id k3_of_match[] = {
	{
		.compatible = "ti,am625",
	},
	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(k3_of_match);

/* Primary BootMode devices */
#define BOOT_DEVICE_RAM                 0x00
#define BOOT_DEVICE_OSPI                0x01
#define BOOT_DEVICE_QSPI                0x02
#define BOOT_DEVICE_SPI                 0x03
#define BOOT_DEVICE_ETHERNET_RGMII      0x04
#define BOOT_DEVICE_ETHERNET_RMII       0x05
#define BOOT_DEVICE_I2C                 0x06
#define BOOT_DEVICE_UART                0x07
#define BOOT_DEVICE_MMC                 0x08
#define BOOT_DEVICE_EMMC                0x09

#define BOOT_DEVICE_USB                 0x0A
#define BOOT_DEVICE_GPMC_NAND           0x0B
#define BOOT_DEVICE_GPMC_NOR            0x0C
#define BOOT_DEVICE_XSPI                0x0E
#define BOOT_DEVICE_NOBOOT              0x0F

/* Backup BootMode devices */
#define BACKUP_BOOT_DEVICE_USB          0x01
#define BACKUP_BOOT_DEVICE_UART         0x03
#define BACKUP_BOOT_DEVICE_ETHERNET     0x04
#define BACKUP_BOOT_DEVICE_MMC          0x05
#define BACKUP_BOOT_DEVICE_SPI          0x06
#define BACKUP_BOOT_DEVICE_I2C          0x07

#define K3_PRIMARY_BOOTMODE             0x0

#define MAIN_DEVSTAT_BACKUP_BOOTMODE		GENMASK(12, 10)
#define MAIN_DEVSTAT_BACKUP_BOOTMODE_CFG	BIT(13)
#define MAIN_DEVSTAT_BACKUP_USB_MODE		BIT(0)

static void k3_get_backup_bootsource(u32 devstat, enum bootsource *src, int *instance)
{
	u32 bkup_bootmode = FIELD_GET(MAIN_DEVSTAT_BACKUP_BOOTMODE, devstat);
	u32 bkup_bootmode_cfg = FIELD_GET(MAIN_DEVSTAT_BACKUP_BOOTMODE_CFG, devstat);

	*src = BOOTSOURCE_UNKNOWN;

	switch (bkup_bootmode) {
	case BACKUP_BOOT_DEVICE_UART:
		*src = BOOTSOURCE_SERIAL;
		return;
	case BACKUP_BOOT_DEVICE_ETHERNET:
		*src = BOOTSOURCE_NET;
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
	case BACKUP_BOOT_DEVICE_I2C:
		*src = BOOTSOURCE_I2C;
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

static void k3_get_primary_bootsource(u32 devstat, enum bootsource *src, int *instance)
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
	case BOOT_DEVICE_ETHERNET_RGMII:
	case BOOT_DEVICE_ETHERNET_RMII:
		*src = BOOTSOURCE_NET;
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

#define AM625_BOOT_PARAM_TABLE_INDEX_OCRAM		IOMEM(0x43c3f290)

static void k3_get_bootsource(u32 devstat, enum bootsource *src, int *instance)
{
	u32 bootmode = readl(AM625_BOOT_PARAM_TABLE_INDEX_OCRAM);

	if (bootmode == K3_PRIMARY_BOOTMODE)
		k3_get_primary_bootsource(devstat, src, instance);
	else
		k3_get_backup_bootsource(devstat, src, instance);
}

#define AM625_WKUP_CTRL_MMR0_BASE		IOMEM(0x43000000)
#define AM625_CTRLMMR_MAIN_DEVSTAT		(AM625_WKUP_CTRL_MMR0_BASE + 0x30)

void am625_get_bootsource(enum bootsource *src, int *instance)
{
	u32 devstat;

	devstat = readl(AM625_CTRLMMR_MAIN_DEVSTAT);

	k3_get_bootsource(devstat, src, instance);
}

static void of_delete_node_path(struct device_node *root, const char *path)
{
	struct device_node *np;

	np = of_find_node_by_path_from(root, path);
	of_delete_node(np);
}

#define CTRLMMR_WKUP_JTAG_DEVICE_ID	(AM625_WKUP_CTRL_MMR0_BASE + 0x18)

#define JTAG_DEV_CORE_NR		GENMASK(21, 19)
#define JTAG_DEV_GPU			BIT(18)
#define JTAG_DEV_FEATURES		GENMASK(17, 13)
#define JTAG_DEV_FEATURE_NO_PRU		0x4

static int am625_of_fixup(struct device_node *root, void *unused)
{
	u32 full_devid = readl(CTRLMMR_WKUP_JTAG_DEVICE_ID);
	u32 feature_mask = FIELD_GET(JTAG_DEV_FEATURES, full_devid);
	int num_cores = FIELD_GET(JTAG_DEV_CORE_NR, full_devid);
	bool has_gpu = full_devid & JTAG_DEV_GPU;
	bool has_pru = !(feature_mask & JTAG_DEV_FEATURE_NO_PRU);
	char path[32];
	int i;

        for (i = num_cores; i < 4; i++) {
		snprintf(path, sizeof(path), "/cpus/cpu@%d", i);
		of_delete_node_path(root, path);

		snprintf(path, sizeof(path), "/cpus/cpu-map/cluster0/core%d", i);
		of_delete_node_path(root, path);

		snprintf(path, sizeof(path), "/bus@f0000/watchdog@e0%d0000", i);
		of_delete_node_path(root, path);
	}

        if (!has_gpu) {
		of_delete_node_path(root, "/bus@f0000/gpu@fd00000");
		of_delete_node_path(root, "/bus@f0000/watchdog@e0f0000");
	}

	if (!has_pru)
		of_delete_node_path(root, "/bus@f0000/pruss@30040000");

	return 0;
}

#define CTRLMMR_MCU_RST_CTRL	IOMEM(0x04518170)
#define RST_CTRL_ESM_ERROR_RST_EN_Z_MASK BIT(17)

static void am625_enable_mcu_esm_reset(void)
{
	/* activate reset of main by ESMO */
	u32 stat = readl(CTRLMMR_MCU_RST_CTRL);
	stat &= ~RST_CTRL_ESM_ERROR_RST_EN_Z_MASK;
	writel(stat, CTRLMMR_MCU_RST_CTRL);
}

static int am625_init(void)
{
	enum bootsource src = BOOTSOURCE_UNKNOWN;
	int instance = 0;

	if (!of_machine_is_compatible("ti,am625"))
		return 0;

	am625_get_bootsource(&src, &instance);
	bootsource_set(src, instance);
	am625_register_dram();

	genpd_activate();

	of_register_fixup(am625_of_fixup, NULL);

	am625_enable_mcu_esm_reset();

	return 0;
}
postcore_initcall(am625_init);

static int omap_env_init(void)
{
	char *partname, *cdevname, *envpath;
	struct cdev *cdev;
	const char *rootpath;
	int instance;

	if (!of_machine_is_compatible("ti,am625"))
		return 0;

	if (bootsource_get() != BOOTSOURCE_MMC)
		return 0;

	instance = bootsource_get_instance();

	cdevname = xasprintf("mmc%d", instance);
	partname = xasprintf("mmc%d.0", instance);

	device_detect_by_name(cdevname);

	cdev = cdev_open_by_name(partname, O_RDONLY);
	if (!cdev) {
		pr_err("Failed to get device %s\n", partname);
		goto out;
	}

	rootpath = cdev_mount_default(cdev, NULL);

	cdev_close(cdev);

	if (IS_ERR(rootpath)) {
		pr_err("Failed to load environment: mount %s failed (%ld)\n",
						cdev->name, PTR_ERR(rootpath));
		goto out;
	}

	symlink(rootpath, "/boot");

	if (IS_ENABLED(CONFIG_ENV_HANDLING)) {
		envpath = xasprintf("%s/barebox.env", rootpath);

		pr_debug("Loading default env from %s on device %s\n",
			 envpath, partname);

		default_environment_path_set(envpath);

		free(envpath);
	}

out:
	free(partname);
	free(cdevname);

	return 0;
}
late_initcall(omap_env_init);
