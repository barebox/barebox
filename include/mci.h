/*
 * (C) Copyright 2010 Juergen Beisert, Pengutronix
 *
 * This code is partially based on u-boot code:
 *
 * Copyright 2008, Freescale Semiconductor, Inc
 * Andy Fleming
 *
 * Based (loosely) on the Linux code
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _MCI_H_
#define _MCI_H_

#include <linux/list.h>
#include <block.h>
#include <fs.h>
#include <regulator.h>

/* These codes should be sorted numerically in order of newness.  If the last
 * nybble is a zero, it will not be printed.  So 0x120 -> "1.2" and 0x123 ->
 * "1.23", 0x1a0 -> "1.10", 0x1b0 and 0x111 -> "1.11" but sort differently.
 * It's not possible to create "1.20".  */

/* Firmware revisions for SD cards */
#define SD_VERSION_SD		0x20000
#define SD_VERSION_1_0		(SD_VERSION_SD | 0x100)
#define SD_VERSION_1_10		(SD_VERSION_SD | 0x1a0)
#define SD_VERSION_2		(SD_VERSION_SD | 0x200)

/* Firmware revisions for MMC cards */
#define MMC_VERSION_MMC		0x10000
#define MMC_VERSION_UNKNOWN	(MMC_VERSION_MMC)
#define MMC_VERSION_1_2		(MMC_VERSION_MMC | 0x120)
#define MMC_VERSION_1_4		(MMC_VERSION_MMC | 0x140)
#define MMC_VERSION_2_2		(MMC_VERSION_MMC | 0x220)
#define MMC_VERSION_3		(MMC_VERSION_MMC | 0x300)
#define MMC_VERSION_4		(MMC_VERSION_MMC | 0x400)
#define MMC_VERSION_4_1		(MMC_VERSION_MMC | 0x410)
#define MMC_VERSION_4_2		(MMC_VERSION_MMC | 0x420)
#define MMC_VERSION_4_3		(MMC_VERSION_MMC | 0x430)
#define MMC_VERSION_4_41	(MMC_VERSION_MMC | 0x441)
#define MMC_VERSION_4_5		(MMC_VERSION_MMC | 0x450)
#define MMC_VERSION_5_0		(MMC_VERSION_MMC | 0x500)
#define MMC_VERSION_5_1		(MMC_VERSION_MMC | 0x510)

#define MMC_CAP_SPI			(1 << 0)
#define MMC_CAP_4_BIT_DATA		(1 << 1)
#define MMC_CAP_8_BIT_DATA		(1 << 2)
#define MMC_CAP_SD_HIGHSPEED		(1 << 3)
#define MMC_CAP_MMC_HIGHSPEED		(1 << 4)
#define MMC_CAP_MMC_HIGHSPEED_52MHZ	(1 << 5)
/* Mask of all caps for bus width */
#define MMC_CAP_BIT_DATA_MASK		(MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA)

#define SD_DATA_4BIT		0x00040000

#define IS_SD(x) (x->version & SD_VERSION_SD)

#define MMC_DATA_READ		1
#define MMC_DATA_WRITE		2

/* command list */
#define MMC_CMD_GO_IDLE_STATE		0
#define MMC_CMD_SEND_OP_COND		1
#define MMC_CMD_ALL_SEND_CID		2
#define MMC_CMD_SET_RELATIVE_ADDR	3
#define MMC_CMD_SET_DSR			4
#define MMC_CMD_SWITCH			6
#define MMC_CMD_SELECT_CARD		7
#define MMC_CMD_SEND_EXT_CSD		8
#define MMC_CMD_SEND_CSD		9
#define MMC_CMD_SEND_CID		10
#define MMC_CMD_STOP_TRANSMISSION	12
#define MMC_CMD_SEND_STATUS		13
#define MMC_CMD_SET_BLOCKLEN		16
#define MMC_CMD_READ_SINGLE_BLOCK	17
#define MMC_CMD_READ_MULTIPLE_BLOCK	18
#define MMC_CMD_WRITE_SINGLE_BLOCK	24
#define MMC_CMD_WRITE_MULTIPLE_BLOCK	25
#define MMC_CMD_APP_CMD			55
#define MMC_CMD_SPI_READ_OCR		58
#define MMC_CMD_SPI_CRC_ON_OFF		59

#define SD_CMD_SEND_RELATIVE_ADDR	3
#define SD_CMD_SWITCH_FUNC		6
#define SD_CMD_SEND_IF_COND		8

#define SD_CMD_APP_SET_BUS_WIDTH	6
#define SD_CMD_APP_SEND_OP_COND		41
#define SD_CMD_APP_SEND_SCR		51

/* SCR definitions in different words */
#define SD_HIGHSPEED_BUSY	0x00020000
#define SD_HIGHSPEED_SUPPORTED	0x00020000

#define MMC_HS_TIMING		0x00000100

#define OCR_BUSY		0x80000000
/** card's response in its OCR if it is a high capacity card */
#define OCR_HCS			0x40000000

#define MMC_VDD_165_195		0x00000080	/* VDD voltage 1.65 - 1.95 */
#define MMC_VDD_20_21		0x00000100	/* VDD voltage 2.0 ~ 2.1 */
#define MMC_VDD_21_22		0x00000200	/* VDD voltage 2.1 ~ 2.2 */
#define MMC_VDD_22_23		0x00000400	/* VDD voltage 2.2 ~ 2.3 */
#define MMC_VDD_23_24		0x00000800	/* VDD voltage 2.3 ~ 2.4 */
#define MMC_VDD_24_25		0x00001000	/* VDD voltage 2.4 ~ 2.5 */
#define MMC_VDD_25_26		0x00002000	/* VDD voltage 2.5 ~ 2.6 */
#define MMC_VDD_26_27		0x00004000	/* VDD voltage 2.6 ~ 2.7 */
#define MMC_VDD_27_28		0x00008000	/* VDD voltage 2.7 ~ 2.8 */
#define MMC_VDD_28_29		0x00010000	/* VDD voltage 2.8 ~ 2.9 */
#define MMC_VDD_29_30		0x00020000	/* VDD voltage 2.9 ~ 3.0 */
#define MMC_VDD_30_31		0x00040000	/* VDD voltage 3.0 ~ 3.1 */
#define MMC_VDD_31_32		0x00080000	/* VDD voltage 3.1 ~ 3.2 */
#define MMC_VDD_32_33		0x00100000	/* VDD voltage 3.2 ~ 3.3 */
#define MMC_VDD_33_34		0x00200000	/* VDD voltage 3.3 ~ 3.4 */
#define MMC_VDD_34_35		0x00400000	/* VDD voltage 3.4 ~ 3.5 */
#define MMC_VDD_35_36		0x00800000	/* VDD voltage 3.5 ~ 3.6 */

#define MMC_SWITCH_MODE_CMD_SET		0x00 /** Change the command set */
 /** Set bits in EXT_CSD byte addressed by index which are 1 in value field */
#define MMC_SWITCH_MODE_SET_BITS	0x01
 /** Clear bits in EXT_CSD byte addressed by index, which are 1 in value field */
#define MMC_SWITCH_MODE_CLEAR_BITS	0x02
 /** Set target byte to value */
#define MMC_SWITCH_MODE_WRITE_BYTE	0x03

#define SD_SWITCH_CHECK		0
#define SD_SWITCH_SWITCH	1

/*
 * EXT_CSD fields
 */

#define EXT_CSD_CMDQ_MODE_EN			15	/* RO */
#define EXT_CSD_SECURE_REMOVAL_TYPE		16	/* R/W */
#define EXT_CSD_PRODUCT_ST8_AWARENSS_ENABLEMENT	17	/* R/W */
#define EXT_CSD_MAX_PRE_LOADING_DATA_SIZE	18	/* RO, 4 bytes */
#define EXT_CSD_PRE_LOADING_DATA_SIZE		22	/* R/W, 4 bytes */
#define EXT_CSD_FFU_STATUS			26	/* RO */
#define EXT_CSD_MODE_OPERATION_CODES		29	/* W */
#define EXT_CSD_MODE_CONFIG			30	/* R/W */
#define EXT_CSD_BARRIER_CTRL			31	/* R/W */
#define EXT_CSD_FLUSH_CACHE			32      /* W */
#define EXT_CSD_CACHE_CTRL			33      /* R/W */
#define EXT_CSD_POWER_OFF_NOTIFICATION		34	/* R/W */
#define EXT_CSD_PACKED_FAILURE_INDEX		35	/* RO */
#define EXT_CSD_PACKED_COMMAND_STATUS		36	/* RO */
#define EXT_CSD_CONTEXT_CONF(index)		(37 + (index) - 1) /* R/W, 15 bytes */
#define EXT_CSD_EXT_PARTITIONS_ATTRIBUTE	52	/* R/W, 2 bytes */
#define EXT_CSD_EXCEPTION_EVENTS_STATUS		54	/* RO, 2 bytes */
#define EXT_CSD_EXCEPTION_EVENTS_CTRL		56	/* R/W, 2 bytes */
#define EXT_CSD_EXCEPTION_DYNCAP_NEEDED		58	/* RO, 1 byte */
#define EXT_CSD_CLASS_6_CTRL			59	/* R/W */
#define EXT_CSD_INI_TIMEOUT_EMU			60	/* RO */
#define EXT_CSD_DATA_SECTOR_SIZE		61	/* RO */
#define EXT_CSD_USE_NATIVE_SECTOR		62	/* RO */
#define EXT_CSD_NATIVE_SECTOR_SIZE		63	/* RO */
#define EXT_CSD_PROGRAM_CID_CSD_DDR_SUPPORT	130	/* R/W */
#define EXT_CSD_PERIODIC_WAKEUP			131	/* R/W/ */
#define EXT_CSD_TCASE_SUPPORT			132	/* R/W */
#define EXT_CSD_PRODUCTION_STATE_AWARENESS	133	/* R/W */
#define EXT_CSD_SEC_BAD_BLK_MGMNT		134	/* R/W */
#define EXT_CSD_ENH_START_ADDR			136	/* R/W, 4 bytes */
#define EXT_CSD_ENH_SIZE_MULT			140	/* R/W, 3 bytes */
#define EXT_CSD_GP_SIZE_MULT0			143	/* R/W */
#define EXT_CSD_GP_SIZE_MULT1			146	/* R/W */
#define EXT_CSD_GP_SIZE_MULT2			149	/* R/W */
#define EXT_CSD_GP_SIZE_MULT3			152	/* R/W */
#define EXT_CSD_PARTITION_SETTING_COMPLETED	155	/* R/W */
#define EXT_CSD_PARTITIONS_ATTRIBUTE		156	/* R/W */
#define EXT_CSD_MAX_ENH_SIZE_MULT		157	/* RO, 3 bytes */
#define EXT_CSD_PARTITIONING_SUPPORT		160	/* RO */
#define EXT_CSD_HPI_MGMT			161	/* R/W */
#define EXT_CSD_RST_N_FUNCTION			162	/* R/W */
#define EXT_CSD_BKOPS_EN			163	/* R/W */
#define EXT_CSD_BKOPS_START			164	/* WO */
#define EXT_CSD_SANITIZE_START			165     /* W */
#define EXT_CSD_WR_REL_PARAM			166	/* RO */
#define EXT_CSD_WR_REL_SET			167	/* R/W */
#define EXT_CSD_RPMB_SIZE_MULT			168	/* RO */
#define EXT_CSD_FW_CONFIG			169	/* R/W */
#define EXT_CSD_USER_WP				171	/* R/W */
#define EXT_CSD_BOOT_WP				173	/* R/W */
#define EXT_CSD_BOOT_WP_STATUS			174	/* RO */
#define EXT_CSD_ERASE_GROUP_DEF			175	/* R/W */
#define EXT_CSD_BOOT_BUS_CONDITIONS		177	/* R/W */
#define EXT_CSD_BOOT_CONFIG_PROT		178	/* R/W */
#define EXT_CSD_PARTITION_CONFIG		179	/* R/W */
#define EXT_CSD_ERASED_MEM_CONT			181	/* RO */
#define EXT_CSD_BUS_WIDTH			183	/* R/W */
#define EXT_CSD_STROBE_SUPPORT			184	/* RO */
#define EXT_CSD_HS_TIMING			185	/* R/W */
#define EXT_CSD_POWER_CLASS			187	/* R/W */
#define EXT_CSD_CMD_SET_REV			189	/* R/W */
#define EXT_CSD_CMD_SET				191	/* R/W */
#define EXT_CSD_REV				192	/* RO */
#define EXT_CSD_CSD_STRUCTURE			194	/* RO */
#define EXT_CSD_DEVICE_TYPE			196	/* RO */
#define EXT_CSD_DRIVER_STRENGTH			197	/* RO */
#define EXT_CSD_OUT_OF_INTERRUPT_TIME		198	/* RO */
#define EXT_CSD_PARTITION_SWITCH_TIME		199	/* RO */
#define EXT_CSD_PWR_CL_52_195			200	/* RO */
#define EXT_CSD_PWR_CL_26_195			201	/* RO */
#define EXT_CSD_PWR_CL_52_360			202	/* RO */
#define EXT_CSD_PWR_CL_26_360			203	/* RO */
#define EXT_CSD_MIN_PERF_R_4_26			205	/* RO */
#define EXT_CSD_MIN_PERF_W_4_26			206	/* RO */
#define EXT_CSD_MIN_PERF_R_8_26_4_52		207	/* RO */
#define EXT_CSD_MIN_PERF_W_8_26_4_52		208	/* RO */
#define EXT_CSD_MIN_PERF_R_8_52			209	/* RO */
#define EXT_CSD_MIN_PERF_W_8_52			210	/* RO */
#define EXT_CSD_SECURE_WP_INFO			211	/* RO */
#define EXT_CSD_SEC_COUNT			212	/* RO, 4 bytes */
#define EXT_CSD_SLEEP_NOTIFICATION_TIME		216	/* RO */
#define EXT_CSD_S_A_TIMEOUT			217	/* RO */
#define EXT_CSD_PRODUCTION_ST8_AWARENSS_TIMEOUT	218	/* RO */
#define EXT_CSD_S_C_VCCQ			219	/* RO */
#define EXT_CSD_S_C_VCC				220	/* RO */
#define EXT_CSD_HC_WP_GRP_SIZE			221	/* RO */
#define EXT_CSD_REL_WR_SEC_C			222	/* RO */
#define EXT_CSD_ERASE_TIMEOUT_MULT		223	/* RO */
#define EXT_CSD_HC_ERASE_GRP_SIZE		224	/* RO */
#define EXT_CSD_ACC_SIZE			225	/* RO */
#define EXT_CSD_BOOT_SIZE_MULT			226	/* RO */
#define EXT_CSD_BOOT_INFO			228	/* RO */
#define EXT_CSD_SEC_TRIM_MULT			229	/* RO */
#define EXT_CSD_SEC_ERASE_MULT			230	/* RO */
#define EXT_CSD_SEC_FEATURE_SUPPORT		231	/* RO */
#define EXT_CSD_TRIM_MULT			232	/* RO */
#define EXT_CSD_MIN_PERF_DDR_R_8_52		234	/* RO */
#define EXT_CSD_MIN_PERF_DDR_W_8_52		235	/* RO */
#define EXT_CSD_PWR_CL_200_195			236	/* RO */
#define EXT_CSD_PWR_CL_200_360			237	/* RO */
#define EXT_CSD_PWR_CL_DDR_52_195		238	/* RO */
#define EXT_CSD_PWR_CL_DDR_52_360		239	/* RO */
#define EXT_CSD_CACHE_FLUSH_POLICY		240	/* RO */
#define EXT_CSD_INI_TIMEOUT_AP			241	/* RO */
#define EXT_CSD_CORRECTLY_PRG_SECTORS_NUM	242	/* RO, 4 bytes */
#define EXT_CSD_BKOPS_STATUS			246	/* RO */
#define EXT_CSD_POWER_OFF_LONG_TIME		247	/* RO */
#define EXT_CSD_GENERIC_CMD6_TIME		248	/* RO */
#define EXT_CSD_CACHE_SIZE			249	/* RO, 4 bytes */
#define EXT_CSD_FIRMWARE_VERSION		254	/* RO, 8 bytes */
#define EXT_CSD_DEVICE_VERSION			262	/* RO, 2 bytes */
#define EXT_CSD_OPTIMAL_TRIM_UNIT_SIZE		264	/* RO */
#define EXT_CSD_OPTIMAL_WRITE_SIZE		265	/* RO */
#define EXT_CSD_OPTIMAL_READ_SIZE		266	/* RO */
#define EXT_CSD_PRE_EOL_INFO			267	/* RO */
#define EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_A	268	/* RO */
#define EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_B	269	/* RO */
#define EXT_CSD_NMBR_OF_FW_SCTRS_CRRCTLY_PRGRMD	302	/* RO, 4 bytes */
#define EXT_CSD_CMDQ_DEPTH			307	/* RO */
#define EXT_CSD_CMDQ_SUPPORT			308	/* RO */
#define EXT_CSD_BARRIER_SUPPORT			486	/* RO */
#define EXT_CSD_FFU_ARG				487	/* RO, 4 bytes */
#define EXT_CSD_OPERATION_CODES_TIMEOUT		491	/* RO */
#define EXT_CSD_FFU_FEATURES			492	/* RO */
#define EXT_CSD_SUPPORTED_MODES			493	/* RO */
#define EXT_CSD_EXT_SUPPORT			494	/* RO */
#define EXT_CSD_LARGE_UNIT_SIZE_M1		495	/* RO */
#define EXT_CSD_CONTEXT_CAPABILITIES		496	/* RO */
#define EXT_CSD_TAG_RES_SIZE			497	/* RO */
#define EXT_CSD_TAG_UNIT_SIZE			498	/* RO */
#define EXT_CSD_DATA_TAG_SUPPORT		499	/* RO */
#define EXT_CSD_MAX_PACKED_WRITES		500	/* RO */
#define EXT_CSD_MAX_PACKED_READS		501	/* RO */
#define EXT_CSD_BKOPS_SUPPORT			502	/* RO */
#define EXT_CSD_HPI_FEATURES			503	/* RO */
#define EXT_CSD_S_CMD_SET			504	/* RO */
#define EXT_CSD_EXT_SECURITY_ERR		505	/* RO */

/*
 * EXT_CSD field definitions
 */
#define EXT_CSD_PART_CONFIG_ACC_MASK	(0x7)
#define EXT_CSD_PART_CONFIG_ACC_BOOT0	(0x1)

#define EXT_CSD_CMD_SET_NORMAL		(1<<0)
#define EXT_CSD_CMD_SET_SECURE		(1<<1)
#define EXT_CSD_CMD_SET_CPSECURE	(1<<2)

#define EXT_CSD_CARD_TYPE_MASK		0x3f
#define EXT_CSD_CARD_TYPE_26		(1<<0)	/* Card can run at 26MHz */
#define EXT_CSD_CARD_TYPE_52		(1<<1)	/* Card can run at 52MHz */
#define EXT_CSD_CARD_TYPE_DDR_1_8V	(1<<2)	/* Card can run at 52MHz */
						/* DDR mode @1.8V or 3V I/O */
#define EXT_CSD_CARD_TYPE_DDR_1_2V	(1<<3)	/* Card can run at 52MHz */
						/* DDR mode @1.2V I/O */
#define EXT_CSD_CARD_TYPE_SDR_1_8V	(1<<4)	/* Card can run at 200MHz */
#define EXT_CSD_CARD_TYPE_SDR_1_2V	(1<<5)	/* Card can run at 200MHz */
						/* SDR mode @1.2V I/O */

/* register PARTITIONS_ATTRIBUTE [156] */
#define EXT_CSD_ENH_USR_MASK		(1 << 0)

/* register PARTITIONING_SUPPORT [160] */
#define EXT_CSD_ENH_ATTRIBUTE_EN_MASK	(1 << 0)

/* register BUS_WIDTH [183], field Bus Mode Selection [4:0] */
#define EXT_CSD_BUS_WIDTH_1	0	/* Card is in 1 bit mode */
#define EXT_CSD_BUS_WIDTH_4	1	/* Card is in 4 bit mode */
#define EXT_CSD_BUS_WIDTH_8	2	/* Card is in 8 bit mode */
#define EXT_CSD_DDR_BUS_WIDTH_4	5	/* Card is in 4 bit DDR mode */
#define EXT_CSD_DDR_BUS_WIDTH_8	6	/* Card is in 8 bit DDR mode */

#define R1_ILLEGAL_COMMAND		(1 << 22)
#define R1_APP_CMD			(1 << 5)

#define R1_SPI_IDLE		(1 << 0)
#define R1_SPI_ERASE_RESET	(1 << 1)
#define R1_SPI_ILLEGAL_COMMAND	(1 << 2)
#define R1_SPI_COM_CRC		(1 << 3)
#define R1_SPI_ERASE_SEQ	(1 << 4)
#define R1_SPI_ADDRESS		(1 << 5)
#define R1_SPI_PARAMETER	(1 << 6)
#define R1_SPI_ERROR		(1 << 7)

/* response types */
#define MMC_RSP_PRESENT (1 << 0)
#define MMC_RSP_136     (1 << 1)                /* 136 bit response */
#define MMC_RSP_CRC     (1 << 2)                /* expect valid crc */
#define MMC_RSP_BUSY    (1 << 3)                /* card may send busy */
#define MMC_RSP_OPCODE  (1 << 4)                /* response contains opcode */

#define MMC_RSP_NONE    (0)
#define MMC_RSP_R1      (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R1b	(MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE|MMC_RSP_BUSY)
#define MMC_RSP_R2      (MMC_RSP_PRESENT|MMC_RSP_136|MMC_RSP_CRC)
#define MMC_RSP_R3      (MMC_RSP_PRESENT)
#define MMC_RSP_R4      (MMC_RSP_PRESENT)
#define MMC_RSP_R5      (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R6      (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R7      (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)

/** command information to be sent to the SD/MMC card */
struct mci_cmd {
	unsigned cmdidx;	/**< Command to be sent to the SD/MMC card */
	unsigned resp_type;	/**< Type of expected response, refer MMC_RSP_* macros */
	unsigned cmdarg;	/**< Command's arguments */
	unsigned response[4];	/**< card's response */
};

/** data information to be used with some SD/MMC commands */
struct mci_data {
	union {
		uint8_t *dest;
		const uint8_t *src; /**< src buffers don't get written to */
	};
	unsigned flags;		/**< refer MMC_DATA_* to define direction */
	unsigned blocks;	/**< block count to handle in this command */
	unsigned blocksize;	/**< block size in bytes (mostly 512) */
};

struct mci_ios {
	unsigned int	clock;			/* clock rate */

	unsigned char	bus_width;		/* data bus width */

#define MMC_BUS_WIDTH_1		0
#define MMC_BUS_WIDTH_4		2
#define MMC_BUS_WIDTH_8		3

	unsigned char	timing;			/* timing specification used */

#define MMC_TIMING_LEGACY	0
#define MMC_TIMING_MMC_HS	1
#define MMC_TIMING_SD_HS	2
#define MMC_TIMING_UHS_SDR12	MMC_TIMING_LEGACY
#define MMC_TIMING_UHS_SDR25	MMC_TIMING_SD_HS
#define MMC_TIMING_UHS_SDR50	3
#define MMC_TIMING_UHS_SDR104	4
#define MMC_TIMING_UHS_DDR50	5
#define MMC_TIMING_MMC_HS200	6

#define MMC_SDR_MODE		0
#define MMC_1_2V_DDR_MODE	1
#define MMC_1_8V_DDR_MODE	2
#define MMC_1_2V_SDR_MODE	3
#define MMC_1_8V_SDR_MODE	4
};

struct mci;

/** host information */
struct mci_host {
	struct device_d *hw_dev;	/**< the host MCI hardware device */
	struct mci *mci;
	const char *devname;		/**< the devicename for the card, defaults to disk%d */
	unsigned voltages;
	unsigned host_caps;	/**< Host's interface capabilities, refer MMC_VDD_* */
	unsigned f_min;		/**< host interface lower limit */
	unsigned f_max;		/**< host interface upper limit */
	unsigned clock;		/**< Current clock used to talk to the card */
	unsigned bus_width;	/**< used data bus width to the card */
	unsigned max_req_size;
	unsigned dsr_val;	/**< optional dsr value */
	int use_dsr;		/**< optional dsr usage flag */
	bool non_removable;	/**< device is non removable */
	bool no_sd;		/**< do not send SD commands during initialization */
	struct regulator *supply;

	/** init the host interface */
	int (*init)(struct mci_host*, struct device_d*);
	/** change host interface settings */
	void (*set_ios)(struct mci_host*, struct mci_ios *);
	/** handle a command */
	int (*send_cmd)(struct mci_host*, struct mci_cmd*, struct mci_data*);
	/** check if a card is inserted */
	int (*card_present)(struct mci_host *);
	/** check if a card is write protected */
	int (*card_write_protected)(struct mci_host *);
};

#define MMC_NUM_BOOT_PARTITION	2
#define MMC_NUM_GP_PARTITION	4
#define MMC_NUM_PHY_PARTITION	6

struct mci_part {
	struct block_device	blk;		/**< the blockdevice for the card */
	struct mci		*mci;
	uint64_t		size;		/* partition size (in bytes) */
	unsigned int		part_cfg;	/* partition type */
	char			*name;
	int			idx;
	unsigned int		area_type;
#define MMC_BLK_DATA_AREA_MAIN	(1<<0)
#define MMC_BLK_DATA_AREA_BOOT	(1<<1)
#define MMC_BLK_DATA_AREA_GP	(1<<2)
#define MMC_BLK_DATA_AREA_RPMB	(1<<3)
};

/** MMC/SD and interface instance information */
struct mci {
	struct mci_host *host;		/**< the host for this card */
	struct device_d dev;		/**< the device for our disk (mcix) */
	unsigned version;
	/** != 0 when a high capacity card is connected (OCR -> OCR_HCS) */
	int high_capacity;
	unsigned card_caps;	/**< Card's capabilities */
	unsigned ocr;		/**< card's "operation condition register" */
	unsigned scr[2];
	unsigned csd[4];	/**< card's "card specific data register" */
	unsigned cid[4];	/**< card's "card identification register" */
	unsigned short rca;	/* FIXME */
	unsigned tran_speed;	/**< Maximum transfer speed */
	/** currently used data block length for read accesses */
	unsigned read_bl_len;
	/** currently used data block length for write accesses */
	unsigned write_bl_len;
	uint64_t capacity;	/**< Card's data capacity in bytes */
	int ready_for_use;	/** true if already probed */
	int dsr_imp;		/**< DSR implementation state from CSD */
	char *ext_csd;
	int probe;
	struct param_d *param_probe;
	struct param_d *param_boot;
	int bootpart;

	struct mci_part part[MMC_NUM_PHY_PARTITION];
	int nr_parts;
	char *cdevname;

	struct mci_part *part_curr;
	u8 ext_csd_part_config;

	struct list_head list;     /* The list of all mci devices */
};

int mci_register(struct mci_host*);
void mci_of_parse(struct mci_host *host);
void mci_of_parse_node(struct mci_host *host, struct device_node *np);
int mci_detect_card(struct mci_host *);
int mci_send_ext_csd(struct mci *mci, char *ext_csd);
int mci_switch(struct mci *mci, unsigned index, unsigned value);

static inline int mmc_host_is_spi(struct mci_host *host)
{
	if (IS_ENABLED(CONFIG_MCI_SPI))
		return host->host_caps & MMC_CAP_SPI;
	else
		return 0;
}

struct mci *mci_get_device_by_name(const char *name);

static inline struct mci *mci_get_device_by_devpath(const char *devpath)
{
	return mci_get_device_by_name(devpath_to_name(devpath));
}

#endif /* _MCI_H_ */
