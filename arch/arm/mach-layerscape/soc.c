// SPDX-License-Identifier: GPL-2.0-only
#include <soc/fsl/scfg.h>
#include <io.h>
#include <init.h>
#include <memory.h>
#include <linux/bug.h>
#include <linux/bitfield.h>
#include <linux/printk.h>
#include <mach/layerscape/layerscape.h>
#include <of.h>
#include <of_address.h>

int __layerscape_soc_type;

static enum scfg_endianess scfg_endianess = SCFG_ENDIANESS_INVALID;

static void scfg_check_endianess(void)
{
	BUG_ON(scfg_endianess == SCFG_ENDIANESS_INVALID);
}

void scfg_clrsetbits32(void __iomem *addr, u32 clear, u32 set)
{
	scfg_check_endianess();

	if (scfg_endianess == SCFG_ENDIANESS_LITTLE)
		clrsetbits_le32(addr, clear, set);
	else
		clrsetbits_be32(addr, clear, set);
}

void scfg_clrbits32(void __iomem *addr, u32 clear)
{
	scfg_check_endianess();

	if (scfg_endianess == SCFG_ENDIANESS_LITTLE)
		clrbits_le32(addr, clear);
	else
		clrbits_be32(addr, clear);
}

void scfg_setbits32(void __iomem *addr, u32 set)
{
	scfg_check_endianess();

	if (scfg_endianess == SCFG_ENDIANESS_LITTLE)
		setbits_le32(addr, set);
	else
		setbits_be32(addr, set);
}

void scfg_out16(void __iomem *addr, u16 val)
{
	scfg_check_endianess();

	if (scfg_endianess == SCFG_ENDIANESS_LITTLE)
		out_le16(addr, val);
	else
		out_be16(addr, val);
}

void scfg_init(enum scfg_endianess endianess)
{
	scfg_endianess = endianess;
}

static int layerscape_soc_from_dt(void)
{
	if (of_machine_is_compatible("fsl,ls1021a"))
		return LAYERSCAPE_SOC_LS1021A;
	if (of_machine_is_compatible("fsl,ls1028a"))
		return LAYERSCAPE_SOC_LS1028A;
	if (of_machine_is_compatible("fsl,ls1046a"))
		return LAYERSCAPE_SOC_LS1046A;

	return 0;
}

static int ls1021a_init(void)
{
	if (!cpu_is_ls1021a())
		return -EINVAL;

	ls1021a_bootsource_init();
	ls102xa_smmu_stream_id_init();
	layerscape_register_pbl_image_handler();
	ls1021a_restart_register_feature();

	return 0;
}

static int ls1028a_init(void)
{
	if (!cpu_is_ls1028a())
		return -EINVAL;

	ls1028a_bootsource_init();
	layerscape_register_pbl_image_handler();
	ls1028a_setup_icids();

	return 0;
}

static int ls1028a_reserve_tfa(void)
{
	resource_size_t tfa_start = LS1028A_TFA_RESERVED_START;
	resource_size_t tfa_size = LS1028A_TFA_RESERVED_SIZE;
	struct resource *res;

	if (!cpu_is_ls1028a())
		return 0;

	res = reserve_sdram_region("tfa", tfa_start, tfa_size);
	if (!res) {
		pr_err("Cannot request SDRAM region %pa - %pa\n", &tfa_start, &tfa_size);
                return -EINVAL;
	}

	of_register_fixup(of_fixup_reserved_memory, res);

	return 0;
}
mmu_initcall(ls1028a_reserve_tfa);

#define DWC3_GSBUSCFG0				0xc100
#define DWC3_GSBUSCFG0_CACHETYPE_MASK		GENMASK(31, 16)

static void layerscape_usb_enable_snooping(void)
{
	struct device_node *np;

	for_each_compatible_node(np, NULL, "snps,dwc3") {
		struct resource res;

		if (of_address_to_resource(np, 0, &res))
			continue;

		/* Set cacheable bit for all of Data read, Descriptor read,
		 * Data write and Descriptor write. Bufferable and read/write
		 * allocate bits are not set. This is the recommended configurationr
		 * in LS1046ARM Rev. 3 34.2.10.2:
		 * "For master interface DMA access, program the GSBUSCFG0
		 * register to 0x2222000F for better performance.".
		 * The 0x000F is configured via snps,incr-burst-type-adjustment
		 * (which despite the name is Layerscape-specific), so below
		 * line only manipulates the upper 16 bits.
		 */
		clrsetbits_le32(IOMEM(res.start) + DWC3_GSBUSCFG0,
				DWC3_GSBUSCFG0_CACHETYPE_MASK,
				FIELD_PREP(DWC3_GSBUSCFG0_CACHETYPE_MASK, 0x2222));
	}
}

static int ls1046a_init(void)
{
	if (!cpu_is_ls1046a())
		return -EINVAL;

	ls1046a_bootsource_init();
	ls1046a_setup_icids();
	layerscape_register_pbl_image_handler();
	layerscape_usb_enable_snooping();

	return 0;
}

static int layerscape_init(void)
{
	struct device_node *root;

	root = of_get_root_node();
	if (root) {
		__layerscape_soc_type = layerscape_soc_from_dt();
		if (!__layerscape_soc_type)
			return 0;
	}

	switch (__layerscape_soc_type) {
	case LAYERSCAPE_SOC_LS1021A:
		return ls1021a_init();
	case LAYERSCAPE_SOC_LS1028A:
		return ls1028a_init();
	case LAYERSCAPE_SOC_LS1046A:
		return ls1046a_init();
	}

	return 0;
}
postcore_initcall(layerscape_init);
