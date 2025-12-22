// SPDX-License-Identifier: GPL-2.0-only
/*
 * EFI Boot Guard, iTCO support (Version 3 and later)
 *
 * Copyright (c) 2006-2011 Wim Van Sebroeck <wim@iguana.be>.
 * Copyright (c) 2019 Siemens AG
 * Copyright (c) 2019 Ahmad Fatoum, Pengutronix
 *
 * Authors:
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *  Christian Storm <christian.storm@siemens.com>
 */

#include <common.h>
#include <init.h>
#include <linux/pci.h>
#include <watchdog.h>

#define ACPIBASE		0x40
#define ACPICTRL_PMCBASE	0x44

#define PMBASE_ADDRMASK		0x0000ff80
#define PMCBASE_ADDRMASK	0xfffffe00

#define ACPIBASE_GCS_OFF	0x3410

#define ACPIBASE_SMI_OFF	0x30
#define ACPIBASE_SMI_END	0x33
#define ACPIBASE_PMC_OFF	0x08
#define ACPIBASE_PMC_END	0x0c
#define ACPIBASE_TCO_OFF	0x60
#define ACPIBASE_TCO_END	0x7f

#define SMI_TCO_MASK		(1 << 13)

#define TCO_TMR_HLT_MASK	(1 << 11)

/* SMI Control and Enable Register */
#define SMI_EN(itco)	((itco)->smibase)
/* TCO base address */
#define TCOBASE(itco)	((itco)->tcobase)

#define TCO_RLD(p)	(TCOBASE(p) + 0x00) /* TCO Timer Reload/Curr. Value */
#define TCOv1_TMR(p)	(TCOBASE(p) + 0x01) /* TCOv1 Timer Initial Value*/
#define TCO_DAT_IN(p)	(TCOBASE(p) + 0x02) /* TCO Data In Register	*/
#define TCO_DAT_OUT(p)	(TCOBASE(p) + 0x03) /* TCO Data Out Register	*/
#define TCO1_STS(p)	(TCOBASE(p) + 0x04) /* TCO1 Status Register	*/
#define TCO2_STS(p)	(TCOBASE(p) + 0x06) /* TCO2 Status Register	*/
#define TCO1_CNT(p)	(TCOBASE(p) + 0x08) /* TCO1 Control Register	*/
#define TCO2_CNT(p)	(TCOBASE(p) + 0x0a) /* TCO2 Control Register	*/
#define TCOv2_TMR(p)	(TCOBASE(p) + 0x12) /* TCOv2 Timer Initial Value*/

#define PMC_NO_REBOOT_MASK	(1 << 4)

#define RCBABASE		0xf0

#define PCI_ID_ITCO_INTEL_ICH9		0x2918

struct itco_priv;

struct itco_info {
	u32 pci_id;
	const char *name;
	u32 pmbase;
	u32 smireg;
	int (*update_no_reboot_bit)(struct itco_priv *itco, bool set);
	unsigned version;
};

struct itco_priv {
	struct pci_dev *pdev;
	struct watchdog wdd;
	void __iomem *io;
	u32 smibase;
	u32 tcobase;
	void __iomem *gcs_pmc;
	struct itco_info *info;
	unsigned timeout;
};

static u32 itco_get_pmbase(struct itco_priv *itco)
{
	u32 pmbase = itco->info->pmbase;

	if (!pmbase)
		pci_read_config_dword(itco->pdev, ACPIBASE, &pmbase);

	return pmbase & PMBASE_ADDRMASK;
}

static inline struct itco_priv *to_itco_priv(struct watchdog *wdd)
{
	return container_of(wdd, struct itco_priv, wdd);
}

static void itco_wdt_ping(struct itco_priv *itco)
{
	/* Reload the timer by writing to the TCO Timer Counter register */
	outw(0x0001, TCO_RLD(itco));
}

static inline unsigned int seconds_to_ticks(struct itco_priv *itco, int secs)
{
	return itco->info->version == 3 ? secs : (secs * 10) / 6;
}

static inline unsigned int ticks_to_seconds(struct itco_priv *itco, int ticks)
{
	return itco->info->version == 3 ? ticks : (ticks * 6) / 10;
}


static int itco_wdt_start(struct itco_priv *itco, unsigned int timeout)
{
	unsigned tmrval;
	u32 value;
	int ret;

	tmrval = seconds_to_ticks(itco, timeout);

	/* Enable TCO SMIs */
	value = inl(SMI_EN(itco)) | SMI_TCO_MASK;
	outl(value, SMI_EN(itco));

	/* Set timer value */
	value = inw(TCOv2_TMR(itco));

	value &= 0xfc00;
	value |= tmrval & 0x3ff;

	outw(value, TCOv2_TMR(itco));
	value = inw(TCOv2_TMR(itco));

	if ((value & 0x3ff) != tmrval)
		return -EINVAL;

	/* Force reloading of timer value */
	outw(1, TCO_RLD(itco));

	/* Clear NO_REBOOT flag */
	ret = itco->info->update_no_reboot_bit(itco, false);
	if (ret)
		return ret;

	/* Clear HLT flag to start timer */
	value = inw(TCO1_CNT(itco)) & ~TCO_TMR_HLT_MASK;
	outw(value, TCO1_CNT(itco));
	value = inw(TCO1_CNT(itco));

	if (value & 0x0800)
		return -EIO;

	return 0;
}

static int itco_wdt_stop(struct itco_priv *itco)
{
	u32 val;

	/* Bit 11: TCO Timer Halt -> 1 = The TCO timer is disabled */
	val = inw(TCO1_CNT(itco)) | 0x0800;
	outw(val, TCO1_CNT(itco));
	val = inb(TCO1_CNT(itco));

	/* Set the NO_REBOOT bit to prevent later reboots, just for sure */
	itco->info->update_no_reboot_bit(itco, true);

	if ((val & 0x0800) == 0)
		return -EIO;
	return 0;
}

static int itco_wdt_set_timeout(struct watchdog *wdd, unsigned int timeout)
{
	struct itco_priv *itco = to_itco_priv(wdd);
	int ret;

	if (!timeout)
		return itco_wdt_stop(itco);

	/* from the specs: */
	/* "Values of 0h-3h are ignored and should not be attempted" */
	if (timeout < 0x04)
		return -EINVAL;

	if (itco->timeout != timeout) {
		ret = itco_wdt_start(itco, timeout);
		if (ret) {
			dev_err(wdd->hwdev, "Fail to (re)start watchdog\n");
			return ret;
		}
	}

	itco_wdt_ping(itco);
	return 0;
}

static inline u32 no_reboot_bit(unsigned version)
{
	u32 enable_bit;

	switch (version) {
	case 5:
	case 3:
		enable_bit = 0x00000010;
		break;
	case 2:
		enable_bit = 0x00000020;
		break;
	case 4:
	case 1:
	default:
		enable_bit = 0x00000002;
		break;
	}

	return enable_bit;
}


static int update_no_reboot_bit(struct itco_priv *itco, bool set)
{
	u32 val32 = 0, newval32 = 0;

	val32 = readl(itco->gcs_pmc);
	if (set)
		val32 |= no_reboot_bit(itco->info->version);
	else
		val32 &= ~no_reboot_bit(itco->info->version);
	writel(val32, itco->gcs_pmc);
	newval32 = readl(itco->gcs_pmc);

	/* make sure the update is successful */
	if (val32 != newval32)
		return -EPERM;

	return 0;
}

static void lpc_ich_enable_acpi_space(struct itco_priv *itco)
{
	u8 reg_save;

	pci_read_config_byte(itco->pdev, ACPICTRL_PMCBASE, &reg_save);
	pci_write_config_byte(itco->pdev, ACPICTRL_PMCBASE, reg_save | 0x80);
}

enum itco_chipsets {
	ITCO_INTEL_ICH9,
};

/* version 1 not supported! */
static struct itco_info itco_chipset_info[] = {
	[ITCO_INTEL_ICH9] = {
		.pci_id = PCI_ID_ITCO_INTEL_ICH9,
		.name = "ICH9", /* QEmu machine q35 */
		.smireg = 0x30,
		.update_no_reboot_bit = update_no_reboot_bit,
		.version = 2,
	},
};

static int itco_wdt_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct itco_priv *itco;
	struct watchdog *wdd;
	u32 rcba_base_cfg;
	u32 pmbase;
	int ret;
	int i;

	pci_enable_device(pdev);
	pci_set_master(pdev);

	itco = xzalloc(sizeof(*itco));

	itco->pdev = pdev;

	for (i = 0; i < ARRAY_SIZE(itco_chipset_info); i++) {
		if (id->device == itco_chipset_info[i].pci_id) {
			itco->info = &itco_chipset_info[i];
			break;
		}
	}

	if (!itco->info)
		return -ENODEV;


	pci_read_config_dword(itco->pdev, RCBABASE, &rcba_base_cfg);
	if (!(rcba_base_cfg & 1)) {
		dev_notice(&pdev->dev, "RCBA is disabled by hardware/BIOS, device disabled\n");
		return -ENODEV;
	}

	pmbase = itco_get_pmbase(itco);
	if (!pmbase) {
		dev_notice(&itco->pdev->dev, "I/O space for ACPI uninitialized\n");
		return -ENODEV;
	}

	itco->smibase = pmbase + ACPIBASE_SMI_OFF;
	itco->tcobase = pmbase + ACPIBASE_TCO_OFF;

	lpc_ich_enable_acpi_space(itco);

	itco->gcs_pmc = IOMEM(rcba_base_cfg & 0xffffc000UL) + ACPIBASE_GCS_OFF;


	dev_notice(&pdev->dev, "gcs_pmc = 0x%p, smibase = 0x%x, tcobase = 0x%x\n",
		   itco->gcs_pmc, itco->smibase, itco->tcobase);

	wdd = &itco->wdd;
	wdd->hwdev = &pdev->dev;
	wdd->set_timeout = itco_wdt_set_timeout;

	wdd->timeout_max = ticks_to_seconds(itco, 0x3ff);

	outw(0x0008, TCO1_STS(itco)); /* Clear the Time Out Status bit */
	outw(0x0002, TCO2_STS(itco)); /* Clear SECOND_TO_STS bit */
	outw(0x0004, TCO2_STS(itco)); /* Clear BOOT_STS bit */

	ret = watchdog_register(wdd);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register watchdog device\n");
		return ret;
	}

	dev_info(&pdev->dev, "probed Intel TCO %s watchdog\n", itco->info->name);

	return 0;
}


static DEFINE_PCI_DEVICE_TABLE(itco_wdt_pci_tbl) = {
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, PCI_ID_ITCO_INTEL_ICH9) },
	{ /* sentinel */ },
};

static struct pci_driver itco_wdt_driver = {
	.name = "itco_wdt",
	.id_table = itco_wdt_pci_tbl,
	.probe = itco_wdt_probe,
};
device_pci_driver(itco_wdt_driver);
