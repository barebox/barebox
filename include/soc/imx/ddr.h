/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2017 NXP
 */

#ifndef __SOC_IMX_DDR_H
#define __SOC_IMX_DDR_H

/* user data type */
enum fw_type {
	FW_1D_IMAGE,
	FW_2D_IMAGE,
};

enum dram_type {
#define DRAM_TYPE_MASK	0x00ff
	DRAM_TYPE_LPDDR4	= 0 << 0,
	DRAM_TYPE_DDR4		= 1 << 0,
};

static inline enum dram_type get_dram_type(unsigned type)
{
	return type & DRAM_TYPE_MASK;
}

enum ddrc_type {
#define DDRC_TYPE_MASK	0xff00
	DDRC_TYPE_MM = 0 << 8,
	DDRC_TYPE_MN = 1 << 8,
	DDRC_TYPE_MQ = 2 << 8,
	DDRC_TYPE_MP = 3 << 8,
};

static inline enum ddrc_type get_ddrc_type(unsigned type)
{
	return type & DDRC_TYPE_MASK;
}

struct dram_cfg_param {
	unsigned int reg;
	unsigned int val;
};

struct dram_fsp_cfg {
	struct dram_cfg_param ddrc_cfg[20];
	struct dram_cfg_param mr_cfg[10];
	unsigned int bypass;
};

struct dram_fsp_msg {
	unsigned int drate;
	enum fw_type fw_type;
	struct dram_cfg_param *fsp_cfg;
	unsigned int fsp_cfg_num;
};

struct dram_timing_info {
	/* umctl2 config */
	struct dram_cfg_param *ddrc_cfg;
	unsigned int ddrc_cfg_num;
	/* fsp config */
	struct dram_fsp_cfg *fsp_cfg;
	unsigned int fsp_cfg_num;
	/* ddrphy config */
	struct dram_cfg_param *ddrphy_cfg;
	unsigned int ddrphy_cfg_num;
	/* ddr fsp train info */
	struct dram_fsp_msg *fsp_msg;
	unsigned int fsp_msg_num;
	/* ddr phy trained CSR */
	struct dram_cfg_param *ddrphy_trained_csr;
	unsigned int ddrphy_trained_csr_num;
	/* ddr phy PIE */
	struct dram_cfg_param *ddrphy_pie;
	unsigned int ddrphy_pie_num;
	/* initialized drate table */
	unsigned int fsp_table[4];
};

struct dram_controller {
	enum ddrc_type ddrc_type;
	enum dram_type dram_type;
	void __iomem *phy_base;
	u32 (*phy_remap)(u32 paddr_apb_from_ctlr);
	void (*get_trained_CDD)(struct dram_controller *dram, u32 fsp);
	void (*set_dfi_clk)(struct dram_controller *dram, unsigned int drate_mhz);
	bool imx8m_ddr_old_spreadsheet;
};

void ddr_get_firmware_lpddr4(void);
void ddr_get_firmware_ddr(void);

static inline void ddr_get_firmware(enum dram_type dram_type)
{
	if (dram_type == DRAM_TYPE_LPDDR4)
		ddr_get_firmware_lpddr4();
	else
		ddr_get_firmware_ddr();
}

int ddr_cfg_phy(struct dram_controller *dram, struct dram_timing_info *timing_info);
void ddrphy_trained_csr_save(struct dram_controller *dram, struct dram_cfg_param *param,
			     unsigned int num);
void *dram_config_save(struct dram_controller *dram, struct dram_timing_info *info,
		       unsigned long base);

/* utils function for ddr phy training */
int wait_ddrphy_training_complete(struct dram_controller *dram);

#define reg32_write(a, v)	writel(v, a)
#define reg32_read(a)		readl(a)

static inline void reg32setbit(unsigned long addr, u32 bit)
{
	setbits_le32(addr, (1 << bit));
}

static inline void *dwc_ddrphy_apb_addr(struct dram_controller *dram, unsigned int addr)
{
	if (dram->phy_remap)
		addr = dram->phy_remap(addr);
	else
		addr *= 4;

	return dram->phy_base + addr;
}

static inline void dwc_ddrphy_apb_wr(struct dram_controller *dram, unsigned int addr, u32 data)
{
	reg32_write(dwc_ddrphy_apb_addr(dram, addr), data);
}

static inline u32 dwc_ddrphy_apb_rd(struct dram_controller *dram, unsigned int addr)
{
	return reg32_read(dwc_ddrphy_apb_addr(dram, addr));
}

extern struct dram_cfg_param ddrphy_trained_csr[];
extern uint32_t ddrphy_trained_csr_num;

enum ddrc_phy_firmware_offset {
	DDRC_PHY_IMEM = 0x00050000U,
	DDRC_PHY_DMEM = 0x00054000U,
};

void ddr_load_train_code(struct dram_controller *dram, enum dram_type dram_type,
			 enum fw_type fw_type);

void ddrc_phy_load_firmware(struct dram_controller *dram,
			    enum ddrc_phy_firmware_offset,
			    const u16 *, size_t);

static inline bool dram_is_lpddr4(enum dram_type dram_type)
{
	return IS_ENABLED(CONFIG_FIRMWARE_IMX_LPDDR4_PMU_TRAIN) &&
		dram_type == DRAM_TYPE_LPDDR4;
}

static inline bool dram_is_ddr4(enum dram_type dram_type)
{
	return IS_ENABLED(CONFIG_FIRMWARE_IMX_DDR4_PMU_TRAIN) &&
		dram_type == DRAM_TYPE_DDR4;
}

#define DDRC_PHY_REG(x)	((x) * 4)

#endif /* __SOC_IMX_DDR_H */