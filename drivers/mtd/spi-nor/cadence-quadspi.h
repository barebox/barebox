/* SPDX-License-Identifier: GPL-2.0-only */

#include <linux/mtd/mtd.h>
#include <linux/mtd/spi-nor.h>
#include <clock.h>

#define CQSPI_FIFO_WIDTH			4

#define CQSPI_REG_SRAM_THRESHOLD_BYTES		50

/* Instruction type */
#define CQSPI_INST_TYPE_SINGLE			0
#define CQSPI_INST_TYPE_DUAL			1
#define CQSPI_INST_TYPE_QUAD			2

#define CQSPI_DUMMY_CLKS_PER_BYTE		8
#define CQSPI_DUMMY_BYTES_MAX			4

#define CQSPI_STIG_DATA_LEN_MAX			8

#define CQSPI_INDIRECTTRIGGER_ADDR_MASK		0xFFFFF

/* Register map */
#define CQSPI_REG_CONFIG			0x00
#define CQSPI_REG_CONFIG_ENABLE_MASK		BIT(0)
#define CQSPI_REG_CONFIG_DECODE_MASK		BIT(9)
#define CQSPI_REG_CONFIG_CHIPSELECT_LSB		10
#define CQSPI_REG_CONFIG_DMA_MASK		BIT(15)
#define CQSPI_REG_CONFIG_BAUD_LSB		19
#define CQSPI_REG_CONFIG_IDLE_LSB		31
#define CQSPI_REG_CONFIG_CHIPSELECT_MASK	0xF
#define CQSPI_REG_CONFIG_BAUD_MASK		0xF

#define CQSPI_REG_RD_INSTR			0x04
#define CQSPI_REG_RD_INSTR_OPCODE_LSB		0
#define CQSPI_REG_RD_INSTR_TYPE_INSTR_LSB	8
#define CQSPI_REG_RD_INSTR_TYPE_ADDR_LSB	12
#define CQSPI_REG_RD_INSTR_TYPE_DATA_LSB	16
#define CQSPI_REG_RD_INSTR_MODE_EN_LSB		20
#define CQSPI_REG_RD_INSTR_DUMMY_LSB		24
#define CQSPI_REG_RD_INSTR_TYPE_INSTR_MASK	0x3
#define CQSPI_REG_RD_INSTR_TYPE_ADDR_MASK	0x3
#define CQSPI_REG_RD_INSTR_TYPE_DATA_MASK	0x3
#define CQSPI_REG_RD_INSTR_DUMMY_MASK		0x1F

#define CQSPI_REG_WR_INSTR			0x08
#define CQSPI_REG_WR_INSTR_OPCODE_LSB		0
#define CQSPI_REG_WR_INSTR_TYPE_ADDR_LSB	12
#define CQSPI_REG_WR_INSTR_TYPE_DATA_LSB	16

#define CQSPI_REG_DELAY				0x0C
#define CQSPI_REG_DELAY_TSLCH_LSB		0
#define CQSPI_REG_DELAY_TCHSH_LSB		8
#define CQSPI_REG_DELAY_TSD2D_LSB		16
#define CQSPI_REG_DELAY_TSHSL_LSB		24
#define CQSPI_REG_DELAY_TSLCH_MASK		0xFF
#define CQSPI_REG_DELAY_TCHSH_MASK		0xFF
#define CQSPI_REG_DELAY_TSD2D_MASK		0xFF
#define CQSPI_REG_DELAY_TSHSL_MASK		0xFF

#define CQSPI_REG_READCAPTURE			0x10
#define CQSPI_REG_READCAPTURE_BYPASS_LSB	0
#define CQSPI_REG_READCAPTURE_DELAY_LSB		1
#define CQSPI_REG_READCAPTURE_DELAY_MASK	0xF

#define CQSPI_REG_SIZE				0x14
#define CQSPI_REG_SIZE_ADDRESS_LSB		0
#define CQSPI_REG_SIZE_PAGE_LSB			4
#define CQSPI_REG_SIZE_BLOCK_LSB		16
#define CQSPI_REG_SIZE_ADDRESS_MASK		0xF
#define CQSPI_REG_SIZE_PAGE_MASK		0xFFF
#define CQSPI_REG_SIZE_BLOCK_MASK		0x3F

#define CQSPI_REG_SRAMPARTITION			0x18
#define CQSPI_REG_INDIRECTTRIGGER		0x1C

#define CQSPI_REG_DMA				0x20
#define CQSPI_REG_DMA_SINGLE_LSB		0
#define CQSPI_REG_DMA_BURST_LSB			8
#define CQSPI_REG_DMA_SINGLE_MASK		0xFF
#define CQSPI_REG_DMA_BURST_MASK		0xFF

#define CQSPI_REG_REMAP				0x24
#define CQSPI_REG_MODE_BIT			0x28

#define CQSPI_REG_SRAMLEVEL			0x2C
#define CQSPI_REG_SRAMLEVEL_RD_LSB		0
#define CQSPI_REG_SRAMLEVEL_WR_LSB		16
#define CQSPI_REG_SRAMLEVEL_RD_MASK		0xFFFF
#define CQSPI_REG_SRAMLEVEL_WR_MASK		0xFFFF

#define CQSPI_REG_IRQSTATUS			0x40
#define CQSPI_REG_IRQMASK			0x44

#define CQSPI_REG_INDIRECTRD			0x60
#define CQSPI_REG_INDIRECTRD_START_MASK		BIT(0)
#define CQSPI_REG_INDIRECTRD_CANCEL_MASK	BIT(1)
#define CQSPI_REG_INDIRECTRD_DONE_MASK		BIT(5)

#define CQSPI_REG_INDIRECTRDWATERMARK		0x64
#define CQSPI_REG_INDIRECTRDSTARTADDR		0x68
#define CQSPI_REG_INDIRECTRDBYTES		0x6C

#define CQSPI_REG_CMDCTRL			0x90
#define CQSPI_REG_CMDCTRL_EXECUTE_MASK		BIT(0)
#define CQSPI_REG_CMDCTRL_INPROGRESS_MASK	BIT(1)
#define CQSPI_REG_CMDCTRL_WR_BYTES_LSB		12
#define CQSPI_REG_CMDCTRL_WR_EN_LSB		15
#define CQSPI_REG_CMDCTRL_ADD_BYTES_LSB		16
#define CQSPI_REG_CMDCTRL_ADDR_EN_LSB		19
#define CQSPI_REG_CMDCTRL_RD_BYTES_LSB		20
#define CQSPI_REG_CMDCTRL_RD_EN_LSB		23
#define CQSPI_REG_CMDCTRL_OPCODE_LSB		24
#define CQSPI_REG_CMDCTRL_WR_BYTES_MASK		0x7
#define CQSPI_REG_CMDCTRL_ADD_BYTES_MASK	0x3
#define CQSPI_REG_CMDCTRL_RD_BYTES_MASK		0x7

#define CQSPI_REG_INDIRECTWR			0x70
#define CQSPI_REG_INDIRECTWR_START_MASK		BIT(0)
#define CQSPI_REG_INDIRECTWR_CANCEL_MASK	BIT(1)
#define CQSPI_REG_INDIRECTWR_DONE_MASK		BIT(5)

#define CQSPI_REG_INDIRECTWRWATERMARK		0x74
#define CQSPI_REG_INDIRECTWRSTARTADDR		0x78
#define CQSPI_REG_INDIRECTWRBYTES		0x7C

#define CQSPI_REG_CMDADDRESS			0x94
#define CQSPI_REG_CMDREADDATALOWER		0xA0
#define CQSPI_REG_CMDREADDATAUPPER		0xA4
#define CQSPI_REG_CMDWRITEDATALOWER		0xA8
#define CQSPI_REG_CMDWRITEDATAUPPER		0xAC

/* Interrupt status bits */
#define CQSPI_REG_IRQ_MODE_ERR			BIT(0)
#define CQSPI_REG_IRQ_UNDERFLOW			BIT(1)
#define CQSPI_REG_IRQ_IND_COMP			BIT(2)
#define CQSPI_REG_IRQ_IND_RD_REJECT		BIT(3)
#define CQSPI_REG_IRQ_WR_PROTECTED_ERR		BIT(4)
#define CQSPI_REG_IRQ_ILLEGAL_AHB_ERR		BIT(5)
#define CQSPI_REG_IRQ_WATERMARK			BIT(6)
#define CQSPI_REG_IRQ_IND_RD_OVERFLOW		BIT(12)

#define CQSPI_IRQ_STATUS_ERR		(CQSPI_REG_IRQ_MODE_ERR		| \
					 CQSPI_REG_IRQ_IND_RD_REJECT	| \
					 CQSPI_REG_IRQ_WR_PROTECTED_ERR	| \
					 CQSPI_REG_IRQ_ILLEGAL_AHB_ERR)

#define CQSPI_IRQ_MASK_RD		(CQSPI_REG_IRQ_WATERMARK	| \
					 CQSPI_REG_IRQ_IND_RD_OVERFLOW	| \
					 CQSPI_REG_IRQ_IND_COMP)

#define CQSPI_IRQ_MASK_WR		(CQSPI_REG_IRQ_IND_COMP		| \
					 CQSPI_REG_IRQ_WATERMARK	| \
					 CQSPI_REG_IRQ_UNDERFLOW)

#define CQSPI_IRQ_STATUS_MASK		0x1FFFE

#define CQSPI_MAX_CHIPSELECT		16

struct cqspi_flash_pdata {
	struct mtd_info mtd;
	struct spi_nor nor;
	u32 clk_rate;
	unsigned int block_size;
	unsigned int read_delay;
	unsigned int tshsl_ns;
	unsigned int tsd2d_ns;
	unsigned int tchsh_ns;
	unsigned int tslch_ns;
	u8	     inst_width;
	u8	     addr_width;
	u8	     data_width;
};

struct cqspi_st {
	struct device	*dev;
	struct clk	*l4_mp_clk;
	struct clk	*qspi_clk;
	unsigned int	sclk;

	void __iomem	*iobase;
	void __iomem	*ahb_base;
	unsigned int	irq_mask;
	int		current_cs;
	unsigned int	master_ref_clk_hz;
	unsigned int	is_decoded_cs;
	unsigned int	fifo_depth;
	struct cqspi_flash_pdata f_pdata[CQSPI_MAX_CHIPSELECT];
	bool no_reconfig;
};

#define CQSPI_REG_IS_IDLE(base)						\
		((readl(base + CQSPI_REG_CONFIG) >>			\
			CQSPI_REG_CONFIG_IDLE_LSB) & 0x1)

#define CQSPI_GET_RD_SRAM_LEVEL(reg_base)				\
		(((readl(reg_base + CQSPI_REG_SRAMLEVEL)) >>		\
		CQSPI_REG_SRAMLEVEL_RD_LSB) & CQSPI_REG_SRAMLEVEL_RD_MASK)

#define CQSPI_TIMEOUT_MS			(800 * MSECOND)
#define CQSPI_READ_TIMEOUT_MS			(100 * MSECOND)

void cqspi_controller_init(struct cqspi_st *cqspi);

int cqspi_exec_flash_cmd(struct cqspi_st *cqspi, unsigned int reg);

unsigned int cqspi_calc_rdreg(struct spi_nor *nor, u8 opcode);

unsigned int cqspi_wait_idle(struct cqspi_st *cqspi);

int cqspi_read_reg(struct spi_nor *nor, u8 opcode, u8 *buf, int len);
int cqspi_write_reg(struct spi_nor *nor, u8 opcode, u8 *buf, int len);
int cqspi_read(struct spi_nor *nor, loff_t from, size_t len,
	       size_t *retlen, u_char *buf);

int cqspi_set_protocol(struct spi_nor *nor, const int read);
