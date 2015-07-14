#ifndef __MACH_MMDC_H
#define __MACH_MMDC_H

#include <mach/imx6-regs.h>

#define P0_IPS (void __iomem *)MX6_MMDC_P0_BASE_ADDR
#define P1_IPS (void __iomem *)MX6_MMDC_P1_BASE_ADDR

#define MDCTL		0x000
#define MDPDC		0x004
#define MDSCR		0x01c
#define MDMISC		0x018
#define MDREF		0x020
#define MAPSR		0x404
#define MPZQHWCTRL	0x800
#define MPWLGCR		0x808
#define MPWLDECTRL0	0x80c
#define MPWLDECTRL1	0x810
#define MPPDCMPR1	0x88c
#define MPSWDAR		0x894
#define MPRDDLCTL	0x848
#define MPMUR		0x8b8
#define MPDGCTRL0	0x83c
#define MPDGCTRL1	0x840
#define MPRDDLCTL	0x848
#define MPWRDLCTL	0x850
#define MPRDDLHWCTL	0x860
#define MPWRDLHWCTL	0x864
#define MPDGHWST0	0x87c
#define MPDGHWST1	0x880
#define MPDGHWST2	0x884
#define MPDGHWST3	0x888

#define MMDCx_MDCTL_SDE0	0x80000000
#define MMDCx_MDCTL_SDE1	0x40000000

#define MMDCx_MDCTL_DSIZ_16B	0x00000000
#define MMDCx_MDCTL_DSIZ_32B	0x00010000
#define MMDCx_MDCTL_DSIZ_64B	0x00020000

#define MMDCx_MDMISC_DDR_4_BANKS	0x00000020

#define IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS0	((void __iomem *)MX6_IOMUXC_BASE_ADDR + 0x5a8)
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS1	((void __iomem *)MX6_IOMUXC_BASE_ADDR + 0x5b0)
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS2	((void __iomem *)MX6_IOMUXC_BASE_ADDR + 0x524)
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS3	((void __iomem *)MX6_IOMUXC_BASE_ADDR + 0x51c)
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS4	((void __iomem *)MX6_IOMUXC_BASE_ADDR + 0x518)
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS5	((void __iomem *)MX6_IOMUXC_BASE_ADDR + 0x50c)
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS6	((void __iomem *)MX6_IOMUXC_BASE_ADDR + 0x5b8)
#define IOMUXC_SW_PAD_CTL_PAD_DRAM_SDQS7	((void __iomem *)MX6_IOMUXC_BASE_ADDR + 0x5c0)


int mmdc_do_write_level_calibration(void);
int mmdc_do_dqs_calibration(void);
void mmdc_print_calibration_results(void);

/* MMDC P0/P1 Registers */
struct mmdc_p_regs {
	u32 mdctl;
	u32 mdpdc;
	u32 mdotc;
	u32 mdcfg0;
	u32 mdcfg1;
	u32 mdcfg2;
	u32 mdmisc;
	u32 mdscr;
	u32 mdref;
	u32 res1[2];
	u32 mdrwd;
	u32 mdor;
	u32 res2[3];
	u32 mdasp;
	u32 res3[240];
	u32 mapsr;
	u32 res4[254];
	u32 mpzqhwctrl;
	u32 res5[2];
	u32 mpwldectrl0;
	u32 mpwldectrl1;
	u32 res6;
	u32 mpodtctrl;
	u32 mprddqby0dl;
	u32 mprddqby1dl;
	u32 mprddqby2dl;
	u32 mprddqby3dl;
	u32 res7[4];
	u32 mpdgctrl0;
	u32 mpdgctrl1;
	u32 res8;
	u32 mprddlctl;
	u32 res9;
	u32 mpwrdlctl;
	u32 res10[25];
	u32 mpmur0;
};

#define MX6SX_IOM_DDR_BASE	0x020e0200
struct mx6sx_iomux_ddr_regs {
	u32 res1[59];
	u32 dram_dqm0;
	u32 dram_dqm1;
	u32 dram_dqm2;
	u32 dram_dqm3;
	u32 dram_ras;
	u32 dram_cas;
	u32 res2[2];
	u32 dram_sdwe_b;
	u32 dram_odt0;
	u32 dram_odt1;
	u32 dram_sdba0;
	u32 dram_sdba1;
	u32 dram_sdba2;
	u32 dram_sdcke0;
	u32 dram_sdcke1;
	u32 dram_sdclk_0;
	u32 dram_sdqs0;
	u32 dram_sdqs1;
	u32 dram_sdqs2;
	u32 dram_sdqs3;
	u32 dram_reset;
};

#define MX6SX_IOM_GRP_BASE	0x020e0500
struct mx6sx_iomux_grp_regs {
	u32 res1[61];
	u32 grp_addds;
	u32 grp_ddrmode_ctl;
	u32 grp_ddrpke;
	u32 grp_ddrpk;
	u32 grp_ddrhys;
	u32 grp_ddrmode;
	u32 grp_b0ds;
	u32 grp_b1ds;
	u32 grp_ctlds;
	u32 grp_ddr_type;
	u32 grp_b2ds;
	u32 grp_b3ds;
};

/*
 * MMDC iomux registers (pinctl/padctl) - (different for IMX6DQ vs IMX6SDL)
 */
#define MX6DQ_IOM_DDR_BASE      0x020e0500
struct mx6dq_iomux_ddr_regs {
	u32 res1[3];
	u32 dram_sdqs5;
	u32 dram_dqm5;
	u32 dram_dqm4;
	u32 dram_sdqs4;
	u32 dram_sdqs3;
	u32 dram_dqm3;
	u32 dram_sdqs2;
	u32 dram_dqm2;
	u32 res2[16];
	u32 dram_cas;
	u32 res3[2];
	u32 dram_ras;
	u32 dram_reset;
	u32 res4[2];
	u32 dram_sdclk_0;
	u32 dram_sdba2;
	u32 dram_sdcke0;
	u32 dram_sdclk_1;
	u32 dram_sdcke1;
	u32 dram_sdodt0;
	u32 dram_sdodt1;
	u32 res5;
	u32 dram_sdqs0;
	u32 dram_dqm0;
	u32 dram_sdqs1;
	u32 dram_dqm1;
	u32 dram_sdqs6;
	u32 dram_dqm6;
	u32 dram_sdqs7;
	u32 dram_dqm7;
};

#define MX6DQ_IOM_GRP_BASE      0x020e0700
struct mx6dq_iomux_grp_regs {
	u32 res1[18];
	u32 grp_b7ds;
	u32 grp_addds;
	u32 grp_ddrmode_ctl;
	u32 res2;
	u32 grp_ddrpke;
	u32 res3[6];
	u32 grp_ddrmode;
	u32 res4[3];
	u32 grp_b0ds;
	u32 grp_b1ds;
	u32 grp_ctlds;
	u32 res5;
	u32 grp_b2ds;
	u32 grp_ddr_type;
	u32 grp_b3ds;
	u32 grp_b4ds;
	u32 grp_b5ds;
	u32 grp_b6ds;
};

#define MX6SDL_IOM_DDR_BASE     0x020e0400
struct mx6sdl_iomux_ddr_regs {
	u32 res1[25];
	u32 dram_cas;
	u32 res2[2];
	u32 dram_dqm0;
	u32 dram_dqm1;
	u32 dram_dqm2;
	u32 dram_dqm3;
	u32 dram_dqm4;
	u32 dram_dqm5;
	u32 dram_dqm6;
	u32 dram_dqm7;
	u32 dram_ras;
	u32 dram_reset;
	u32 res3[2];
	u32 dram_sdba2;
	u32 dram_sdcke0;
	u32 dram_sdcke1;
	u32 dram_sdclk_0;
	u32 dram_sdclk_1;
	u32 dram_sdodt0;
	u32 dram_sdodt1;
	u32 dram_sdqs0;
	u32 dram_sdqs1;
	u32 dram_sdqs2;
	u32 dram_sdqs3;
	u32 dram_sdqs4;
	u32 dram_sdqs5;
	u32 dram_sdqs6;
	u32 dram_sdqs7;
};

#define MX6SDL_IOM_GRP_BASE     0x020e0700
struct mx6sdl_iomux_grp_regs {
	u32 res1[18];
	u32 grp_b7ds;
	u32 grp_addds;
	u32 grp_ddrmode_ctl;
	u32 grp_ddrpke;
	u32 res2[2];
	u32 grp_ddrmode;
	u32 grp_b0ds;
	u32 res3;
	u32 grp_ctlds;
	u32 grp_b1ds;
	u32 grp_ddr_type;
	u32 grp_b2ds;
	u32 grp_b3ds;
	u32 grp_b4ds;
	u32 grp_b5ds;
	u32 res4;
	u32 grp_b6ds;
};

/* Device Information: Varies per DDR3 part number and speed grade */
struct mx6_ddr3_cfg {
	u16 mem_speed;	/* ie 1600 for DDR3-1600 (800,1066,1333,1600) */
	u8 density;	/* chip density (Gb) (1,2,4,8) */
	u8 width;	/* bus width (bits) (4,8,16) */
	u8 banks;	/* number of banks */
	u8 rowaddr;	/* row address bits (11-16)*/
	u8 coladdr;	/* col address bits (9-12) */
	u8 pagesz;	/* page size (K) (1-2) */
	u16 trcd;	/* tRCD=tRP=CL (ns*100) */
	u16 trcmin;	/* tRC min (ns*100) */
	u16 trasmin;	/* tRAS min (ns*100) */
	u8 SRT;		/* self-refresh temperature: 0=normal, 1=extended */
};

/* System Information: Varies per board design, layout, and term choices */
struct mx6_ddr_sysinfo {
	u8 dsize;	/* size of bus (in dwords: 0=16bit,1=32bit,2=64bit) */
	u8 cs_density;	/* density per chip select (Gb) */
	u8 ncs;		/* number chip selects used (1|2) */
	char cs1_mirror;/* enable address mirror (0|1) */
	char bi_on;	/* Bank interleaving enable */
	u8 rtt_nom;	/* Rtt_Nom (DDR3_RTT_*) */
	u8 rtt_wr;	/* Rtt_Wr (DDR3_RTT_*) */
	u8 ralat;	/* Read Additional Latency (0-7) */
	u8 walat;	/* Write Additional Latency (0-3) */
	u8 mif3_mode;	/* Command prediction working mode */
	u8 rst_to_cke;	/* Time from SDE enable to CKE rise */
	u8 sde_to_rst;	/* Time from SDE enable until DDR reset# is high */
	u8 pd_fast_exit;/* enable precharge powerdown fast-exit */
};

/*
 * Board specific calibration:
 *   This includes write leveling calibration values as well as DQS gating
 *   and read/write delays. These values are board/layout/device specific.
 *   Freescale recommends using the i.MX6 DDR Stress Test Tool V1.0.2
 *   (DOC-96412) to determine these values over a range of boards and
 *   temperatures.
 */
struct mx6_mmdc_calibration {
	/* write leveling calibration */
	u32 p0_mpwldectrl0;
	u32 p0_mpwldectrl1;
	u32 p1_mpwldectrl0;
	u32 p1_mpwldectrl1;
	/* read DQS gating */
	u32 p0_mpdgctrl0;
	u32 p0_mpdgctrl1;
	u32 p1_mpdgctrl0;
	u32 p1_mpdgctrl1;
	/* read delay */
	u32 p0_mprddlctl;
	u32 p1_mprddlctl;
	/* write delay */
	u32 p0_mpwrdlctl;
	u32 p1_mpwrdlctl;
};

/* configure iomux (pinctl/padctl) */
void mx6dq_dram_iocfg(unsigned width,
		      const struct mx6dq_iomux_ddr_regs *,
		      const struct mx6dq_iomux_grp_regs *);
void mx6sdl_dram_iocfg(unsigned width,
		       const struct mx6sdl_iomux_ddr_regs *,
		       const struct mx6sdl_iomux_grp_regs *);
void mx6sx_dram_iocfg(unsigned width,
		      const struct mx6sx_iomux_ddr_regs *,
		      const struct mx6sx_iomux_grp_regs *);

/* configure mx6 mmdc registers */
void mx6_dram_cfg(const struct mx6_ddr_sysinfo *,
		  const struct mx6_mmdc_calibration *,
		  const struct mx6_ddr3_cfg *);

#endif /* __MACH_MMDC_H */
