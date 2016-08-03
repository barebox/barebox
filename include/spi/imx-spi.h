#ifndef __SPI_IMX_SPI_H
#define __SPI_IMX_SPI_H

#define CSPI_0_0_RXDATA		0x00
#define CSPI_0_0_TXDATA		0x04
#define CSPI_0_0_CTRL		0x08
#define CSPI_0_0_INT		0x0C
#define CSPI_0_0_DMA		0x18
#define CSPI_0_0_STAT		0x0C
#define CSPI_0_0_PERIOD		0x14
#define CSPI_0_0_TEST		0x10
#define CSPI_0_0_RESET		0x1C

#define CSPI_0_0_CTRL_ENABLE		(1 << 10)
#define CSPI_0_0_CTRL_MASTER		(1 << 11)
#define CSPI_0_0_CTRL_XCH		(1 << 9)
#define CSPI_0_0_CTRL_LOWPOL		(1 << 5)
#define CSPI_0_0_CTRL_PHA		(1 << 6)
#define CSPI_0_0_CTRL_SSCTL		(1 << 7)
#define CSPI_0_0_CTRL_HIGHSSPOL 	(1 << 8)
#define CSPI_0_0_CTRL_CS(x)		(((x) & 0x3) << 19)
#define CSPI_0_0_CTRL_BITCOUNT(x)	(((x) & 0x1f) << 0)
#define CSPI_0_0_CTRL_DATARATE(x)	(((x) & 0x7) << 14)

#define CSPI_0_0_CTRL_MAXDATRATE	0x10
#define CSPI_0_0_CTRL_DATAMASK		0x1F
#define CSPI_0_0_CTRL_DATASHIFT 	14

#define CSPI_0_0_STAT_TE		(1 << 0)
#define CSPI_0_0_STAT_TH		(1 << 1)
#define CSPI_0_0_STAT_TF		(1 << 2)
#define CSPI_0_0_STAT_RR		(1 << 4)
#define CSPI_0_0_STAT_RH		(1 << 5)
#define CSPI_0_0_STAT_RF		(1 << 6)
#define CSPI_0_0_STAT_RO		(1 << 7)

#define CSPI_0_0_PERIOD_32KHZ		(1 << 15)

#define CSPI_0_0_TEST_LBC		(1 << 14)

#define CSPI_0_0_RESET_START		(1 << 0)

#define CSPI_0_4_CTRL			0x08
#define CSPI_0_4_CTRL_BL_SHIFT		8
#define CSPI_0_4_CTRL_BL_MASK		0x1f
#define CSPI_0_4_CTRL_DRCTL_SHIFT	20
#define CSPI_0_4_CTRL_CS_SHIFT		24

#define CSPI_0_7_RXDATA			0x00
#define CSPI_0_7_TXDATA			0x04
#define CSPI_0_7_CTRL			0x08
#define CSPI_0_7_CTRL_ENABLE		(1 << 0)
#define CSPI_0_7_CTRL_MASTER		(1 << 1)
#define CSPI_0_7_CTRL_XCH		(1 << 2)
#define CSPI_0_7_CTRL_POL		(1 << 4)
#define CSPI_0_7_CTRL_PHA		(1 << 5)
#define CSPI_0_7_CTRL_SSCTL		(1 << 6)
#define CSPI_0_7_CTRL_SSPOL		(1 << 7)
#define CSPI_0_7_CTRL_DRCTL_SHIFT	8
#define CSPI_0_7_CTRL_CS_SHIFT		12
#define CSPI_0_7_CTRL_DR_SHIFT		16
#define CSPI_0_7_CTRL_BL_SHIFT		20
#define CSPI_0_7_STAT			0x14
#define CSPI_0_7_STAT_RR		(1 << 3)

#define CSPI_2_3_RXDATA		0x00
#define CSPI_2_3_TXDATA		0x04
#define CSPI_2_3_CTRL		0x08
#define CSPI_2_3_CTRL_ENABLE		(1 <<  0)
#define CSPI_2_3_CTRL_XCH		(1 <<  2)
#define CSPI_2_3_CTRL_SMC		(1 <<  3)
#define CSPI_2_3_CTRL_MODE(cs)	(1 << ((cs) +  4))
#define CSPI_2_3_CTRL_POSTDIV_OFFSET	8
#define CSPI_2_3_CTRL_PREDIV_OFFSET	12
#define CSPI_2_3_CTRL_CS(cs)		((cs) << 18)
#define CSPI_2_3_CTRL_BL_OFFSET	20

#define CSPI_2_3_CONFIG	0x0c
#define CSPI_2_3_CONFIG_SCLKPHA(cs)	(1 << ((cs) +  0))
#define CSPI_2_3_CONFIG_SCLKPOL(cs)	(1 << ((cs) +  4))
#define CSPI_2_3_CONFIG_SBBCTRL(cs)	(1 << ((cs) +  8))
#define CSPI_2_3_CONFIG_SSBPOL(cs)	(1 << ((cs) + 12))

#define CSPI_2_3_INT		0x10
#define CSPI_2_3_INT_TEEN		(1 <<  0)
#define CSPI_2_3_INT_RREN		(1 <<  3)

#define CSPI_2_3_STAT		0x18
#define CSPI_2_3_STAT_TF		(1 << 2)
#define CSPI_2_3_STAT_RR		(1 << 3)

#endif /* __SPI_IMX_SPI_H */
