// SPDX-License-Identifier: GPL-2.0+
#include <common.h>
#include <io.h>
#include <soc/fsl/immap_lsch2.h>
#include <soc/fsl/fsl_ddr_sdram.h>
#include <asm/system.h>
#include <mach/layerscape/errata.h>
#include <mach/layerscape/lowlevel.h>
#include <soc/fsl/scfg.h>

static inline void set_usb_pcstxswingfull(u32 __iomem *scfg, u32 offset)
{
	scfg_clrsetbits32(scfg + offset / 4,
			0x7f << 9,
			SCFG_USB_PCSTXSWINGFULL << 9);
}

static void erratum_a008997_ls1021a(void)
{
	u32 __iomem *scfg = (u32 __iomem *)LSCH2_SCFG_ADDR;

	set_usb_pcstxswingfull(scfg, SCFG_USB3PRM2CR_USB1);
}

static void erratum_a008997_ls1046a(void)
{
	u32 __iomem *scfg = (u32 __iomem *)LSCH2_SCFG_ADDR;

	set_usb_pcstxswingfull(scfg, SCFG_USB3PRM2CR_USB1);
	set_usb_pcstxswingfull(scfg, SCFG_USB3PRM2CR_USB2);
	set_usb_pcstxswingfull(scfg, SCFG_USB3PRM2CR_USB3);
}

static void erratum_a009007(void __iomem *phy, u16 val1, u16 val2, u16 val3, u16 val4)
{
	scfg_out16(phy + SCFG_USB_PHY_RX_OVRD_IN_HI, val1);
	scfg_out16(phy + SCFG_USB_PHY_RX_OVRD_IN_HI, val2);
	scfg_out16(phy + SCFG_USB_PHY_RX_OVRD_IN_HI, val3);
	scfg_out16(phy + SCFG_USB_PHY_RX_OVRD_IN_HI, val4);
}

static void erratum_a009007_ls1046a(void)
{
	erratum_a009007(IOMEM(SCFG_USB_PHY1), 0x0000, 0x0080, 0x0380, 0x0b80);
	erratum_a009007(IOMEM(SCFG_USB_PHY2), 0x0000, 0x0080, 0x0380, 0x0b80);
	erratum_a009007(IOMEM(SCFG_USB_PHY3), 0x0000, 0x0080, 0x0380, 0x0b80);
}

static void erratum_a009007_ls1021a(void)
{
	erratum_a009007(IOMEM(SCFG_USB_PHY1), 0x0000, 0x8000, 0x8004, 0x800C);
}

static inline void set_usb_txvreftune(u32 __iomem *scfg, u32 offset)
{
	scfg_clrsetbits32(scfg + offset / 4, 0xf << 6, SCFG_USB_TXVREFTUNE << 6);
}

static void erratum_a009008_layerscape(void)
{
	u32 __iomem *scfg = IOMEM(LSCH2_SCFG_ADDR);

	set_usb_txvreftune(scfg, SCFG_USB3PRM1CR_USB1);
}

static void erratum_a009008_ls1046a(void)
{
	u32 __iomem *scfg = IOMEM(LSCH2_SCFG_ADDR);

	set_usb_txvreftune(scfg, SCFG_USB3PRM1CR_USB2);
	set_usb_txvreftune(scfg, SCFG_USB3PRM1CR_USB3);
}

static inline void set_usb_sqrxtune(u32 __iomem *scfg, u32 offset)
{
	scfg_clrbits32(scfg + offset / 4, SCFG_USB_SQRXTUNE_MASK << 23);
}

static void erratum_a009798_ls1021a(void)
{
	u32 __iomem *scfg = IOMEM(LSCH2_SCFG_ADDR);

	set_usb_sqrxtune(scfg, SCFG_USB3PRM1CR_USB1);
}

static void erratum_a009798_ls1046a(void)
{
	u32 __iomem *scfg = IOMEM(LSCH2_SCFG_ADDR);

	set_usb_sqrxtune(scfg, SCFG_USB3PRM1CR_USB1);
	set_usb_sqrxtune(scfg, SCFG_USB3PRM1CR_USB2);
	set_usb_sqrxtune(scfg, SCFG_USB3PRM1CR_USB3);
}

static void erratum_a008850_early(void)
{
	/* part 1 of 2 */
	struct ccsr_cci400 __iomem *cci = IOMEM(LSCH2_CCI400_ADDR);
	struct ccsr_ddr __iomem *ddr = IOMEM(LSCH2_DDR_ADDR);

	/* Skip if running at lower exception level */
#if __LINUX_ARM_ARCH__ > 7
		if (current_el() < 3)
			return;
#endif

	/* disables propagation of barrier transactions to DDRC from CCI400 */
	out_le32(&cci->ctrl_ord, CCI400_CTRLORD_TERM_BARRIER);

	/* disable the re-ordering in DDRC */
	ddr_out32(&ddr->eor, DDR_EOR_RD_REOD_DIS | DDR_EOR_WD_REOD_DIS);
}

static void layerscape_errata(void)
{
	erratum_a008850_early();
	erratum_a009008_layerscape();
}

void ls1046a_errata(void)
{
	layerscape_errata();
	erratum_a009008_ls1046a();
	erratum_a009798_ls1046a();
	erratum_a008997_ls1046a();
	erratum_a009007_ls1046a();
}

void ls1021a_errata(void)
{
	layerscape_errata();
	erratum_a009798_ls1021a();
	erratum_a008997_ls1021a();
	erratum_a009007_ls1021a();
}

static void erratum_a008850_post(void)
{
	/* part 2 of 2 */
	struct ccsr_cci400 __iomem *cci = IOMEM(LSCH2_CCI400_ADDR);
	struct ccsr_ddr __iomem *ddr = IOMEM(LSCH2_DDR_ADDR);
	u32 tmp;

	/* Skip if running at lower exception level */
#if __LINUX_ARM_ARCH__ > 7
		if (current_el() < 3)
			return;
#endif

	/* enable propagation of barrier transactions to DDRC from CCI400 */
	out_le32(&cci->ctrl_ord, CCI400_CTRLORD_EN_BARRIER);

	/* enable the re-ordering in DDRC */
	tmp = ddr_in32(&ddr->eor);
	tmp &= ~(DDR_EOR_RD_REOD_DIS | DDR_EOR_WD_REOD_DIS);
	ddr_out32(&ddr->eor, tmp);
}

/*
 * This additional workaround of A009942 checks the condition to determine if
 * the CPO value set by the existing A009942 workaround needs to be updated.
 * If need, print a warning to prompt user reconfigure DDR debug_29[24:31] with
 * expected optimal value, the optimal value is highly board dependent.
 */
static void erratum_a009942_check_cpo(void)
{
	struct ccsr_ddr __iomem *ddr =
		(struct ccsr_ddr __iomem *)(LSCH2_DDR_ADDR);
	u32 cpo, cpo_e, cpo_o, cpo_target, cpo_optimal;
	u32 cpo_min = ddr_in32(&ddr->debug[9]) >> 24;
	u32 cpo_max = cpo_min;
	u32 sdram_cfg, i, tmp, lanes, ddr_type;
	bool update_cpo = false, has_ecc = false;

	sdram_cfg = ddr_in32(&ddr->sdram_cfg);
	if (sdram_cfg & SDRAM_CFG_32_BE)
		lanes = 4;
	else if (sdram_cfg & SDRAM_CFG_16_BE)
		lanes = 2;
	else
		lanes = 8;

	if (sdram_cfg & SDRAM_CFG_ECC_EN)
		has_ecc = true;

	/* determine the maximum and minimum CPO values */
	for (i = 9; i < 9 + lanes / 2; i++) {
		cpo = ddr_in32(&ddr->debug[i]);
		cpo_e = cpo >> 24;
		cpo_o = (cpo >> 8) & 0xff;
		tmp = min(cpo_e, cpo_o);
		if (tmp < cpo_min)
			cpo_min = tmp;
		tmp = max(cpo_e, cpo_o);
		if (tmp > cpo_max)
			cpo_max = tmp;
	}

	if (has_ecc) {
		cpo = ddr_in32(&ddr->debug[13]);
		cpo = cpo >> 24;
		if (cpo < cpo_min)
			cpo_min = cpo;
		if (cpo > cpo_max)
			cpo_max = cpo;
	}

	cpo_target = ddr_in32(&ddr->debug[28]) & 0xff;
	cpo_optimal = ((cpo_max + cpo_min) >> 1) + 0x27;
	debug("cpo_optimal = 0x%x, cpo_target = 0x%x\n", cpo_optimal,
	      cpo_target);
	debug("cpo_max = 0x%x, cpo_min = 0x%x\n", cpo_max, cpo_min);

	ddr_type = (sdram_cfg & SDRAM_CFG_SDRAM_TYPE_MASK) >>
		    SDRAM_CFG_SDRAM_TYPE_SHIFT;
	if (ddr_type == SDRAM_TYPE_DDR4)
		update_cpo = (cpo_min + 0x3b) < cpo_target ? true : false;
	else if (ddr_type == SDRAM_TYPE_DDR3)
		update_cpo = (cpo_min + 0x3f) < cpo_target ? true : false;

	if (update_cpo) {
		printf("WARN: pls set popts->cpo_sample = 0x%x ", cpo_optimal);
		printf("in <board>/ddr.c to optimize cpo\n");
	}
}

void ls1046a_errata_post_ddr(void)
{
	erratum_a008850_post();
	erratum_a009942_check_cpo();
}

void ls1021a_errata_post_ddr(void)
{
	erratum_a008850_post();
}
