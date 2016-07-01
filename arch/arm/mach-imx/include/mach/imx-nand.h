#ifndef __ASM_ARCH_NAND_H
#define __ASM_ARCH_NAND_H

#include <linux/mtd/mtd.h>

void imx21_barebox_boot_nand_external(void *boarddata);
void imx25_barebox_boot_nand_external(void *boarddata);
void imx27_barebox_boot_nand_external(void *boarddata);
void imx31_barebox_boot_nand_external(void *boarddata);
void imx35_barebox_boot_nand_external(void *boarddata);
void imx_nand_set_layout(int writesize, int datawidth);

struct imx_nand_platform_data {
	int width;
	unsigned int hw_ecc:1;
	unsigned int flash_bbt:1;
};

#define nfc_is_v21()		(cpu_is_mx25() || cpu_is_mx35())
#define nfc_is_v1()		(cpu_is_mx31() || cpu_is_mx27() || cpu_is_mx21())
#define nfc_is_v3_2()		(cpu_is_mx51() || cpu_is_mx53())
#define nfc_is_v3()		nfc_is_v3_2()

#define NFC_V1_ECC_STATUS_RESULT	0x0c
#define NFC_V1_RSLTMAIN_AREA		0x0e
#define NFC_V1_RSLTSPARE_AREA		0x10

#define NFC_V2_ECC_STATUS_RESULT1	0x0c
#define NFC_V2_ECC_STATUS_RESULT2	0x0e
#define NFC_V2_SPAS			0x10

#define NFC_V1_V2_BUF_SIZE		0x00
#define NFC_V1_V2_BUF_ADDR		0x04
#define NFC_V1_V2_FLASH_ADDR		0x06
#define NFC_V1_V2_FLASH_CMD		0x08
#define NFC_V1_V2_CONFIG		0x0a

#define NFC_V1_V2_WRPROT		0x12
#define NFC_V1_UNLOCKSTART_BLKADDR	0x14
#define NFC_V1_UNLOCKEND_BLKADDR	0x16
#define NFC_V21_UNLOCKSTART_BLKADDR	0x20
#define NFC_V21_UNLOCKEND_BLKADDR	0x22
#define NFC_V1_V2_NF_WRPRST		0x18
#define NFC_V1_V2_CONFIG1		0x1a
#define NFC_V1_V2_CONFIG2		0x1c

#define NFC_V2_CONFIG1_ECC_MODE_4       (1 << 0)
#define NFC_V1_V2_CONFIG1_SP_EN		(1 << 2)
#define NFC_V1_V2_CONFIG1_ECC_EN	(1 << 3)
#define NFC_V1_V2_CONFIG1_INT_MSK	(1 << 4)
#define NFC_V1_V2_CONFIG1_BIG		(1 << 5)
#define NFC_V1_V2_CONFIG1_RST		(1 << 6)
#define NFC_V1_V2_CONFIG1_CE		(1 << 7)
#define NFC_V1_V2_CONFIG1_ONE_CYCLE	(1 << 8)
#define NFC_V2_CONFIG1_PPB(x)		(((x) & 0x3) << 9)
#define NFC_V2_CONFIG1_FP_INT		(1 << 11)

#define NFC_V1_V2_CONFIG2_INT		(1 << 15)

#define NFC_V2_SPAS_SPARESIZE(spas)	((spas) >> 1)

#define NFC_V3_FLASH_CMD		(host->regs_axi + 0x00)
#define NFC_V3_FLASH_ADDR0		(host->regs_axi + 0x04)

#define NFC_V3_CONFIG1			(host->regs_axi + 0x34)
#define NFC_V3_CONFIG1_SP_EN		(1 << 0)
#define NFC_V3_CONFIG1_RBA(x)		(((x) & 0x7 ) << 4)

#define NFC_V3_ECC_STATUS_RESULT	(host->regs_axi + 0x38)

#define NFC_V3_LAUNCH			(host->regs_axi + 0x40)

#define NFC_V3_WRPROT			(host->regs_ip + 0x0)
#define NFC_V3_WRPROT_LOCK_TIGHT	(1 << 0)
#define NFC_V3_WRPROT_LOCK		(1 << 1)
#define NFC_V3_WRPROT_UNLOCK		(1 << 2)
#define NFC_V3_WRPROT_BLS_UNLOCK	(2 << 6)

#define NFC_V3_WRPROT_UNLOCK_BLK_ADD0   (host->regs_ip + 0x04)

#define NFC_V3_CONFIG2			(host->regs_ip + 0x24)
#define NFC_V3_CONFIG2_PS_512			(0 << 0)
#define NFC_V3_CONFIG2_PS_2048			(1 << 0)
#define NFC_V3_CONFIG2_PS_4096			(2 << 0)
#define NFC_V3_CONFIG2_ONE_CYCLE		(1 << 2)
#define NFC_V3_CONFIG2_ECC_EN			(1 << 3)
#define NFC_V3_CONFIG2_2CMD_PHASES		(1 << 4)
#define NFC_V3_CONFIG2_NUM_ADDR_PHASE0		(1 << 5)
#define NFC_V3_CONFIG2_ECC_MODE_8		(1 << 6)
#define NFC_V3_MX51_CONFIG2_PPB(x)		(((x) & 0x3) << 7)
#define NFC_V3_MX53_CONFIG2_PPB(x)		(((x) & 0x3) << 8)
#define NFC_V3_CONFIG2_NUM_ADDR_PHASE1(x)	(((x) & 0x3) << 12)
#define NFC_V3_CONFIG2_INT_MSK			(1 << 15)
#define NFC_V3_CONFIG2_ST_CMD(x)		(((x) & 0xff) << 24)
#define NFC_V3_CONFIG2_SPAS(x)			(((x) & 0xff) << 16)

#define NFC_V3_CONFIG3				(host->regs_ip + 0x28)
#define NFC_V3_CONFIG3_ADD_OP(x)		(((x) & 0x3) << 0)
#define NFC_V3_CONFIG3_FW8			(1 << 3)
#define NFC_V3_CONFIG3_SBB(x)			(((x) & 0x7) << 8)
#define NFC_V3_CONFIG3_NUM_OF_DEVICES(x)	(((x) & 0x7) << 12)
#define NFC_V3_CONFIG3_RBB_MODE			(1 << 15)
#define NFC_V3_CONFIG3_NO_SDMA			(1 << 20)

#define NFC_V3_IPC			(host->regs_ip + 0x2C)
#define NFC_V3_IPC_CREQ			(1 << 0)
#define NFC_V3_IPC_INT			(1 << 31)

#define NFC_V3_DELAY_LINE		(host->regs_ip + 0x34)

/*
 * Operation modes for the NFC. Valid for v1, v2 and v3
 * type controllers.
 */
#define NFC_CMD				(1 << 0)
#define NFC_ADDR			(1 << 1)
#define NFC_INPUT			(1 << 2)
#define NFC_OUTPUT			(1 << 3)
#define NFC_ID				(1 << 4)
#define NFC_STATUS			(1 << 5)

/*
 * For external NAND boot this defines the magic value for the bad block table
 * This is found at offset ARM_HEAD_SPARE_OFS in the image on NAND.
 */
#define IMX_NAND_BBT_MAGIC 0xbadb10c0

#endif /* __ASM_ARCH_NAND_H */
