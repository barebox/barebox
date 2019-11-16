#ifndef __MCI_SDHCI_H
#define __MCI_SDHCI_H

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
#define  SDHCI_BUFFER_READ_ENABLE		BIT(11)
#define  SDHCI_BUFFER_WRITE_ENABLE		BIT(10)
#define  SDHCI_DATA_LINE_ACTIVE			BIT(2)
#define  SDHCI_CMD_INHIBIT_DATA			BIT(1)
#define  SDHCI_CMD_INHIBIT_CMD			BIT(0)
#define SDHCI_PRESENT_STATE1					0x26
#define SDHCI_HOST_CONTROL__POWER_CONTROL__BLOCK_GAP_CONTROL	0x28
#define SDHCI_HOST_CONTROL					0x28
#define  SDHCI_CARD_DETECT_SIGNAL_SELECTION	BIT(7)
#define  SDHCI_CARD_DETECT_TEST_LEVEL		BIT(6)
#define  SDHCI_DATA_WIDTH_8BIT			BIT(5)
#define  SDHCI_HIGHSPEED_EN			BIT(2)
#define  SDHCI_DATA_WIDTH_4BIT			BIT(1)
#define SDHCI_POWER_CONTROL					0x29
#define  SDHCI_BUS_VOLTAGE_330			SDHCI_BUS_VOLTAGE(7)
#define  SDHCI_BUS_VOLTAGE(v)			((v) << 1)
#define  SDHCI_BUS_POWER_EN			BIT(0)
#define SDHCI_CLOCK_CONTROL__TIMEOUT_CONTROL__SOFTWARE_RESET	0x2c
#define SDHCI_CLOCK_CONTROL					0x2c
#define  SDHCI_FREQ_SEL(x)			(((x) & 0xff) << 8)
#define  SDHCI_SDCLOCK_EN			BIT(2)
#define  SDHCI_INTCLOCK_STABLE			BIT(1)
#define  SDHCI_INTCLOCK_EN			BIT(0)
#define SDHCI_TIMEOUT_CONTROL					0x2e
#define SDHCI_SOFTWARE_RESET					0x2f
#define  SDHCI_RESET_ALL			BIT(0)
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
#define  SDHCI_INT_DATA_AVAIL			BIT(5)
#define  SDHCI_INT_SPACE_AVAIL			BIT(4)
#define  SDHCI_INT_DMA				BIT(3)
#define  SDHCI_INT_XFER_COMPLETE		BIT(1)
#define  SDHCI_INT_CMD_COMPLETE			BIT(0)
#define SDHCI_INT_ERROR_STATUS					0x32
#define SDHCI_INT_ENABLE					0x34
#define SDHCI_SIGNAL_ENABLE					0x38
#define SDHCI_ACMD12_ERR__HOST_CONTROL2				0x3C
#define SDHCI_CAPABILITIES					0x40
#define SDHCI_CAPABILITIES_1					0x42
#define  SDHCI_HOSTCAP_VOLTAGE_180		BIT(10)
#define  SDHCI_HOSTCAP_VOLTAGE_300		BIT(9)
#define  SDHCI_HOSTCAP_VOLTAGE_330		BIT(8)
#define  SDHCI_HOSTCAP_HIGHSPEED		BIT(5)
#define  SDHCI_HOSTCAP_8BIT			BIT(2)

#define SDHCI_SPEC_200_MAX_CLK_DIVIDER	256
#define SDHCI_MMC_BOOT						0xC4

struct sdhci {
	u32 (*read32)(struct sdhci *host, int reg);
	u16 (*read16)(struct sdhci *host, int reg);
	u8 (*read8)(struct sdhci *host, int reg);
	void (*write32)(struct sdhci *host, int reg, u32 val);
	void (*write16)(struct sdhci *host, int reg, u16 val);
	void (*write8)(struct sdhci *host, int reg, u8 val);
};

static inline u32 sdhci_read32(struct sdhci *host, int reg)
{
	return host->read32(host, reg);
}

static inline u32 sdhci_read16(struct sdhci *host, int reg)
{
	return host->read16(host, reg);
}

static inline u32 sdhci_read8(struct sdhci *host, int reg)
{
	return host->read8(host, reg);
}

static inline void sdhci_write32(struct sdhci *host, int reg, u32 val)
{
	host->write32(host, reg, val);
}

static inline void sdhci_write16(struct sdhci *host, int reg, u32 val)
{
	host->write16(host, reg, val);
}

static inline void sdhci_write8(struct sdhci *host, int reg, u32 val)
{
	host->write8(host, reg, val);
}

void sdhci_read_response(struct sdhci *host, struct mci_cmd *cmd);
void sdhci_set_cmd_xfer_mode(struct sdhci *host, struct mci_cmd *cmd,
			     struct mci_data *data, bool dma, u32 *command,
			     u32 *xfer);
int sdhci_transfer_data(struct sdhci *sdhci, struct mci_data *data);

#endif /* __MCI_SDHCI_H */
