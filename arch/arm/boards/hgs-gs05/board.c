// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2025 Pengutronix
// SPDX-FileCopyrightText: Leica Geosystems AG

#include <boards/hgs/common.h>
#include <bootsource.h>
#include <common.h>
#include <deep-probe.h>
#include <envfs.h>
#include <environment.h>
#include <init.h>
#include <i2c/i2c.h>
#include <linux/phy.h>
#include <mach/imx/bbu.h>
#include <mach/imx/generic.h>
#include <mfd/hgs-efi.h>
#include <of.h>
#include <state.h>

#define PHY_ID_AR8031	0x004dd074
#define AR_PHY_ID_MASK	0xffffffff

#define HGS_GS05_BASE_NAME	"Hexagon Geosystems GS05"

#define HGS_GS05_MACHINE(_revid, _compatible, _model_suffix) \
	HGS_MACHINE(_revid, _compatible, HGS_GS05_BASE_NAME " " _model_suffix)

static struct hgs_machine hgs_gs05_variants[] = {
	HGS_GS05_MACHINE(HGS_BOARD_REV_C, "hgs,gs05-rev-c", "Rev-C"),
	HGS_GS05_MACHINE(HGS_BOARD_REV_D, "hgs,gs05-rev-d", "Rev-D"),
};

#define HGS_GS05_LEGACY_MACHINE(_revchar, _revid, _compatible, _model_suffix) \
{									\
	.revision = _revchar,						\
	.machine = HGS_GS05_MACHINE(_revid, _compatible, _model_suffix) \
}

static struct hgs_gs05_legacy_machine {
	u8 revision;
	struct hgs_machine machine;
} hgs_gs05_legacy_variants[] = {
	HGS_GS05_LEGACY_MACHINE('C', HGS_BOARD_REV_C, "hgs,gs05-rev-c", "Rev-C"),
	HGS_GS05_LEGACY_MACHINE('D', HGS_BOARD_REV_D, "hgs,gs05-rev-d", "Rev-D"),
};

static int ar8031_phy_fixup(struct phy_device *phydev)
{
	/* enable rgmii rxc skew and phy mode select to RGMII copper */
	phy_write(phydev, 0x1d, 0x1f);
	phy_write(phydev, 0x1e, 0x8);
	phy_write(phydev, 0x1d, 0x00);
	phy_write(phydev, 0x1e, 0x82ee);
	phy_write(phydev, 0x1d, 0x05);
	phy_write(phydev, 0x1e, 0x100);

	return 0;
}

static struct hgs_machine *
hgs_gs05_get_board_from_legacy(const unsigned char *serial)
{
	struct hgs_gs05_legacy_machine *machine;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(hgs_gs05_legacy_variants); i++) {
		machine = &hgs_gs05_legacy_variants[i];
		if (serial[6] == machine->revision)
			return &machine->machine;
	}

	return ERR_PTR(-EINVAL);
}

static struct hgs_machine *
hgs_gs05_select_board(const unsigned char *serial, bool legacy_format)
{
	const struct hgs_board_revision *rev;
	struct hgs_machine *machine;
	unsigned int i;

	/* TODO: Remove legacy handling if no longer required */
	if (legacy_format)
		return hgs_gs05_get_board_from_legacy(serial);

	rev = hgs_get_rev_from_part_trace((struct hgs_part_trace_code *)serial);
	if (!rev)
		return ERR_PTR(-EINVAL);

	for (i = 0; i < ARRAY_SIZE(hgs_gs05_variants); i++) {
		machine = &hgs_gs05_variants[i];
		if (rev->id == machine->revision)
			return machine;
	}

	return ERR_PTR(-EINVAL);
}

static u64
hgs_gs05_set_efi_poll_intervall(struct device *efid, u64 new_polling_interval)
{
	const char *old_interval_str;
	char *new_interval;
	u64 old_interval;

	old_interval_str = dev_get_param(efid->parent, "polling_interval");
	kstrtoull(old_interval_str, 10, &old_interval);

	pr_debug("Update EFI UART-Rx poll interval: %llu ns -> %llu ns\n",
		 old_interval, new_polling_interval);

	new_interval = basprintf("%llu", new_polling_interval);
	dev_set_param(efid->parent, "polling_interval", new_interval);
	free(new_interval);

	return old_interval;
}

/* '"' + sizeof(struct hgs_part_trace_code) + '"' + string delim '\0' */
#define HGS_GS05_SERIAL_NUMBER_CHARS	\
	(1 + sizeof(struct hgs_part_trace_code) + 1 + 1)

static struct hgs_machine *hgs_gs05_get_board(struct device *dev)
{
	u8 buf[HGS_GS05_SERIAL_NUMBER_CHARS] = { };
	struct device *efi_dev = get_device_by_name("efi");
	struct hgs_efi *efi = dev_get_priv(efi_dev->parent);
	struct hgs_sep_cmd cmd = {
		.type = HGS_SEP_MSG_TYPE_COMMAND,
		.msg_id = 11,
		.reply_data = buf,
		.reply_data_size = sizeof(buf),
	};
	u64 orig_poll_interval;
	unsigned char *resp;
	bool legacy = false;
	unsigned int len;
	int ret;

	/*
	 * The GS05 has a very slow EFI UART baudrate of 19200 bps. So the
	 * serial-number query takes ~18ms just for the transfer, not taking
	 * the EFI MCU command processing into account.
	 *
	 * This causes an UART Rx FIFO overflow with the standard EFI MCU
	 * driver settings for polling_window and polling_intervall because the
	 * EFI response is too long for the 32-Rx byte FIFO and the poll
	 * reschedule is too late.
	 *
	 * Set the polling_interval to 1ms to workaround the slow UART baudrate,
	 * so the complete poll intervall takes ~11ms. This shall ensure that
	 * the reschdule happens at least once while retrieving the EFI
	 * response.
	 *
	 * Adapt the polling_interval and not the polling_window to be more
	 * responsive and avoid unnecessary wait times.
	 *
	 * The polling_interval unit is ns.
	 */
	orig_poll_interval = hgs_gs05_set_efi_poll_intervall(efi_dev, 1 * MSECOND);
	ret = hgs_efi_exec(efi, &cmd);
	hgs_gs05_set_efi_poll_intervall(efi_dev, orig_poll_interval);
	if (ret) {
		dev_warn(dev, "Failed to query serial number\n");
		return ERR_PTR(-EINVAL);
	}

	resp = hgs_efi_extract_str_response(buf);
	if (IS_ERR(resp)) {
		dev_warn(dev, "Failed to query EFI response");
		return ERR_CAST(resp);
	}
	len = strlen(resp);

	switch (len) {
	case 0:
		dev_warn(dev, "Empty EFI serial number detected\n");
		return ERR_PTR(-EINVAL);
	case 17:
		dev_warn(dev, "Legacy Revision format detected: please update PP4 and re-program board-id in the new format.\n");
		legacy = true;
		fallthrough;
	case 29:
		barebox_set_serial_number(resp);
		return hgs_gs05_select_board(resp, legacy);
	default:
		dev_warn(dev, "Invalid serial number EFI response length\n");
		return ERR_PTR(-EINVAL);
	}
}

static struct hgs_machine *hgs_gs05_fallback_board(struct device *dev)
{
	struct hgs_machine *fallback;

	fallback = &hgs_gs05_variants[ARRAY_SIZE(hgs_gs05_variants) - 1];
	dev_warn(dev, "Board detection failed, fallback to: %s\n",
		 fallback->model);

	return fallback;
}

static int hgs_gs05_probe(struct device *dev)
{
	struct hgs_machine *board;

	phy_register_fixup_for_uid(PHY_ID_AR8031, AR_PHY_ID_MASK,
				   ar8031_phy_fixup);
	/*
	 * Ensure that the MCU driver was already probed before try to access
	 * the MCU to query the revision.
	 */
	of_devices_ensure_probed_by_compatible("hgs,efi-gs05");

	/* Ensure state is probed to be able to query the hostname */
	state_by_alias("state");

	board = hgs_gs05_get_board(dev);
	if (IS_ERR(board))
		board = hgs_gs05_fallback_board(dev);
	board->dev = dev;
	board->type = HGS_HW_GS05;
	board->console_alias = "serial2";
	board->hostname = xstrdup(getenv("state.product.hostname"));

	return hgs_common_boot(board);
}

static const struct of_device_id hgs_gs05_of_match[] = {
	{ .compatible = "hgs,gs05" },
	{ /* Sentinel */ }
};
BAREBOX_DEEP_PROBE_ENABLE(hgs_gs05_of_match);

static struct driver hgs_gs05_board_driver = {
	.name = "board-hgs-gs05",
	.probe = hgs_gs05_probe,
	.of_compatible = hgs_gs05_of_match,
};
coredevice_platform_driver(hgs_gs05_board_driver);
