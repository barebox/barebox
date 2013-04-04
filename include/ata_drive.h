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
#define ATA_CMD_READ_EXT	0x25
#define ATA_CMD_WRITE		0x30
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
	void *drvdata;
	struct block_device blk;
	uint16_t *id;
	int initialized;
};

int ide_port_register(struct device_d *, struct ata_ioports *);
int ata_port_register(struct ata_port *port);

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
