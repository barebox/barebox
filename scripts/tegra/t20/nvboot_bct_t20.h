/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 */

#ifndef INCLUDED_NVBOOT_BCT_T20_H
#define INCLUDED_NVBOOT_BCT_T20_H

#include <sys/types.h>
#include "nvboot_sdram_param_t20.h"

/**
 * Defines the number of 32-bit words in the customer_data area of the BCT.
 */
#define NVBOOT_BCT_CUSTOMER_DATA_WORDS 296

/**
 * Defines the number of bytes in the customer_data area of the BCT.
 */
#define NVBOOT_BCT_CUSTOMER_DATA_SIZE \
		(NVBOOT_BCT_CUSTOMER_DATA_WORDS * 4)

/**
 * Defines the number of bytes in the reserved area of the BCT.
 */
#define NVBOOT_BCT_RESERVED_SIZE       3

/**
 * Defines the maximum number of bootloader descriptions in the BCT.
 */
#define NVBOOT_MAX_BOOTLOADERS         4

/**
 * Defines the maximum number of device parameter sets in the BCT.
 * The value must be equal to (1 << # of device straps)
 */
#define NVBOOT_BCT_MAX_PARAM_SETS      4

/**
 * Defines the maximum number of SDRAM parameter sets in the BCT.
 * The value must be equal to (1 << # of SDRAM straps)
 */
#define NVBOOT_BCT_MAX_SDRAM_SETS      4

/**
 * Defines the number of entries (bits) in the bad block table.
 * The consequences of changing its value are as follows.  Using P as the
 * # of physical blocks in the boot loader and B as the value of this
 * constant:
 *    B > P: There will be unused storage in the bad block table.
 *    B < P: The virtual block size will be greater than the physical block
 *           size, so the granularity of the bad block table will be less than
 *           one bit per physical block.
 *
 * 4096 bits is enough to represent an 8MiB partition of 2KiB blocks with one
 * bit per block (1 virtual block = 1 physical block).  This occupies 512 bytes
 * of storage.
 */
#define NVBOOT_BAD_BLOCK_TABLE_SIZE 4096

/**
 * Defines the maximum number of blocks to search for BCTs.
 *
 * This value covers the initial block and a set of journal blocks.
 *
 * Ideally, this number will span several erase units for reliable updates
 * and tolerance for blocks to become bad with use.  Safe updates require
 * a minimum of 2 erase units in which BCTs can appear.
 *
 * To ensure that the BCT search spans a sufficient range of configurations,
 * the search block count has been set to 64. This allows for redundancy with
 * a wide range of parts and provides room for greater problems in this
 * region of the device.
 */
#define NVBOOT_MAX_BCT_SEARCH_BLOCKS   64

/*
 * Defines the CMAC-AES-128 hash length in 32 bit words. (128 bits = 4 words)
 */
enum {NVBOOT_CMAC_AES_HASH_LENGTH = 4};

/**
 * Defines the storage for a hash value (128 bits).
 */
typedef struct nvboot_hash_rec {
	u_int32_t hash[NVBOOT_CMAC_AES_HASH_LENGTH];
} nvboot_hash;

/* Defines the params that can be configured for NAND devices. */
typedef struct nvboot_nand_params_rec {
	/**
	 * Specifies the clock divider for the PLL_P 432MHz source.
	 * If it is set to 18, then clock source to Nand controller is
	 * 432 / 18 = 24MHz.
	 */
	u_int8_t clock_divider;

	/* Specifies the value to be programmed to Nand Timing Register 1 */
	u_int32_t nand_timing;

	/* Specifies the value to be programmed to Nand Timing Register 2 */
	u_int32_t nand_timing2;

	/* Specifies the block size in log2 bytes */
	u_int8_t block_size_log2;

	/* Specifies the page size in log2 bytes */
	u_int8_t page_size_log2;
} nvboot_nand_params;

/* Defines various data widths supported. */
typedef enum {
	/**
	 * Specifies a 1 bit interface to eMMC.
	 * Note that 1-bit data width is only for the driver's internal use.
	 * Fuses doesn't provide option to select 1-bit data width.
	 * The driver selects 1-bit internally based on need.
	 * It is used for reading Extended CSD and when the power class
	 * requirements of a card for 4-bit or 8-bit transfers are not
	 * supported by the target board.
	 */
	nvboot_sdmmc_data_width_1bit = 0,

	/* Specifies a 4 bit interface to eMMC. */
	nvboot_sdmmc_data_width_4bit = 1,

	/* Specifies a 8 bit interface to eMMC. */
	nvboot_sdmmc_data_width_8bit = 2,

	nvboot_sdmmc_data_width_num,
	nvboot_sdmmc_data_width_force32 = 0x7FFFFFFF
} nvboot_sdmmc_data_width;

/* Defines the parameters that can be changed after BCT is read. */
typedef struct nvboot_sdmmc_params_rec {
	/**
	 * Specifies the clock divider for the SDMMC controller's clock source,
	 * which is PLLP running at 432MHz.  If it is set to 18, then the SDMMC
	 * controller runs at 432/18 = 24MHz.
	 */
	u_int8_t clock_divider;

	/* Specifies the data bus width. Supported data widths are 4/8 bits. */
	nvboot_sdmmc_data_width data_width;

	/**
	 * Max Power class supported by the target board.
	 * The driver determines the best data width and clock frequency
	 * supported within the power class range (0 to Max) if the selected
	 * data width cannot be used at the chosen clock frequency.
	 */
	u_int8_t max_power_class_supported;
} nvboot_sdmmc_params;

typedef enum {
	/* Specifies SPI clock source to be PLLP. */
	nvboot_spi_clock_source_pllp_out0 = 0,

	/* Specifies SPI clock source to be PLLC. */
	nvboot_spi_clock_source_pllc_out0,

	/* Specifies SPI clock source to be PLLM. */
	nvboot_spi_clock_source_pllm_out0,

	/* Specifies SPI clock source to be ClockM. */
	nvboot_spi_clock_source_clockm,

	nvboot_spi_clock_source_num,
	nvboot_spi_clock_source_force32 = 0x7FFFFFF
} nvboot_spi_clock_source;


/**
 * Defines the parameters SPI FLASH devices.
 */
typedef struct nvboot_spiflash_params_rec {
	/**
	 * Specifies the clock source to use.
	 */
	nvboot_spi_clock_source clock_source;

	/**
	 * Specifes the clock divider to use.
	 * The value is a 7-bit value based on an input clock of 432Mhz.
	 * Divider = (432+ DesiredFrequency-1)/DesiredFrequency;
	 * Typical values:
	 *     NORMAL_READ at 20MHz: 22
	 *     FAST_READ   at 33MHz: 14
	 *     FAST_READ   at 40MHz: 11
	 *     FAST_READ   at 50MHz:  9
	 */
	u_int8_t clock_divider;

	/**
	 * Specifies the type of command for read operations.
	 * NV_FALSE specifies a NORMAL_READ Command
	 * NV_TRUE  specifies a FAST_READ   Command
	 */
	u_int8_t read_command_type_fast;
} nvboot_spiflash_params;

/**
* Defines the union of the parameters required by each device.
*/
typedef union {
	/* Specifies optimized parameters for NAND */
	nvboot_nand_params nand_params;
	/* Specifies optimized parameters for eMMC and eSD */
	nvboot_sdmmc_params sdmmc_params;
	/* Specifies optimized parameters for SPI NOR */
	nvboot_spiflash_params spiflash_params;
} nvboot_dev_params;

/**
 * Identifies the types of devices from which the system booted.
 * Used to identify primary and secondary boot devices.
 * @note These no longer match the fuse API device values (for
 * backward compatibility with AP15).
 */
typedef enum {
	/* Specifies a default (unset) value. */
	nvboot_dev_type_none = 0,

	/* Specifies NAND. */
	nvboot_dev_type_nand,

	/* Specifies SPI NOR. */
	nvboot_dev_type_spi = 3,

	/* Specifies SDMMC (either eMMC or eSD). */
	nvboot_dev_type_sdmmc,

	nvboot_dev_type_max,

	/* Ignore -- Forces compilers to make 32-bit enums. */
	nvboot_dev_type_force32 = 0x7FFFFFFF
} nvboot_dev_type;

/**
 * Stores information needed to locate and verify a boot loader.
 *
 * There is one \c nv_bootloader_info structure for each copy of a BL stored on
 * the device.
 */
typedef struct nv_bootloader_info_rec {
	u_int32_t version;
	u_int32_t start_blk;
	u_int32_t start_page;
	u_int32_t length;
	u_int32_t load_addr;
	u_int32_t entry_point;
	u_int32_t attribute;
	nvboot_hash crypto_hash;
} nv_bootloader_info;

/**
 * Defines the bad block table structure stored in the BCT.
 */
typedef struct nvboot_badblock_table_rec {
	u_int32_t entries_used;
	u_int8_t virtual_blk_size_log2;
	u_int8_t block_size_log2;
	u_int8_t bad_blks[NVBOOT_BAD_BLOCK_TABLE_SIZE / 8];
} nvboot_badblock_table;

/**
 * Contains the information needed to load BLs from the secondary boot device.
 *
 * - Supplying NumParamSets = 0 indicates not to load any of them.
 * - Supplying NumDramSets  = 0 indicates not to load any of them.
 * - The \c random_aes_blk member exists to increase the difficulty of
 *   key attacks based on knowledge of this structure.
 */
typedef struct nvboot_config_table_rec {
	nvboot_hash crypto_hash;
	nvboot_hash random_aes_blk;
	u_int32_t boot_data_version;
	u_int32_t block_size_log2;
	u_int32_t page_size_log2;
	u_int32_t partition_size;
	u_int32_t num_param_sets;
	nvboot_dev_type dev_type[NVBOOT_BCT_MAX_PARAM_SETS];
	nvboot_dev_params dev_params[NVBOOT_BCT_MAX_PARAM_SETS];
	u_int32_t num_sdram_sets;
	nvboot_sdram_params sdram_params[NVBOOT_BCT_MAX_SDRAM_SETS];
	nvboot_badblock_table badblock_table;
	u_int32_t bootloader_used;
	nv_bootloader_info bootloader[NVBOOT_MAX_BOOTLOADERS];
	u_int8_t customer_data[NVBOOT_BCT_CUSTOMER_DATA_SIZE];

	/*
	 * ODMDATA is stored in the BCT in IRAM by the BootROM.
	 * Read the data @ bct_start + (bct_size - 12). This works
	 * on T20 and T30 BCTs, which are locked down. If this changes
	 * in new chips, we can revisit this algorithm.
	 */
	u_int32_t odm_data;
	u_int32_t reserved1;
	u_int8_t enable_fail_back;
	u_int8_t reserved[NVBOOT_BCT_RESERVED_SIZE];
} nvboot_config_table;
#endif /* #ifndef INCLUDED_NVBOOT_BCT_T20_H */
