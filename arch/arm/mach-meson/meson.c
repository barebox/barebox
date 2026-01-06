// SPDX-License-Identifier: GPL-2.0-only

#include <deep-probe.h>
#include <init.h>
#include <of.h>
#include <partitions.h>
#include <mach/meson/g12.h>
#include <mach/meson/meson.h>

static const struct of_device_id meson_of_match[] = {
#ifdef CONFIG_ARCH_MESON_G12
	{ .compatible = "amlogic,g12a", .data = (void *)MESON_SOC_G12 },
	{ .compatible = "amlogic,g12b", .data = (void *)MESON_SOC_G12 },
	{ .compatible = "amlogic,sm1", .data = (void *)MESON_SOC_G12 },
#endif
	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(meson_of_match);

enum meson_soc meson_soc(void)
{
	static enum meson_soc soc;
	const struct of_device_id *match;

	if (soc)
		return soc;

	match = of_match_node(meson_of_match, of_get_root_node());
	if (match)
		soc = (uintptr_t)match->data;

	return soc;
}

static int meson_soc_init(void)
{
	if (meson_soc() == MESON_SOC_UNKNOWN)
		return 0;

	/* a boot image on SD overlaps the EFI PART magic, which leaves
	 * only the alternate GPT
	 */
	gpt_suppress_fix_messages();

	if (meson_is_g12())
		return meson_g12_init();

	return 0;
}
postcore_initcall(meson_soc_init);

static int meson_mem_init(void)
{
	if (meson_is_g12())
		return meson_g12_mem_init();

	return 0;
}
mem_initcall(meson_mem_init);
