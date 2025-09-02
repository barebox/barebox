// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 Ivaylo Ivanov <ivo.ivanov.ivanov1@gmail.com>
 */
#include <common.h>
#include <pbl.h>
#include <linux/sizes.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <asm/sections.h>
#include <asm/cache.h>
#include <asm/mmu.h>

extern char __dtb_exynos8895_dreamlte_start[];
extern char __dtb_exynos990_x1s_start[];

/* called from assembly */
void lowlevel_exynos(void *downstream_fdt);

static bool is_model(const void *fdt, const char *prefix)
{
	int node, len;
	const char *model;

	node = fdt_path_offset(fdt, "/");
	if (node < 0)
		return false;

	/*
	 * Samsung doesn't keep compatibles as consistent as model
	 * strings. For example:
	 *
	 * compatible = "samsung,X1S EUR OPEN 05", "samsung,EXYNOS990";
	 * compatible = "samsung, DREAMLTE KOR rev01", "samsung,EXYNOS8895";
	 * compatible = "samsung, J7VE LTE LTN OPEN 00", "samsung,exynos7870";
	 *
	 * Use models for matching instead to avoid confusion.
	 */
	model = fdt_getprop(fdt, node, "model", &len);
	if (!model)
		return false;

	while (*prefix) {
		if (*model++ != *prefix++)
			return false;
	}
	return true;
}

static noinline void exynos_continue(void *downstream_fdt)
{
	void *fdt;
	unsigned long membase, memsize;
	char *__dtb_start;

	/* select device tree dynamically */
	if (is_model(downstream_fdt, "Samsung DREAMLTE")) {
		__dtb_start = __dtb_exynos8895_dreamlte_start;
	} else if (is_model(downstream_fdt, "Samsung X1S")) {
		__dtb_start = __dtb_exynos990_x1s_start;
	} else {
		/* we didn't match any device */
		return;
	}
	fdt = __dtb_start + get_runtime_offset();
	fdt_find_mem(fdt, &membase, &memsize);

	barebox_arm_entry(membase, memsize, fdt);
}

void lowlevel_exynos(void *downstream_fdt)
{
	arm_cpu_lowlevel_init();

	relocate_to_current_adr();

	setup_c();

	if (!downstream_fdt || fdt_check_header(downstream_fdt))
		return;

	exynos_continue(downstream_fdt);
}
