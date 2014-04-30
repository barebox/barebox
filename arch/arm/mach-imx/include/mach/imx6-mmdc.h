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

#endif /* __MACH_MMDC_H */
