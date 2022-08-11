// SPDX-License-Identifier: GPL-2.0-or-later

#include <of.h>
#include <deep-probe.h>

static const struct of_device_id meerkat96_match[] = {
	{ .compatible = "novtech,imx7d-meerkat96" },
	{ /* Sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(meerkat96_match);
