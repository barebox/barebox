/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __MCI_SDHCI_H
#define __MCI_SDHCI_H

#include <pbl.h>
#include <dma.h>
#include <linux/iopoll.h>
#include <linux/sizes.h>

#define SDHCI_DMA_ADDRESS					0x00
#define SDHCI_BLOCK_SIZE__BLOCK_COUNT				0x04
#define SDHCI_BLOCK_SIZE					0x04
#define  SDHCI_DMA_BOUNDARY_512K		SDHCI_DMA_BOUNDARY(7)
#define  SDHCI_DMA_BOUNDARY_256K		SDHCI_DMA_BOUNDARY(6)
#define  SDHCI_DMA_BOUNDARY_128K		SDHCI_DMA_BOUNDARY(5)
#define  SDHCI_DMA_BOUNDARY_64K			SDHCI_DMA_BOUNDARY(4)
#define  SDHCI_DMA_BOUNDARY_32K			SDHCI_DMA_BOUNDARY(3)
#define  SDHCI_DMA_BOUNDARY_16K			SDHCI_DMA_BOUNDARY(2)
#define  SDHCI_DMA_BOUNDARY_8K			SDHCI_DMA_BOUNDARY(1)
#define  SDHCI_DMA_BOUNDARY_4K			SDHCI_DMA_BOUNDARY(0)
#define  SDHCI_DMA_BOUNDARY(x)			(((x) & 0x7) << 12)
#define  SDHCI_DEFAULT_BOUNDARY_SIZE		SZ_512K
#define  SDHCI_DEFAULT_BOUNDARY_ARG		SDHCI_DMA_BOUNDARY_512K
#define  SDHCI_TRANSFER_BLOCK_SIZE(x)		((x) & 0xfff)
#define SDHCI_BLOCK_COUNT					0x06
#define SDHCI_ARGUMENT						0x08
#define SDHCI_TRANSFER_MODE__COMMAND				0x0c
#define SDHCI_TRANSFER_MODE					0x0c
#define  SDHCI_MULTIPLE_BLOCKS			BIT(5)
#define  SDHCI_DATA_TO_HOST			BIT(4)
#define  SDHCI_BLOCK_COUNT_EN			BIT(1)
#define  SDHCI_DMA_EN				BIT(0)
#define SDHCI_COMMAND						0x0e
#define  SDHCI_CMD_INDEX(c)			(((c) & 0x3f) << 8)
#define  SDHCI_COMMAND_CMDTYP_SUSPEND		(1 << 6)
#define  SDHCI_COMMAND_CMDTYP_RESUME		(2 << 6)
#define  SDHCI_COMMAND_CMDTYP_ABORT		(3 << 6)
#define  SDHCI_DATA_PRESENT			BIT(5)
#define  SDHCI_CMD_INDEX_CHECK_EN		BIT(4)
#define  SDHCI_CMD_CRC_CHECK_EN			BIT(3)
#define  SDHCI_RESP_TYPE_48_BUSY		3
#define  SDHCI_RESP_TYPE_48			2
#define  SDHCI_RESP_TYPE_136			1
#define  SDHCI_RESP_NONE			0
#define SDHCI_RESPONSE_0					0x10
#define SDHCI_RESPONSE_1					0x14
#define SDHCI_RESPONSE_2					0x18
#define SDHCI_RESPONSE_3					0x1c
#define SDHCI_BUFFER						0x20
#define SDHCI_PRESENT_STATE					0x24
#define  SDHCI_WRITE_PROTECT			BIT(19)
#define  SDHCI_CARD_DETECT			BIT(18)
#define  SDHCI_CARD_PRESENT			BIT(16)
#define  SDHCI_BUFFER_READ_ENABLE		BIT(11)
#define  SDHCI_BUFFER_WRITE_ENABLE		BIT(10)
#define  SDHCI_DATA_LINE_ACTIVE			BIT(2)
#define  SDHCI_CMD_INHIBIT_DATA			BIT(1)
#define  SDHCI_CMD_INHIBIT_CMD			BIT(0)
#define SDHCI_PRESENT_STATE1					0x26
#define SDHCI_HOST_CONTROL__POWER_CONTROL__BLOCK_GAP_CONTROL	0x28
#define SDHCI_HOST_CONTROL					0x28
#define  SDHCI_CTRL_LED         BIT(0)
#define  SDHCI_CTRL_4BITBUS     BIT(1)
#define  SDHCI_CTRL_HISPD       BIT(2)
#define  SDHCI_CTRL_DMA_MASK    0x18
#define   SDHCI_CTRL_SDMA       0x00
#define   SDHCI_CTRL_ADMA1      0x08
#define   SDHCI_CTRL_ADMA32     0x10
#define   SDHCI_CTRL_ADMA64     0x18
#define   SDHCI_CTRL_ADMA3      0x18
#define  SDHCI_CTRL_8BITBUS     BIT(5)
#define  SDHCI_CTRL_CDTEST_INS  BIT(6)
#define  SDHCI_CTRL_CDTEST_EN   BIT(7)
#define SDHCI_POWER_CONTROL					0x29
#define  SDHCI_POWER_ON				0x01
#define  SDHCI_POWER_180			0x0A
#define  SDHCI_POWER_300			0x0C
#define  SDHCI_POWER_330			0x0E
#define  SDHCI_BUS_VOLTAGE_330			SDHCI_BUS_VOLTAGE(7)
#define  SDHCI_BUS_VOLTAGE(v)			((v) << 1)
#define  SDHCI_BUS_POWER_EN			BIT(0)
#define SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET	0x2c
#define SDHCI_CLOCK_CONTROL					0x2C
#define  SDHCI_DIVIDER_SHIFT			8
#define  SDHCI_DIVIDER_HI_SHIFT			6
#define  SDHCI_DIV_MASK				0xFF
#define  SDHCI_DIV_HI_MASK			0x300
#define  SDHCI_DIV_MASK_LEN			8
#define  SDHCI_FREQ_SEL(x)                     (((x) & 0xff) << 8)
#define  SDHCI_DIV_HI_MASK			0x300
#define  SDHCI_PROG_CLOCK_MODE			BIT(5)
#define  SDHCI_CLOCK_CARD_EN			BIT(2)
#define  SDHCI_CLOCK_PLL_EN			BIT(3)
#define  SDHCI_CLOCK_INT_STABLE			BIT(1)
#define  SDHCI_CLOCK_INT_EN     		BIT(0)
#define SDHCI_TIMEOUT_CONTROL					0x2e
#define SDHCI_SOFTWARE_RESET					0x2f
#define  SDHCI_RESET_ALL			BIT(0)
#define  SDHCI_RESET_CMD			BIT(1)
#define  SDHCI_RESET_DATA			BIT(2)
#define SDHCI_INT_STATUS					0x30
#define SDHCI_INT_NORMAL_STATUS					0x30
#define  SDHCI_INT_DATA_END_BIT			BIT(22)
#define  SDHCI_INT_DATA_CRC			BIT(21)
#define  SDHCI_INT_DATA_TIMEOUT			BIT(20)
#define  SDHCI_INT_INDEX			BIT(19)
#define  SDHCI_INT_END_BIT			BIT(18)
#define  SDHCI_INT_CRC				BIT(17)
#define  SDHCI_INT_TIMEOUT			BIT(16)
#define  SDHCI_INT_ERROR			BIT(15)
#define  SDHCI_INT_CARD_INT			BIT(8)
#define  SDHCI_INT_CARD_INSERT			BIT(6)
#define  SDHCI_INT_DATA_AVAIL			BIT(5)
#define  SDHCI_INT_SPACE_AVAIL			BIT(4)
#define  SDHCI_INT_DMA				BIT(3)
#define  SDHCI_INT_XFER_COMPLETE		BIT(1)
#define  SDHCI_INT_CMD_COMPLETE			BIT(0)
#define SDHCI_INT_ERROR_STATUS					0x32
#define SDHCI_INT_ENABLE					0x34
#define SDHCI_INT_ERROR_ENABLE					0x36
#define SDHCI_SIGNAL_ENABLE					0x38
#define SDHCI_ACMD12_ERR__HOST_CONTROL2				0x3C
#define SDHCI_HOST_CONTROL2					0x3E
#define  SDHCI_CTRL_64BIT_ADDR			BIT(13)
#define  SDHCI_CTRL_V4_MODE			BIT(12)
#define SDHCI_CAPABILITIES					0x40
#define  SDHCI_TIMEOUT_CLK_MASK			GENMASK(5, 0)
#define  SDHCI_TIMEOUT_CLK_UNIT			0x00000080
#define  SDHCI_CLOCK_BASE_MASK			GENMASK(13, 8)
#define  SDHCI_CLOCK_V3_BASE_MASK		GENMASK(15, 8)
#define  SDHCI_MAX_BLOCK_MASK			0x00030000
#define  SDHCI_MAX_BLOCK_SHIFT			16
#define  SDHCI_CAN_DO_8BIT			0x00040000
#define  SDHCI_CAN_DO_ADMA2			0x00080000
#define  SDHCI_CAN_DO_ADMA1			0x00100000
#define  SDHCI_CAN_DO_HISPD			0x00200000
#define  SDHCI_CAN_DO_SDMA			0x00400000
#define  SDHCI_CAN_DO_SUSPEND			0x00800000
#define  SDHCI_CAN_VDD_330			0x01000000
#define  SDHCI_CAN_VDD_300			0x02000000
#define  SDHCI_CAN_VDD_180			0x04000000
#define  SDHCI_CAN_64BIT_V4			0x08000000
#define  SDHCI_CAN_64BIT			0x10000000

#define SDHCI_CAPABILITIES_1			0x44
#define  SDHCI_SUPPORT_SDR50			0x00000001
#define  SDHCI_SUPPORT_SDR104			0x00000002
#define  SDHCI_SUPPORT_DDR50			0x00000004
#define  SDHCI_DRIVER_TYPE_A			0x00000010
#define  SDHCI_DRIVER_TYPE_C			0x00000020
#define  SDHCI_DRIVER_TYPE_D			0x00000040
#define  SDHCI_RETUNING_TIMER_COUNT_MASK	GENMASK(11, 8)
#define  SDHCI_USE_SDR50_TUNING			0x00002000
#define  SDHCI_RETUNING_MODE_MASK		GENMASK(15, 14)
#define  SDHCI_CLOCK_MUL_MASK			GENMASK(23, 16)
#define  SDHCI_CAN_DO_ADMA3			0x08000000
#define  SDHCI_SUPPORT_HS400			0x80000000 /* Non-standard */

#define SDHCI_PRESET_FOR_SDR12	0x66
#define SDHCI_PRESET_FOR_SDR25	0x68
#define SDHCI_PRESET_FOR_SDR50	0x6A
#define SDHCI_PRESET_FOR_SDR104	0x6C
#define SDHCI_PRESET_FOR_DDR50	0x6E
#define SDHCI_PRESET_FOR_HS400	0x74 /* Non-standard */
#define SDHCI_PRESET_CLKGEN_SEL		BIT(10)
#define SDHCI_PRESET_SDCLK_FREQ_MASK	GENMASK(9, 0)

#define SDHCI_HOST_VERSION	0xFE
#define  SDHCI_VENDOR_VER_MASK	0xFF00
#define  SDHCI_VENDOR_VER_SHIFT	8
#define  SDHCI_SPEC_VER_MASK	0x00FF
#define  SDHCI_SPEC_VER_SHIFT	0
#define   SDHCI_SPEC_100	0
#define   SDHCI_SPEC_200	1
#define   SDHCI_SPEC_300	2
#define   SDHCI_SPEC_400	3
#define   SDHCI_SPEC_410	4
#define   SDHCI_SPEC_420	5

#define  SDHCI_CLOCK_MUL_SHIFT	16

#define SDHCI_ADMA_ADDRESS					0x58
#define SDHCI_ADMA_ADDRESS_HI					0x5c

#define SDHCI_MMC_BOOT						0xC4

#define SDHCI_MAX_DIV_SPEC_200	256
#define SDHCI_MAX_DIV_SPEC_300	2046

struct sdhci {
	u32 (*read32)(struct sdhci *host, int reg);
	u16 (*read16)(struct sdhci *host, int reg);
	u8 (*read8)(struct sdhci *host, int reg);
	void (*write32)(struct sdhci *host, int reg, u32 val);
	void (*write16)(struct sdhci *host, int reg, u16 val);
	void (*write8)(struct sdhci *host, int reg, u8 val);

	void __iomem *base;

	int max_clk; /* Max possible freq (Hz) */
	int clk_mul; /* Clock Muliplier value */

	int flags;		/* Host attributes */
#define SDHCI_USE_SDMA		(1<<0)	/* Host is SDMA capable */
#define SDHCI_USE_ADMA		(1<<1)	/* Host is ADMA capable */
#define SDHCI_REQ_USE_DMA	(1<<2)	/* Use DMA for this req. */
#define SDHCI_DEVICE_DEAD	(1<<3)	/* Device unresponsive */
#define SDHCI_SDR50_NEEDS_TUNING (1<<4)	/* SDR50 needs tuning */
#define SDHCI_AUTO_CMD12	(1<<6)	/* Auto CMD12 support */
#define SDHCI_AUTO_CMD23	(1<<7)	/* Auto CMD23 support */
#define SDHCI_PV_ENABLED	(1<<8)	/* Preset value enabled */
#define SDHCI_USE_64_BIT_DMA	(1<<12)	/* Use 64-bit DMA */
#define SDHCI_HS400_TUNING	(1<<13)	/* Tuning for HS400 */
#define SDHCI_SIGNALING_330	(1<<14)	/* Host is capable of 3.3V signaling */
#define SDHCI_SIGNALING_180	(1<<15)	/* Host is capable of 1.8V signaling */
#define SDHCI_SIGNALING_120	(1<<16)	/* Host is capable of 1.2V signaling */

	unsigned int version; /* SDHCI spec. version */

	enum mci_timing timing;
	bool preset_enabled; /* Preset is enabled */
	bool v4_mode;		/* Host Version 4 Enable */

	unsigned int quirks;
#define SDHCI_QUIRK_MISSING_CAPS		BIT(27)
	unsigned int quirks2;
#define SDHCI_QUIRK2_CLOCK_DIV_ZERO_BROKEN	BIT(15)
	u32 caps;	/* CAPABILITY_0 */
	u32 caps1;	/* CAPABILITY_1 */
	bool read_caps;	/* Capability flags have been read */
	u32 sdma_boundary;

	struct mci_host	*mci;
};

static inline u32 sdhci_read32(struct sdhci *host, int reg)
{
	if (host->read32)
		return host->read32(host, reg);
	else
		return readl(host->base + reg);
}

static inline u32 sdhci_read16(struct sdhci *host, int reg)
{
	if (host->read16)
		return host->read16(host, reg);
	else
		return readw(host->base + reg);
}

static inline u32 sdhci_read8(struct sdhci *host, int reg)
{
	if (host->read8)
		return host->read8(host, reg);
	else
		return readb(host->base + reg);
}

static inline void sdhci_write32(struct sdhci *host, int reg, u32 val)
{
	if (host->write32)
		host->write32(host, reg, val);
	else
		writel(val, host->base + reg);
}

static inline void sdhci_write16(struct sdhci *host, int reg, u32 val)
{
	if (host->write16)
		host->write16(host, reg, val);
	else
		writew(val, host->base + reg);
}

static inline void sdhci_write8(struct sdhci *host, int reg, u32 val)
{
	if (host->write8)
		host->write8(host, reg, val);
	else
		writeb(val, host->base + reg);
}

#define SDHCI_NO_DMA DMA_ERROR_CODE
int sdhci_wait_idle(struct sdhci *host, struct mci_cmd *cmd);
int sdhci_wait_for_done(struct sdhci *host, u32 mask);
void sdhci_read_response(struct sdhci *host, struct mci_cmd *cmd);
void sdhci_set_cmd_xfer_mode(struct sdhci *host, struct mci_cmd *cmd,
			     struct mci_data *data, bool dma, u32 *command,
			     u32 *xfer);
void sdhci_setup_data_pio(struct sdhci *sdhci, struct mci_data *data);
void sdhci_setup_data_dma(struct sdhci *sdhci, struct mci_data *data, dma_addr_t *dma);
int sdhci_transfer_data(struct sdhci *sdhci, struct mci_data *data, dma_addr_t dma);
int sdhci_transfer_data_pio(struct sdhci *sdhci, struct mci_data *data);
int sdhci_transfer_data_dma(struct sdhci *sdhci, struct mci_data *data,
			    dma_addr_t dma);
int sdhci_reset(struct sdhci *sdhci, u8 mask);
u16 sdhci_calc_clk(struct sdhci *host, unsigned int clock,
		   unsigned int *actual_clock, unsigned int input_clock);
void sdhci_set_clock(struct sdhci *host, unsigned int clock, unsigned int input_clock);
void sdhci_enable_clk(struct sdhci *host, u16 clk);
void sdhci_enable_v4_mode(struct sdhci *host);
int sdhci_setup_host(struct sdhci *host);
void __sdhci_read_caps(struct sdhci *host, const u16 *ver,
			const u32 *caps, const u32 *caps1);
static inline void sdhci_read_caps(struct sdhci *host)
{
	__sdhci_read_caps(host, NULL, NULL, NULL);
}
void sdhci_set_bus_width(struct sdhci *host, int width);

#define sdhci_read8_poll_timeout(sdhci, reg, val, cond, timeout_us) \
	read_poll_timeout(sdhci_read8, val, cond, timeout_us, sdhci, reg)

#define sdhci_read16_poll_timeout(sdhci, reg, val, cond, timeout_us) \
	read_poll_timeout(sdhci_read16, val, cond, timeout_us, sdhci, reg)

#define sdhci_read32_poll_timeout(sdhci, reg, val, cond, timeout_us) \
	read_poll_timeout(sdhci_read32, val, cond, timeout_us, sdhci, reg)

#endif /* __MCI_SDHCI_H */
