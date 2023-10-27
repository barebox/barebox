// SPDX-License-Identifier: GPL-2.0-or-later

#include <deep-probe.h>
#include <of.h>

static const struct of_device_id calao_of_match[] = {
	{ .compatible = "calao,tny-a9260" },
	{ .compatible = "calao,tny-a9g20" },
	{ .compatible = "calao,usb-a9260" },
	{ .compatible = "calao,usb-a9g20" },
	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(calao_of_match);
