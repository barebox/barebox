/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_IMX_NAND_BCB_H
#define __MACH_IMX_NAND_BCB_H

#define	FCB_FINGERPRINT		0x20424346	/* 'FCB' */
#define	FCB_VERSION_1		0x01000000
#define	FCB_FINGERPRINT_OFF	0x4		/* FCB fingerprint offset*/

#define	DBBT_FINGERPRINT	0x54424244	/* 'DBBT' */
#define	DBBT_VERSION_1		0x01000000
#define	DBBT_FINGERPRINT_OFF	0x4		/* DBBT fingerprint offset*/

struct dbbt_block {
	uint32_t Checksum;
	uint32_t FingerPrint;
	uint32_t Version;
	uint32_t numberBB; /* reserved on i.MX6 */
	uint32_t DBBTNumOfPages;
};

struct fcb_block {
	uint32_t Checksum;		/* First fingerprint in first byte */
	uint32_t FingerPrint;		/* 2nd fingerprint at byte 4 */
	uint32_t Version;		/* 3rd fingerprint at byte 8 */
	uint8_t DataSetup;
	uint8_t DataHold;
	uint8_t AddressSetup;
	uint8_t DSAMPLE_TIME;
	/* These are for application use only and not for ROM. */
	uint8_t NandTimingState;
	uint8_t REA;
	uint8_t RLOH;
	uint8_t RHOH;
	uint32_t PageDataSize;		/* 2048 for 2K pages, 4096 for 4K pages */
	uint32_t TotalPageSize;		/* 2112 for 2K pages, 4314 for 4K pages */
	uint32_t SectorsPerBlock;	/* Number of 2K sections per block */
	uint32_t NumberOfNANDs;		/* Total Number of NANDs - not used by ROM */
	uint32_t TotalInternalDie;	/* Number of separate chips in this NAND */
	uint32_t CellType;		/* MLC or SLC */
	uint32_t EccBlockNEccType;	/* Type of ECC, can be one of BCH-0-20 */
	uint32_t EccBlock0Size;		/* Number of bytes for Block0 - BCH */
	uint32_t EccBlockNSize;		/* Block size in bytes for all blocks other than Block0 - BCH */
	uint32_t EccBlock0EccType;	/* Ecc level for Block 0 - BCH */
	uint32_t MetadataBytes;		/* Metadata size - BCH */
	uint32_t NumEccBlocksPerPage;	/* Number of blocks per page for ROM use - BCH */
	uint32_t EccBlockNEccLevelSDK;	/* Type of ECC, can be one of BCH-0-20 */
	uint32_t EccBlock0SizeSDK;	/* Number of bytes for Block0 - BCH */
	uint32_t EccBlockNSizeSDK;	/* Block size in bytes for all blocks other than Block0 - BCH */
	uint32_t EccBlock0EccLevelSDK;	/* Ecc level for Block 0 - BCH */
	uint32_t NumEccBlocksPerPageSDK;/* Number of blocks per page for SDK use - BCH */
	uint32_t MetadataBytesSDK;	/* Metadata size - BCH */
	uint32_t EraseThreshold;	/* To set into BCH_MODE register */
	uint32_t BootPatch;		/* 0 for normal boot and 1 to load patch starting next to FCB */
	uint32_t PatchSectors;		/* Size of patch in sectors */
	uint32_t Firmware1_startingPage;/* Firmware image starts on this sector */
	uint32_t Firmware2_startingPage;/* Secondary FW Image starting Sector */
	uint32_t PagesInFirmware1;	/* Number of sectors in firmware image */
	uint32_t PagesInFirmware2;	/* Number of sector in secondary FW image */
	uint32_t DBBTSearchAreaStartAddress; /* Page address where dbbt search area begins */
	uint32_t BadBlockMarkerByte;	/* Byte in page data that have manufacturer marked bad block marker, */
					/* this will be swapped with metadata[0] to complete page data. */
	uint32_t BadBlockMarkerStartBit;/* For BCH ECC sizes other than 8 and 16 the bad block marker does not */
					/* start at 0th bit of BadBlockMarkerByte. This field is used to get to */
					/* the start bit of bad block marker byte with in BadBlockMarkerByte */
	uint32_t BBMarkerPhysicalOffset;/* FCB value that gives byte offset for bad block marker on physical NAND page */
	uint32_t BCHType;

	uint32_t TMTiming2_ReadLatency;
	uint32_t TMTiming2_PreambleDelay;
	uint32_t TMTiming2_CEDelay;
	uint32_t TMTiming2_PostambleDelay;
	uint32_t TMTiming2_CmdAddPause;
	uint32_t TMTiming2_DataPause;
	uint32_t TMSpeed;
	uint32_t TMTiming1_BusyTimeout;

	uint32_t DISBBM;	/* the flag to enable (1)/disable(0) bi swap */
	uint32_t BBMarkerPhysicalOffsetInSpareData; /* The swap position of main area in spare area */
};

#endif /* __MACH_IMX_NAND_BCB_H */
