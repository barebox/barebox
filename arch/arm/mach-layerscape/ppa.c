// SPDX-License-Identifier: GPL-2.0

#define pr_fmt(fmt) "ppa: " fmt

#include <common.h>
#include <init.h>
#include <mmu.h>
#include <firmware.h>
#include <memory.h>
#include <linux/sizes.h>
#include <linux/arm-smccc.h>
#include <asm/mmu.h>
#include <soc/fsl/immap_lsch2.h>
#include <asm/system.h>
#include <image-fit.h>
#include <asm/psci.h>
#include <mach/layerscape/layerscape.h>
#include <asm/cache.h>

int ppa_entry(const void *, u32 *, u32 *);

#define SEC_JR3_OFFSET                     0x40000

static int of_psci_do_fixup(struct device_node *root, void *unused)
{
	unsigned long psci_version;
	struct device_node *np;
	struct arm_smccc_res res = {};

	arm_smccc_smc(ARM_PSCI_0_2_FN_PSCI_VERSION, 0, 0, 0, 0, 0, 0, 0, &res);
	psci_version = res.a0;

	for_each_compatible_node_from(np, root, NULL, "fsl,sec-v4.0-job-ring") {
		const void *reg;
		int na = of_n_addr_cells(np);
		u64 offset;

		reg = of_get_property(np, "reg", NULL);
		if (!reg)
			continue;

		offset = of_read_number(reg, na);
		if (offset != SEC_JR3_OFFSET)
			continue;

		of_delete_node(np);
		break;
	}

	return of_psci_fixup(root, psci_version, "smc");
}

static int ppa_init(void *ppa, size_t ppa_size, void *sec_firmware_addr)
{
	int ret;
	u32 *boot_loc_ptr_l, *boot_loc_ptr_h;
	struct ccsr_scfg __iomem *scfg = (void *)(LSCH2_SCFG_ADDR);
	struct fit_handle *fit;
	void *conf;
	const void *buf;
	unsigned long firmware_size;

	boot_loc_ptr_l = &scfg->scratchrw[1];
	boot_loc_ptr_h = &scfg->scratchrw[0];

	fit = fit_open_buf(ppa, ppa_size, false, BOOTM_VERIFY_AVAILABLE);
	if (IS_ERR(fit)) {
		pr_err("Cannot open ppa FIT image: %pe\n", fit);
		return PTR_ERR(fit);
	}

	conf = fit_open_configuration(fit, NULL);
	if (IS_ERR(conf)) {
		pr_err("Cannot open default config in ppa FIT image: %pe\n", conf);
		ret = PTR_ERR(conf);
		goto err;
	}


	ret = fit_open_image(fit, conf, "firmware", &buf, &firmware_size);
	if (ret) {
		pr_err("Cannot open firmware image in ppa FIT image: %s\n",
		       strerror(-ret));
		goto err;
	}

	/* Copy the secure firmware to secure memory */
	memcpy(sec_firmware_addr, buf, firmware_size);
	sync_caches_for_execution();

	ret = ppa_entry(sec_firmware_addr, boot_loc_ptr_l, boot_loc_ptr_h);
	if (ret) {
		pr_err("Couldn't install PPA firmware: %s\n", strerror(-ret));
		goto err;
	}

	pr_info("PPA firmware installed at 0x%p, now in EL%d\n",
		sec_firmware_addr, current_el());

	of_register_fixup(of_psci_do_fixup, NULL);
err:
	fit_close(fit);

	return ret;
}

int ls1046a_ppa_init(resource_size_t ppa_start, resource_size_t ppa_size)
{
	resource_size_t ppa_end = ppa_start + ppa_size - 1;
	struct resource *res;
	void *ppa_fw;
	size_t ppa_fw_size;
	int el = current_el();
	int ret;

	res = reserve_sdram_region("ppa", ppa_start, ppa_size);
	if (!res) {
		pr_err("Cannot request SDRAM region %pa - %pa\n",
		       &ppa_start, &ppa_end);
		return -EINVAL;
	}

	get_builtin_firmware(ppa_ls1046a_bin, &ppa_fw, &ppa_fw_size);

	if (el == 3) {
		unsigned long cr;

		asm volatile("mrs %0, sctlr_el3" : "=r" (cr) : : "cc");
		remap_range((void *)ppa_start, ppa_size, MAP_CACHED);

		ret = ppa_init(ppa_fw, ppa_fw_size, (void *)ppa_start);

		asm volatile("msr sctlr_el2, %0" : : "r" (cr) : "cc");
		remap_range((void *)ppa_start, ppa_size, MAP_UNCACHED);

		if (ret)
			return ret;
	}

	of_register_fixup(of_fixup_reserved_memory, res);

	return 0;
}
