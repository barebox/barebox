/*
 * (c) 2012 Juergen Beisert <kernel@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Note: this driver works for the i.MX28 SoC. It might work for the
 * i.MX23 Soc as well, but is not tested yet.
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <errno.h>
#include <malloc.h>
#include <watchdog.h>
#include <reset_source.h>
#include <linux/err.h>

#define MXS_RTC_CTRL 0x0
#define MXS_RTC_SET_ADDR 0x4
#define MXS_RTC_CLR_ADDR 0x8
# define MXS_RTC_CTRL_WATCHDOGEN (1 << 4)

#define MXS_RTC_STAT 0x10
# define MXS_RTC_STAT_WD_PRESENT (1 << 29)

#define MXS_RTC_WATCHDOG 0x50

/* HW_RTC_PERSISTENT0 - holds bits used to configure various hardware settings */
#define MXS_RTC_PERSISTENT0 0x60

/* FIXME */
# define MXS_RTC_PERSISTENT0_SPARE_ANALOG (1 << 22)
/* dubious meaning from inside the SoC's firmware ROM */
# define MXS_RTC_PERSISTENT0_EXT_RST (1 << 21)
/* dubious meaning from inside the SoC's firmware ROM */
# define MXS_RTC_PERSISTENT0_THM_RST (1 << 20)
/* reserved on i.MX28 */
# define MXS_RTC_PERSISTENT0_RELEASE_GND (1 << 19)
# define MXS_RTC_PERSISTENT0_ENABLE_LRADC_PWRUP (1 << 18)
# define MXS_RTC_PERSISTENT0_AUTO_RESTART (1 << 17)
# define MXS_RTC_PERSISTENT0_DISABLE_PSWITCH (1 << 16)
# define MXS_RTC_PERSISTENT0_LOWERBIAS (1 << 14)
# define MXS_RTC_PERSISTENT0_DISABLE_XTALOK (1 << 13)
# define MXS_RTC_PERSISTENT0_MSEC_RES (1 << 8)
# define MXS_RTC_PERSISTENT0_ALARM_WAKE (1 << 7)
# define MXS_RTC_PERSISTENT0_XTAL32_FREQ (1 << 6)
# define MXS_RTC_PERSISTENT0_XTAL32KHZ_PWRUP (1 << 5)
# define MXS_RTC_PERSISTENT0_XTAL24MHZ_PWRUP (1 << 4)
# define MXS_RTC_PERSISTENT0_LCK_SECS (1 << 3)
# define MXS_RTC_PERSISTENT0_ALARM_EN (1 << 2)
# define MXS_RTC_PERSISTENT0_ALARM_WAKE_EN (1 << 1)
# define MXS_RTC_PERSISTENT0_CLOCKSOURCE (1 << 0)

/* HW_RTC_PERSISTENT1 - holds bits related to the ROM and redundant boot handling */
#define MXS_RTC_PERSISTENT1 0x70


/*
 * some of the following bits are for error reporting from ROM to the chained
 * firmware. It seems, if the error reporting bits are not cleared when the
 * chained firmware is running, the next time the following rule is active:
 *  "Loader enters recovery mode if any non-USB boot mode has an error.
 * Which results into a system that seems not to start anymore.
 */

/* dubious meaning from inside the SoC's firmware ROM */
# define MXS_RTC_PERSISTENT1_FORCE_UPDATER (1 << 31)

/* names are from the i.MX28 datasheet. Undocumented behaviour */
# define MXS_RTC_PERSISTENT1_ENUMERATE_500MA_TWICE (1 << 12)
# define MXS_RTC_PERSISTENT1_USB_BOOT_PLAYER_MODE (1 << 11)
# define MXS_RTC_PERSISTENT1_SKIP_CHECKDISK (1 << 10)
# define MXS_RTC_PERSISTENT1_USB_LOW_POWER_MODE (1 << 9)
# define MXS_RTC_PERSISTENT1_OTG_HNP_BIT (1 << 8)
# define MXS_RTC_PERSISTENT1_OTG_ATL_ROLE_BIT (1 << 7)
/*
 * a few undocumented bits
 */
# define MXS_RTC_PERSISTENT1_SD_INIT_SEQ_2_ENABLE (1 << 6)
# define MXS_RTC_PERSISTENT1_SD_CMD0_DISABLE (1 << 5)
# define MXS_RTC_PERSISTENT1_SD_INIT_SEQ_1_DISABLE (1 << 4)
/*
 * If this bit is set, ROM puts the SD/MMC card in high-speed mode.
 * If this bit is set, the ROM driver will use a maximum speed based on the
 * results of device identification and limited by choices available in the
 * SSP clock index.
 */
# define MXS_RTC_PERSISTENT1_SD_SPEED_ENABLE (1 << 3)
/*
 * The NAND driver sets this bit to indicate to the SDK that the boot image
 * has ECC errors that reached the warning threshold. The SDK regenerates the
 * firmware by copying it from the backup image. The SDK clears this bit.
 * This bit had change its meaning from i.XM23 to i.MX28. Refer section
 * 35.8 "NAND Boot Mode" for further details in the i.MX23 RM.
 */
# define MXS_RTC_PERSISTENT1_NAND_SDK_BLOCK_REWRITE (1 << 2)
/*
 * When this bit is set, ROM attempts to boot from the secondary image if the
 * boot driver supports it. This bit is set by the ROM boot driver and cleared
 * by the SDK after repair.
 * If not reset, the ROM seems to continue to start from the secondary image
 * which will fail forever if there is no secondary image
 */
# define MXS_RTC_PERSISTENT1_NAND_SECONDARY_BOOT (1 << 1)
/*
 * When this bit is set, the ROM code forces the system to boot in recovery
 * mode, regardless of the selected mode. The ROM clears the bit.
 */
# define MXS_RTC_PERSISTENT1_FORCE_RECOVERY (1 << 0)

#define MXS_RTC_DEBUG 0xc0

#define WDOG_TICK_RATE 1000 /* the watchdog uses a 1 kHz clock rate */

struct imx28_wd {
	struct watchdog wd;
	void __iomem *regs;
};

#define to_imx28_wd(h) container_of(h, struct imx28_wd, wd)

static int imx28_watchdog_set_timeout(struct watchdog *wd, unsigned timeout)
{
	struct imx28_wd *pwd = (struct imx28_wd *)to_imx28_wd(wd);
	void __iomem *base;

	if (timeout) {
		writel(timeout * WDOG_TICK_RATE, pwd->regs + MXS_RTC_WATCHDOG);
		base = pwd->regs + MXS_RTC_SET_ADDR;
	} else {
		base = pwd->regs + MXS_RTC_CLR_ADDR;
	}
	writel(MXS_RTC_CTRL_WATCHDOGEN, base + MXS_RTC_CTRL);
	writel(MXS_RTC_PERSISTENT1_FORCE_UPDATER, base + MXS_RTC_PERSISTENT1);

	return 0;
}

static void __maybe_unused imx28_detect_reset_source(const struct imx28_wd *p)
{
	u32 reg;

	reg = readl(p->regs + MXS_RTC_PERSISTENT0);
	if (reg & MXS_RTC_PERSISTENT0_EXT_RST) {
		writel(MXS_RTC_PERSISTENT0_EXT_RST,
			p->regs + MXS_RTC_PERSISTENT0 + MXS_RTC_CLR_ADDR);
		/*
		 * if the RTC has woken up the SoC, additionally the ALARM_WAKE
		 * bit is set. This bit should have precedence, because it
		 * reports the real event, why we are here.
		 */
		if (reg & MXS_RTC_PERSISTENT0_ALARM_WAKE) {
			writel(MXS_RTC_PERSISTENT0_ALARM_WAKE,
				p->regs + MXS_RTC_PERSISTENT0 + MXS_RTC_CLR_ADDR);
			reset_source_set(RESET_WKE);
			return;
		}
		reset_source_set(RESET_POR);
		return;
	}
	if (reg & MXS_RTC_PERSISTENT0_THM_RST) {
		writel(MXS_RTC_PERSISTENT0_THM_RST,
			p->regs + MXS_RTC_PERSISTENT0 + MXS_RTC_CLR_ADDR);
		reset_source_set(RESET_RST);
		return;
	}
	reg = readl(p->regs + MXS_RTC_PERSISTENT1);
	if (reg & MXS_RTC_PERSISTENT1_FORCE_UPDATER) {
		writel(MXS_RTC_PERSISTENT1_FORCE_UPDATER,
			p->regs + MXS_RTC_PERSISTENT1 + MXS_RTC_CLR_ADDR);
		reset_source_set(RESET_WDG);
		return;
	}

	reset_source_set(RESET_RST);
}

static int imx28_wd_probe(struct device_d *dev)
{
	struct resource *iores;
	struct imx28_wd *priv;
	int rc;

	priv = xzalloc(sizeof(struct imx28_wd));
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	priv->regs = IOMEM(iores->start);
	priv->wd.set_timeout = imx28_watchdog_set_timeout;
	priv->wd.timeout_max = ULONG_MAX / WDOG_TICK_RATE;
	priv->wd.hwdev = dev;

	if (!(readl(priv->regs + MXS_RTC_STAT) & MXS_RTC_STAT_WD_PRESENT)) {
		rc = -ENODEV;
		goto on_error;
	}

	/* disable the debug feature to ensure a working WD */
	writel(0x00000000, priv->regs + MXS_RTC_DEBUG);

	rc = watchdog_register(&priv->wd);
	if (rc != 0)
		goto on_error;

	if (IS_ENABLED(CONFIG_RESET_SOURCE))
		imx28_detect_reset_source(priv);

	dev->priv = priv;
	return 0;

on_error:
	free(priv);
	return rc;
}

static void imx28_wd_remove(struct device_d *dev)
{
	struct imx28_wd *priv= dev->priv;
	watchdog_deregister(&priv->wd);
	free(priv);
}

static struct driver_d imx28_wd_driver = {
	.name   = "im28wd",
	.probe  = imx28_wd_probe,
	.remove = imx28_wd_remove,
};
device_platform_driver(imx28_wd_driver);
