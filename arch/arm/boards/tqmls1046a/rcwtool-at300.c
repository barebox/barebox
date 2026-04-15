
#include <ls1046a-rcwtool.h>

int main(void)
{
	printf("# RCW values:\n");
	printf("\n");

	set_val(SYS_PLL_CFG              ,       0);
	set_val(SYS_PLL_RAT              ,       6);
	set_val(MEM_PLL_CFG              ,       0);
	set_val(MEM_PLL_RAT              ,      20);
	set_val(CGA_PLL1_CFG             ,       0);
	set_val(CGA_PLL1_RAT             ,      16);
	set_val(CGA_PLL2_CFG             ,       0);
	set_val(CGA_PLL2_RAT             ,      14);
	set_val(C1_PLL_SEL               ,       0);
	set_val(SRDS_PRTCL_S1            ,  0x1133); /* XFI9 (10G): BASE-FPGA, XFI10 (10G): BASE-FPGA, SGMII5 (1G): Unused, SGMII6 (1G): Midplane/MGMT-Module */
	set_val(SRDS_PRTCL_S2            ,  0x8888); /* PCIe1 Gen3 x4 */
	set_val(SRDS_PLL_REF_CLK_SEL_S1_PLL1  ,  0); /* SGMII: 100MHz */
	set_val(SRDS_PLL_REF_CLK_SEL_S1_PLL2  ,  1); /* XFI: 156.25MHz */
	set_val(SRDS_PLL_REF_CLK_SEL_S2_PLL1  ,  0); /* PCIe: 100MHz */
	set_val(SRDS_PLL_REF_CLK_SEL_S2_PLL2  ,  0); /* unused: not connected */
	set_val(SRDS_PLL_PD_S1           ,       0);
	set_val(SRDS_PLL_PD_S2           ,       0); /* Note: PLL1 needed for single reference clock */
	set_val(SRDS_DIV_PEX_S1          ,       0); /* PCIe: train up to a max rate of 8G (unused) */
	set_val(SRDS_DIV_PEX_S2          ,       0); /* PCIe: train up to a max rate of 8G (used for PCIe1 Gen3 x4) */
	set_val(DDR_REFCLK_SEL           ,       0);
	set_val(SRDS_REFCLK_SEL_S1       ,       0); /* Separate reference clocks to both PLLs */
	set_val(SRDS_REFCLK_SEL_S2       ,       1); /* Single reference clock (SD2_REF_CLK1) to both PLLs */
	set_val(DDR_FDBK_MULT            ,       2);
	set_val(PBI_SRC                  ,       4); /* QSPI */
	set_val(BOOT_HO                  ,       0);
	set_val(SB_EN                    ,       0);
	set_val(IFC_MODE                 ,      37);
	set_val(HWA_CGA_M1_CLK_SEL       ,       6);
	set_val(DRAM_LAT                 ,       1);
	set_val(DDR_RATE                 ,       0);
	set_val(DDR_RSV0                 ,       0);
	set_val(SYS_PLL_SPD              ,       0);
	set_val(MEM_PLL_SPD              ,       0);
	set_val(CGA_PLL1_SPD             ,       0);
	set_val(CGA_PLL2_SPD             ,       0);
	set_val(HOST_AGT_PEX             ,       0);
	set_val(GP_INFO1                 ,       0);
	set_val(GP_INFO2                 ,       0);
	set_val(UART_EXT                 ,       0); /* see UART_BASE field definition */
	set_val(IRQ_EXT                  ,       0); /* 0 => GPIO => see IRQ_BASE field definition */
	set_val(SPI_EXT                  ,       0); /* see SPI_BASE field definition */
	set_val(SDHC_EXT                 ,       0); /* see SDHC_BASE field definition */
	set_val(UART_BASE                ,       6); /* 6 => UART1/2 including RTX/CTS */
	set_val(ASLEEP                   ,       1); /* GPIO1_13 */
	set_val(RTC                      ,       1); /* GPIO1_14 */
	set_val(SDHC_BASE                ,       0); /* internal eMMC */
	set_val(IRQ_OUT                  ,       1); /* reserved, must be 1 */
	set_val(IRQ_BASE                 ,   0x1FE); /* GPIO1[23:30] */
	set_val(SPI_BASE                 ,       0); /* SPI mode -> FRAM */
	set_val(IFC_GRP_A_EXT            ,       1); /* 1 => QSPI_A_DATA[3], needed for boot flash? */
	set_val(IFC_GRP_D_EXT            ,       0);
	set_val(IFC_GRP_E1_EXT           ,       0);
	set_val(IFC_GRP_F_EXT            ,       1); /* 1 = QSPI needed for boot flash? */
	set_val(IFC_GRP_E1_BASE          ,       1); /* GPIO2[10:12] */
	set_val(IFC_GRP_D_BASE           ,       1); /* GPIO2[13:15] */
	set_val(IFC_GRP_A_BASE           ,       1); /* GPIO2[25:27] */
	set_val(IFC_A_22_24              ,       0);
	set_val(EC1                      ,       1); /* GPIO3 (No RGMII) */
	set_val(EC2                      ,       1); /* GPIO3 (No RGMII) */
	set_val(LVDD_VSEL                ,       0); /* 1.8V */
	set_val(I2C_IPGCLK_SEL           ,       0);
	set_val(EM1                      ,       1); /* GPIO3 (No RGMII) */
	set_val(EM2                      ,       1); /* GPIO3 (No RGMII) */
	set_val(EMI2_DMODE               ,       1);
	set_val(EMI2_CMODE               ,       1);
	set_val(USB_DRVVBUS              ,       0);
	set_val(USB_PWRFAULT             ,       0);
	set_val(TVDD_VSEL                ,       0); /* 1.8V */
	set_val(DVDD_VSEL                ,       0); /* 1.8V */
	set_val(EMI1_DMODE               ,       1);
	set_val(EVDD_VSEL                ,       0);
	set_val(IIC2_BASE                ,       0); /* must be 0b000 */
	set_val(EMI1_CMODE               ,       1);
	set_val(IIC2_EXT                 ,       0); /* 0 => I2C2, i2c_clkgen_cpu */
	set_val(SYSCLK_FREQ              ,     600);
	set_val(HWA_CGA_M2_CLK_SEL       ,       1);

	print_rcw(rcw);

	return 0;
}
