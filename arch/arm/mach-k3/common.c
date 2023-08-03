// SPDX-License-Identifier: GPL-2.0-only
#include <of.h>
#include <deep-probe.h>
#include <init.h>
#include <pm_domain.h>

static const struct of_device_id k3_of_match[] = {
	{
		.compatible = "ti,am625",
	},
	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(k3_of_match);

static int am625_init(void)
{
	if (!of_machine_is_compatible("ti,am625"))
		return 0;

	genpd_activate();

	return 0;
}
postcore_initcall(am625_init);
