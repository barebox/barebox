// SPDX-License-Identifier: GPL-2.0-only

#include <deep-probe.h>
#include <init.h>
#include <of.h>
#include <partitions.h>
#include <mach/meson/a9.h>
#include <mach/meson/g12.h>
#include <mach/meson/gx.h>
#include <mach/meson/meson.h>

static const struct of_device_id meson_of_match[] = {
#ifdef CONFIG_ARCH_MESON_GX
	{ .compatible = "amlogic,meson-gxbb", .data = (void *)MESON_SOC_GX },
	{ .compatible = "amlogic,meson-gxl", .data = (void *)MESON_SOC_GX },
	{ .compatible = "amlogic,meson-gxlx", .data = (void *)MESON_SOC_GX },
	{ .compatible = "amlogic,meson-gxm", .data = (void *)MESON_SOC_GX },
#endif
#ifdef CONFIG_ARCH_MESON_G12
	{ .compatible = "amlogic,g12a", .data = (void *)MESON_SOC_G12 },
	{ .compatible = "amlogic,g12b", .data = (void *)MESON_SOC_G12 },
	{ .compatible = "amlogic,sm1", .data = (void *)MESON_SOC_G12 },
#endif
#ifdef CONFIG_ARCH_MESON_A9
	{ .compatible = "amlogic,a9", .data = (void *)MESON_SOC_A9 },
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

	if (meson_is_gx())
		return meson_gx_init();
	if (meson_is_g12())
		return meson_g12_init();
	if (meson_is_a9())
		return meson_a9_init();

	return 0;
}
postcore_initcall(meson_soc_init);

static int meson_mem_init(void)
{
	if (meson_is_gx())
		return meson_gx_mem_init();
	if (meson_is_g12())
		return meson_g12_mem_init();
	if (meson_is_a9())
		return meson_a9_mem_init();

	return 0;
}
mem_initcall(meson_mem_init);

static int meson_postmem_init(void)
{
	if (meson_is_a9())
		return meson_a9_reserve_init();

	return 0;
}
postmem_initcall(meson_postmem_init);
