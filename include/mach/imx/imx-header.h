/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __IMX_HEADER_H__
#define __IMX_HEADER_H__

#include <linux/types.h>

#define HEADER_LEN 0x1000	/* length of the blank area + IVT + DCD */
#define CSF_LEN 0x2000		/* length of the CSF (needed for HAB) */
#define FLEXSPI_HEADER_LEN	HEADER_LEN

#define DEK_BLOB_HEADER 8	/* length of DEK blob header */
#define DEK_BLOB_KEY 32		/* length of DEK blob AES-256 key */
#define DEK_BLOB_MAC 16		/* length of DEK blob MAC */

/* DEK blob length excluding DEK itself */
#define DEK_BLOB_OVERHEAD (DEK_BLOB_HEADER + DEK_BLOB_KEY + DEK_BLOB_MAC)

/*
 * ============================================================================
 * i.MX flash header v1 handling. Found on i.MX35 and i.MX51
 * ============================================================================
 */
#define DCD_BARKER       0xb17219e9

struct imx_flash_header {
	uint32_t app_code_jump_vector;
	uint32_t app_code_barker;
	uint32_t app_code_csf;
	uint32_t dcd_ptr_ptr;
	uint32_t super_root_key;
	uint32_t dcd;
	uint32_t app_dest;
	uint32_t dcd_barker;
	uint32_t dcd_block_len;
} __attribute__((packed));

struct imx_boot_data {
	uint32_t start;
	uint32_t size;
	uint32_t plugin;
} __attribute__((packed));

struct imx_dcd_rec_v1 {
	uint32_t type;
	uint32_t addr;
	uint32_t val;
} __attribute__((packed));

#define TAG_IVT_HEADER	0xd1
#define IVT_VERSION	0x40
#define TAG_DCD_HEADER	0xd2
#define DCD_VERSION	0x40
#define TAG_UNLOCK	0xb2
#define TAG_NOP		0xc0
#define TAG_WRITE	0xcc
#define TAG_CHECK	0xcf
#define PARAMETER_FLAG_MASK	(1 << 3)
#define PARAMETER_FLAG_SET	(1 << 4)

#define PLUGIN_HDMI_IMAGE	0x0002

/*
 * As per Table 6-22 "eMMC/SD BOOT layout", in Normal Boot layout HDMI
 * firmware image starts at LBA# 64 and ends at LBA# 271
 */
#define PLUGIN_HDMI_SIZE	((271 - 64 + 1) * 512)

struct imx_ivt_header {
	uint8_t tag;
	uint16_t length;
	uint8_t version;
} __attribute__((packed));

struct imx_flash_header_v2 {
	struct imx_ivt_header header;

	uint32_t entry;
	uint32_t reserved1;
	uint32_t dcd_ptr;
	uint32_t boot_data_ptr;
	uint32_t self;
	uint32_t csf;
	uint32_t reserved2;

	struct imx_boot_data boot_data;
	struct imx_ivt_header dcd_header;
} __attribute__((packed));

static inline bool is_imx_flash_header_v2(const void *blob)
{
	const struct imx_flash_header_v2 *hdr = blob;

	return  hdr->header.tag == TAG_IVT_HEADER &&
		hdr->header.version >= IVT_VERSION;
}

struct config_data {
	uint32_t image_load_addr;
	uint32_t image_ivt_offset;
	uint32_t image_flexspi_ivt_offset;
	uint32_t image_flexspi_fcfb_offset;
	uint32_t image_size;
	uint32_t max_load_size;
	uint32_t load_size;
	uint32_t pbl_code_size;
	char *outfile;
	char *srkfile;
	int header_version;
	off_t header_gap;
	uint32_t first_opcode;
	int cpu_type;
	int (*check)(const struct config_data *data, uint32_t cmd,
		     uint32_t addr, uint32_t mask);
	int (*write_mem)(const struct config_data *data, uint32_t addr,
			 uint32_t val, int width, int set_bits, int clear_bits);
	int (*nop)(const struct config_data *data);
	int csf_space;
	char *csf;
	int sign_image;
	char *signed_hdmi_firmware_file;
	int encrypt_image;
	size_t dek_size;
};

#define MAX_RECORDS_DCD_V2 1024
struct imx_dcd_v2_write_rec {
	uint32_t addr;
	uint32_t val;
} __attribute__((packed));

struct imx_dcd_v2_write {
	uint8_t tag;
	uint16_t length;
	uint8_t param;
	struct imx_dcd_v2_write_rec data[MAX_RECORDS_DCD_V2];
} __attribute__((packed));

struct imx_dcd_v2_check {
	uint8_t tag;
	uint16_t length;
	uint8_t param;
	uint32_t addr;
	uint32_t mask;
	uint32_t count;
} __attribute__((packed));

enum imx_dcd_v2_check_cond {
	until_all_bits_clear = 0, /* until ((*address & mask) == 0) { ...} */
	until_any_bit_clear = 1, /* until ((*address & mask) != mask) { ...} */
	until_all_bits_set = 2, /* until ((*address & mask) == mask) { ...} */
	until_any_bit_set = 3, /* until ((*address & mask) != 0) { ...} */
} __attribute__((packed));

/* FlexSPI conifguration block FCFB */
#define FCFB_HEAD_TAG			0x46434642	/* "FCFB" */
#define FCFB_VERSION			0x56010000	/* V<major><minor><bugfix> = V100 */
#define FCFB_SAMLPE_CLK_SRC_INTERNAL	0
#define FCFB_DEVTYPE_SERIAL_NOR		1
#define FCFB_SFLASH_PADS_SINGLE		1
#define FCFB_SFLASH_PADS_DUAL		2
#define FCFB_SFLASH_PADS_QUAD		4
#define FCFB_SFLASH_PADS_OCTAL		8
#define FCFB_SERIAL_CLK_FREQ_30MHZ	1
#define FCFB_SERIAL_CLK_FREQ_50MHZ	2
#define FCFB_SERIAL_CLK_FREQ_60MHZ	3
#define FCFB_SERIAL_CLK_FREQ_75MHZ	4
#define FCFB_SERIAL_CLK_FREQ_80MHZ	5
#define FCFB_SERIAL_CLK_FREQ_100MHZ	6
#define FCFB_SERIAL_CLK_FREQ_133MHZ	7
#define FCFB_SERIAL_CLK_FREQ_166MHZ	8

/* Instruction set for the LUT register. */
#define LUT_STOP			0x00
#define LUT_CMD				0x01
#define LUT_ADDR			0x02
#define LUT_CADDR_SDR			0x03
#define LUT_MODE			0x04
#define LUT_MODE2			0x05
#define LUT_MODE4			0x06
#define LUT_MODE8			0x07
#define LUT_NXP_WRITE			0x08
#define LUT_NXP_READ			0x09
#define LUT_LEARN_SDR			0x0A
#define LUT_DATSZ_SDR			0x0B
#define LUT_DUMMY			0x0C
#define LUT_DUMMY_RWDS_SDR		0x0D
#define LUT_JMP_ON_CS			0x1F
#define LUT_CMD_DDR			0x21
#define LUT_ADDR_DDR			0x22
#define LUT_CADDR_DDR			0x23
#define LUT_MODE_DDR			0x24
#define LUT_MODE2_DDR			0x25
#define LUT_MODE4_DDR			0x26
#define LUT_MODE8_DDR			0x27
#define LUT_WRITE_DDR			0x28
#define LUT_READ_DDR			0x29
#define LUT_LEARN_DDR			0x2A
#define LUT_DATSZ_DDR			0x2B
#define LUT_DUMMY_DDR			0x2C
#define LUT_DUMMY_RWDS_DDR		0x2D

/*
 * Macro for constructing the LUT entries with the following
 * register layout:
 *
 *  -----------------------
 *  | INSTR | PAD | OPRND |
 *  -----------------------
 */
#define PAD_SHIFT		8
#define INSTR_SHIFT		10
#define OPRND_SHIFT		16

/* Macros for constructing the LUT register. */
#define LUT_DEF(ins, pad, opr)	\
	(((ins) << INSTR_SHIFT) | ((pad) << PAD_SHIFT) | (opr))

struct imx_fcfb_common {
	uint32_t tag;
	uint32_t version;
	uint32_t reserved1;
	uint8_t read_sample;
	uint8_t datahold;
	uint8_t datasetup;
	uint8_t coladdrwidth;
	uint8_t devcfgenable;
	uint8_t reserved2[3];
	uint32_t devmodeseq;
	uint32_t devmodearg;
	uint8_t cmd_enable;
	uint8_t reserved3[3];
	uint32_t cmd_seq[4];
	uint32_t cmd_arg[4];
	uint32_t controllermisc;
	uint8_t dev_type;
	uint8_t sflash_pad;
	uint8_t serial_clk;
	uint8_t lut_custom;
	uint32_t reserved4[2];
	uint32_t sflashA1;
	uint32_t sflashA2;
	uint32_t sflashB1;
	uint32_t sflashB2;
	uint32_t cspadover;
	uint32_t sclkpadover;
	uint32_t datapadover;
	uint32_t dqspadover;
	uint32_t timeout_ms;
	uint32_t commandInt_ns;
	uint32_t datavalid_ns;
	uint16_t busyoffset;
	uint16_t busybitpolarity;
	struct {
		struct {
			uint16_t instr[8];
		} seq[16];
	} lut;
	uint16_t lut_custom_seq[24];
	uint8_t reserved5[16];
} __attribute__((packed));

struct imx_fcfb_nor {
	struct imx_fcfb_common	memcfg;
	uint32_t		page_sz;
	uint32_t		sector_sz;
	uint32_t		ipcmd_serial_clk;
	uint8_t			reserved[52];
} __attribute__((packed));

#endif
