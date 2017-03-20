/*
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
 */

#ifndef ATA_DISK_H
# define ATA_DISK

#include <block.h>

/* IDE register file */
#define IDE_REG_DATA 0x00
#define IDE_REG_ERR 0x01
#define IDE_REG_NSECT 0x02
#define IDE_REG_LBAL 0x03
#define IDE_REG_LBAM 0x04
#define IDE_REG_LBAH 0x05
#define IDE_REG_DEVICE 0x06
#define IDE_REG_STATUS 0x07

#define IDE_REG_FEATURE IDE_REG_ERR /* and their aliases */
#define IDE_REG_CMD IDE_REG_STATUS

#define IDE_REG_ALT_STATUS 0x00
#define IDE_REG_DEV_CTL 0x00
#define IDE_REG_DRV_ADDR 0x01

#define ATA_CMD_ID_ATA		0xEC
#define ATA_CMD_READ		0x20
#define ATA_CMD_PIO_READ_EXT	0x24
#define ATA_CMD_READ_EXT	0x25
#define ATA_CMD_WRITE		0x30
#define ATA_CMD_PIO_WRITE_EXT	0x34
#define ATA_CMD_WRITE_EXT	0x35

/* drive's status flags */
#define ATA_STATUS_BUSY		(1 << 7)
#define ATA_STATUS_READY	(1 << 6)
#define ATA_STATUS_WR_FLT	(1 << 5)
#define ATA_STATUS_DSC		(1 << 4)
#define ATA_STATUS_DRQ		(1 << 3)
#define ATA_STATUS_CORR		(1 << 2)
#define ATA_STATUS_IDX		(1 << 1)
#define ATA_STATUS_ERROR	(1 << 0)
/* command flags */
#define LBA_FLAG		(1 << 6)
#define ATA_DEVCTL_SOFT_RESET	(1 << 2)
#define ATA_DEVCTL_INTR_DISABLE	(1 << 1)

#define ata_id_u32(id,n)        \
        (((uint32_t) (id)[(n) + 1] << 16) | ((uint32_t) (id)[(n)]))
#define ata_id_u64(id,n)        \
        ( ((uint64_t) (id)[(n) + 3] << 48) | \
          ((uint64_t) (id)[(n) + 2] << 32) | \
          ((uint64_t) (id)[(n) + 1] << 16) | \
          ((uint64_t) (id)[(n) + 0]) )

#define ata_id_has_lba(id)               ((id)[49] & (1 << 9))

enum {
	ATA_ID_SERNO		= 10,
#define ATA_ID_SERNO_LEN 20
	ATA_ID_FW_REV		= 23,
#define ATA_ID_FW_REV_LEN 8
	ATA_ID_PROD		= 27,
#define ATA_ID_PROD_LEN 40
	ATA_ID_CAPABILITY	= 49,
	ATA_ID_FIELD_VALID	= 53,
	ATA_ID_LBA_CAPACITY	= 60,
	ATA_ID_MWDMA_MODES	= 63,
	ATA_ID_PIO_MODES	= 64,
	ATA_ID_QUEUE_DEPTH	= 75,
	ATA_ID_MAJOR_VER	= 80,
	ATA_ID_COMMAND_SET_1	= 82,
	ATA_ID_COMMAND_SET_2	= 83,
	ATA_ID_CFSSE		= 84,
	ATA_ID_CFS_ENABLE_1	= 85,
	ATA_ID_CFS_ENABLE_2	= 86,
	ATA_ID_CSF_DEFAULT	= 87,
	ATA_ID_UDMA_MODES	= 88,
	ATA_ID_HW_CONFIG	= 93,
	ATA_ID_LBA_CAPACITY_2	= 100,
};

static inline int ata_id_has_lba48(const uint16_t *id)
{
	if ((id[ATA_ID_COMMAND_SET_2] & 0xC000) != 0x4000)
		return 0;
	if (!ata_id_u64(id, ATA_ID_LBA_CAPACITY_2))
		return 0;
	return id[ATA_ID_COMMAND_SET_2] & (1 << 10);
}

/** addresses of each individual IDE drive register */
struct ata_ioports {
	void __iomem *cmd_addr;
	void __iomem *data_addr;
	void __iomem *error_addr;
	void __iomem *feature_addr;
	void __iomem *nsect_addr;
	void __iomem *lbal_addr;
	void __iomem *lbam_addr;
	void __iomem *lbah_addr;
	void __iomem *device_addr;
	void __iomem *status_addr;
	void __iomem *command_addr;
	void __iomem *altstatus_addr;
	void __iomem *ctl_addr;
	void __iomem *alt_dev_addr;

	/* hard reset line handling */
	void (*reset)(int);	/* true: assert reset, false: de-assert reset */
	int dataif_be;	/* true if 16 bit data register is big endian */
	int mmio; /* true if memory-mapped io */
};

struct ata_port;

struct ata_port_operations {
	int (*init)(struct ata_port *port);
	int (*read)(struct ata_port *port, void *buf, unsigned int block, int num_blocks);
	int (*write)(struct ata_port *port, const void *buf, unsigned int block, int num_blocks);
	int (*read_id)(struct ata_port *port, void *buf);
	int (*reset)(struct ata_port *port);
};

struct ata_port {
	struct ata_port_operations *ops;
	struct device_d *dev;
	struct device_d class_dev;
	const char *devname;
	void *drvdata;
	struct block_device blk;
	uint16_t *id;
	int lba48;
	int initialized;
	int probe;
};

struct ide_port {
	struct ata_ioports io;	/**< register file */
	struct ata_port port;
};

int ide_port_register(struct ide_port *ide);
int ata_port_register(struct ata_port *port);
int ata_port_detect(struct ata_port *port);

struct device_d;

/**
 * @file
 * @brief Register file examples of generic types of ATA devices
 *
 * PC IDE:
 *
 *        Offset       Read      Write           Note
 *-----------------------------------------------------------
 *        0x1f0        data      data        16 bit register
 *        0x1f1        error    feature
 *        0x1f2       sec cnt   set cnt
 *        0x1f3       sec no    sec no
 *        0x1f4       cyl low   cyl low
 *        0x1f5       cyl high  cyl high
 *        0x1f6        head      head
 *        0x1f7       status    command
 *        0x3f6     alt status  dev cntrl
 *        0x3f7       drv addr
 *
 * PCMCIA memory mapped:
 *
 *        Offset       Read      Write           Note
 *-----------------------------------------------------------
 *        0x0          data      data        16 bit register
 *        0x1          error    feature
 *        0x2         sec cnt   set cnt
 *        0x3         sec no    sec no
 *        0x4         cyl low   cyl low
 *        0x5         cyl high  cyl high
 *        0x6          head      head
 *        0x7         status    command
 *        0x8          data      data       16 bit or 8 bit register (even byte)
 *        0x9          data      data       8 bit register (odd byte)
 *        0xd          error    feature     dup of offset 1
 *        0xe       alt status  dev cntrl
 *        0xf        drv addr
 *       0x400         data      data       16 bit area with 1 kiB in size
 */

#endif /* ATA_DISK */
