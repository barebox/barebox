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

#ifndef INCLUDED_NVBOOT_BCT_T114_H
#define INCLUDED_NVBOOT_BCT_T114_H

#include <sys/types.h>
#include "nvboot_sdram_param_t114.h"

/**
 * Defines the number of 32-bit words in the customer_data area of the BCT.
 */
#define NVBOOT_BCT_CUSTOMER_DATA_WORDS		178

/**
 * Defines the number of bytes in the customer_data area of the BCT.
 */
#define NVBOOT_BCT_CUSTOMER_DATA_SIZE \
		(NVBOOT_BCT_CUSTOMER_DATA_WORDS * 4)

/**
 * Defines the number of bytes in the reserved area of the BCT.
 */
#define NVBOOT_BCT_RESERVED_SIZE		2

/**
 * Defines the maximum number of bootloader descriptions in the BCT.
 */
#define NVBOOT_MAX_BOOTLOADERS			4

/**
 * Defines the maximum number of device parameter sets in the BCT.
 * The value must be equal to (1 << # of device straps)
 */
#define NVBOOT_BCT_MAX_PARAM_SETS		4

/**
 * Defines the maximum number of SDRAM parameter sets in the BCT.
 * The value must be equal to (1 << # of SDRAM straps)
 */
#define NVBOOT_BCT_MAX_SDRAM_SETS		4

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
#define NVBOOT_BAD_BLOCK_TABLE_SIZE		4096

/**
 * Defines the amount of padding needed to pad the bad block table to a
 * multiple of AES block size.
 */
#define NVBOOT_BAD_BLOCK_TABLE_PADDING	10

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
#define NVBOOT_MAX_BCT_SEARCH_BLOCKS	64

#define ARSE_RSA_MAX_MODULUS_SIZE		2048

/**
 * Defines the RSA modulus length in bits and bytes used for PKC secure boot.
 */
enum {NVBOOT_SE_RSA_MODULUS_LENGTH_BITS = ARSE_RSA_MAX_MODULUS_SIZE};

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

/*
 *  Defines the storage for the RSA public key's modulus
 *  in the BCT
 */
typedef struct nvboot_rsa_key_modulus_rec
{
	/// The modulus size is 2048-bits.
	u_int32_t modulus[NVBOOT_SE_RSA_MODULUS_LENGTH_BITS / 8 / 4];
} nvboot_rsa_key_modulus;

typedef struct nvboot_rsa_pss_sig_rec
{
	/*
	 * The RSA-PSS signature length is equal to the
	 * length in octets of the RSA modulus.
	 * In our case, it's 2048-bits.
	 */
	u_int32_t signature[NVBOOT_SE_RSA_MODULUS_LENGTH_BITS / 8 / 4];
} nvboot_rsa_pss_sig;

typedef union nvboot_object_signature_rec
{
	/*
	 * Specifies the AES-CMAC signature for the rest of the BCT structure if symmetric key
	 * encryption secure boot scheme is used.
	 */
	nvboot_hash crypto_hash;

	/*
	 * Specifies the RSASSA-PSS signature for the rest of the BCT structure if public
	 * key cryptography secure boot scheme is used.
	 */
	nvboot_rsa_pss_sig rsa_pss_sig;
} nvboot_object_signature;

typedef struct nvboot_ecid_rec
{
	u_int32_t	ecid_0;
	u_int32_t	ecid_1;
	u_int32_t	ecid_2;
	u_int32_t	ecid_3;
} nvboot_ecid;

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
	/* Specifies a 4 bit Ddr interface to eMMC. */
	nvboot_sdmmc_data_width_ddr_4bit = 5,
	/* Specifies a 8 bit Ddr interface to eMMC. */
	nvboot_sdmmc_data_width_ddr_8bit = 6,

	nvboot_sdmmc_data_width_num,
	nvboot_sdmmc_data_width_force32 = 0x7FFFFFFF
} nvboot_sdmmc_data_width;

/* Defines the parameters that can be changed after BCT is read. */
typedef struct nvboot_sdmmc_params_rec {
	/**
	 * Specifies the clock divider for the SDMMC controller's clock source,
	 * which is PLLP running at 216MHz.  If it is set to 9, then the SDMMC
	 * controller runs at 216/9 = 24MHz.
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

	/* Specifies the max page size supported by driver */
	u_int8_t multi_page_support;
} nvboot_sdmmc_params;

typedef enum {
	/* Specifies SPI clock source to be PLLP. */
	nvboot_spi_clock_source_pllp_out0 = 0,

	/* Specifies SPI clock source to be ClockM. */
	nvboot_spi_clock_source_clockm = 6,

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
	u_int32_t clock_source;

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

	/* 0 = 2k page size, 1 = 16K page size */
	u_int8_t page_size_2k_or_16k;
} nvboot_spiflash_params;

/**
* Defines the union of the parameters required by each device.
*/
typedef union {
	u_int8_t size[64];
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

	/* Specifies SPI NOR. */
	nvboot_dev_type_spi = 3,

	/* Specifies SDMMC (either eMMC or eSD). */
	nvboot_dev_type_sdmmc = 4,

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

	/* Specifies the AES-CMAC MAC or RSASSA-PSS signature of the BL. */
	nvboot_object_signature signature;
} nv_bootloader_info;

/**
 * Defines the bad block table structure stored in the BCT.
 */
typedef struct nvboot_badblock_table_rec {
	u_int32_t entries_used;
	u_int8_t virtual_blk_size_log2;
	u_int8_t block_size_log2;
	u_int8_t bad_blks[NVBOOT_BAD_BLOCK_TABLE_SIZE / 8];
	/*
	 * Add a reserved field as padding to make the bad block table structure
	 * a multiple of 16 bytes (AES block size).
	 */
	u_int8_t reserved[NVBOOT_BAD_BLOCK_TABLE_PADDING];
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
	nvboot_badblock_table badblock_table;
	nvboot_rsa_key_modulus key;
	nvboot_object_signature signature;
	u_int8_t customer_data[NVBOOT_BCT_CUSTOMER_DATA_SIZE];
	u_int32_t odm_data;
	u_int32_t reserved1;

	/* START OF SIGNED SECTION OF THE BCT */
	nvboot_hash random_aes_blk;
	nvboot_ecid unique_chip_id;
	u_int32_t boot_data_version;
	u_int32_t block_size_log2;
	u_int32_t page_size_log2;
	u_int32_t partition_size;
	u_int32_t num_param_sets;
	nvboot_dev_type dev_type[NVBOOT_BCT_MAX_PARAM_SETS];
	nvboot_dev_params dev_params[NVBOOT_BCT_MAX_PARAM_SETS];
	u_int32_t num_sdram_sets;
	nvboot_sdram_params sdram_params[NVBOOT_BCT_MAX_SDRAM_SETS];

	u_int32_t bootloader_used;
	nv_bootloader_info bootloader[NVBOOT_MAX_BOOTLOADERS];

	u_int8_t enable_fail_back;

	/*
	 * Specify whether or not to enable JTAG access when the JTAG disable fuse
	 * has not been burned.
	 * SecureJtagControl = NV_FALSE (0) = Disable JTAG access.
	 * SecureJtagControl = NV_TRUE (1) = Enable JTAG access.
	 */
	u_int8_t secure_jtag_control;
	u_int8_t reserved[NVBOOT_BCT_RESERVED_SIZE];
} nvboot_config_table;
#endif /* #ifndef INCLUDED_NVBOOT_BCT_T114_H */
