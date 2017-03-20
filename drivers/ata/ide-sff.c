#include <common.h>
#include <ata_drive.h>
#include <io.h>
#include <errno.h>
#include <clock.h>
#include <disks.h>
#include <malloc.h>

/* max timeout for a rotating disk in [ms] */
#define MAX_TIMEOUT 5000

#define to_ata_drive_access(x) container_of((x), struct ide_port, port)

#define DISK_MASTER 0
#define DISK_SLAVE 1

/**
 * Read a byte from the ATA controller
 * @param ide IDE port structure
 * @param addr Register adress
 * @return Register's content
 */
static inline uint8_t ata_rd_byte(struct ide_port *ide, void __iomem *addr)
{
	if (ide->io.mmio)
		return readb(addr);
	else
		return (uint8_t) inb((int) addr);
}

/**
 * Write a byte to the ATA controller
 * @param ide IDE port structure
 * @param value Value to write
 * @param addr Register adress
 * @return Register's content
 */
static inline void ata_wr_byte(struct ide_port *ide, uint8_t value,
			       void __iomem *addr)
{
	if (ide->io.mmio)
		writeb(value, addr);
	else
		outb(value, (int) addr);
}

/**
 * Read a word from the ATA controller
 * @param ide IDE port structure
 * @param addr Register adress
 * @return Register's content
 */
static inline uint16_t ata_rd_word(struct ide_port *ide,
				   void __iomem *addr)
{
	if (ide->io.mmio)
		return readw(addr);
	else
		return (uint16_t) inw((int) addr);
}

/**
 * Write a word to the ATA controller
 * @param ide IDE port structure
 * @param value Value to write
 * @param addr Register adress
 * @return Register's content
 */
static inline void ata_wr_word(struct ide_port *ide, uint16_t value,
			       void __iomem *addr)
{
	if (ide->io.mmio)
		writew(value, addr);
	else
		outw(value, (int) addr);
}

/**
 * Read the status register of the ATA drive
 * @param io Register file
 * @return Register's content
 */
static uint8_t ata_rd_status(struct ide_port *ide)
{
	return ata_rd_byte(ide, ide->io.status_addr);
}

/**
 * Wait until the disk is busy or time out
 * @param io Register file
 * @param timeout Timeout in [ms]
 * @return 0 on success, -ETIMEDOUT else
 */
static int ata_wait_busy(struct ide_port *ide, unsigned timeout)
{
	uint8_t status;
	uint64_t start = get_time_ns();
	uint64_t toffs = timeout * 1000 * 1000;

	do {
		status = ata_rd_status(ide);
		if (status & ATA_STATUS_BUSY)
			return 0;
	} while (!is_timeout(start, toffs));

	return -ETIMEDOUT;
}

/**
 * Wait until the disk is ready again or time out
 * @param io Register file
 * @param timeout Timeout in [ms]
 * @return 0 on success, -ETIMEDOUT else
 *
 * This function is useful to check if the disk has accepted a command.
 */
static int ata_wait_ready(struct ide_port *ide, unsigned timeout)
{
	uint8_t status;
	uint64_t start = get_time_ns();
	uint64_t toffs = timeout * MSECOND;

	do {
		status = ata_rd_status(ide);
		if (!(status & ATA_STATUS_BUSY)) {
			if (status & ATA_STATUS_READY)
				return 0;
		}
	} while (!is_timeout(start, toffs));

	return -ETIMEDOUT;
}

/**
 * Setup the sector number in LBA notation (LBA28)
 * @param io Register file
 * @param drive 0 master drive, 1 slave drive
 * @param num Sector number
 */
static int ata_set_lba_sector(struct ata_port *port, unsigned drive,
				uint64_t num)
{
	struct ide_port *ide = to_ata_drive_access(port);

	if (drive > 1)
		return -EINVAL;

	if (port->lba48) {
		if (num > (1ULL << 48) - 1)
			return -EINVAL;

		ata_wr_byte(ide, LBA_FLAG | drive << 4, ide->io.device_addr);

		ata_wr_byte(ide, 0x00, ide->io.nsect_addr);
		ata_wr_byte(ide, num >> 24, ide->io.lbal_addr);
		ata_wr_byte(ide, num >> 32, ide->io.lbam_addr);
		ata_wr_byte(ide, num >> 40, ide->io.lbah_addr);
	} else {
		if (num > (1ULL << 28) - 1)
			return -EINVAL;

		ata_wr_byte(ide, 0xA0 | LBA_FLAG | drive << 4 | num >> 24,
			    ide->io.device_addr);
	}

	ata_wr_byte(ide, 0x01, ide->io.nsect_addr);
	ata_wr_byte(ide, num, ide->io.lbal_addr);	/* 0 ... 7 */
	ata_wr_byte(ide, num >> 8, ide->io.lbam_addr); /* 8 ... 15 */
	ata_wr_byte(ide, num >> 16, ide->io.lbah_addr); /* 16 ... 23 */

	return 0;
}

/**
 * Write an ATA command into the disk
 * @param io Register file
 * @param cmd Command to write
 * @return 0 on success
 */
static int ata_wr_cmd(struct ide_port *ide, uint8_t cmd)
{
	int rc;

	rc = ata_wait_ready(ide, MAX_TIMEOUT);
	if (rc != 0)
		return rc;

	ata_wr_byte(ide, cmd, ide->io.command_addr);
	return 0;
}

/**
 * Write a new value into the "device control register"
 * @param io Register file
 * @param val Value to write
 */
static void ata_wr_dev_ctrl(struct ide_port *ide, uint8_t val)
{
	ata_wr_byte(ide, val, ide->io.ctl_addr);
}

/**
 * Read one sector from the drive (always SECTOR_SIZE bytes at once)
 * @param io Register file
 * @param buf Buffer to read the data into
 */
static void ata_rd_sector(struct ide_port *ide, void *buf)
{
	unsigned u = SECTOR_SIZE / sizeof(uint16_t);
	uint16_t *b = buf;

	if (ide->io.dataif_be) {
		for (; u > 0; u--)
			*b++ = be16_to_cpu(ata_rd_word(ide, ide->io.data_addr));
	} else {
		for (; u > 0; u--)
			*b++ = le16_to_cpu(ata_rd_word(ide, ide->io.data_addr));
	}
}

/**
 * Write one sector into the drive
 * @param io Register file
 * @param buf Buffer to read the data from
 */
static void ata_wr_sector(struct ide_port *ide, const void *buf)
{
	unsigned u = SECTOR_SIZE / sizeof(uint16_t);
	const uint16_t *b = buf;

	if (ide->io.dataif_be) {
		for (; u > 0; u--)
			ata_wr_word(ide, cpu_to_be16(*b++), ide->io.data_addr);
	} else {
		for (; u > 0; u--)
			ata_wr_word(ide, cpu_to_le16(*b++), ide->io.data_addr);
	}
}

/**
 * Read the ATA disk's description info
 * @param d All we need to know about the disk
 * @return 0 on success
 */
static int ide_read_id(struct ata_port *port, void *buf)
{
	struct ide_port *ide = to_ata_drive_access(port);
	int rc;

	ata_wr_byte(ide, 0xA0, ide->io.device_addr);	/* FIXME drive */
	ata_wr_byte(ide, 0x00, ide->io.lbal_addr);
	ata_wr_byte(ide, 0x00, ide->io.lbam_addr);
	ata_wr_byte(ide, 0x00, ide->io.lbah_addr);

	rc = ata_wr_cmd(ide, ATA_CMD_ID_ATA);
	if (rc != 0)
		return rc;

	rc = ata_wait_ready(ide, MAX_TIMEOUT);
	if (rc != 0)
		return rc;

	ata_rd_sector(ide, buf);

	return 0;
}

static int ide_reset(struct ata_port *port)
{
	struct ide_port *ide = to_ata_drive_access(port);
	int rc;
	uint8_t reg;

	/* try a hard reset first (if available) */
	if (ide->io.reset != NULL) {
		pr_debug("%s: Resetting drive...\n", __func__);
		ide->io.reset(1);
		rc = ata_wait_busy(ide, 500);
		ide->io.reset(0);
		if (rc == 0) {
			rc = ata_wait_ready(ide, MAX_TIMEOUT);
			if (rc != 0)
				return rc;
		} else {
			pr_debug("%s: Drive does not respond to RESET line. Ignored\n",
					__func__);
		}
	}

	/* try a soft reset */
	ata_wr_dev_ctrl(ide, ATA_DEVCTL_SOFT_RESET | ATA_DEVCTL_INTR_DISABLE);
	rc = ata_wait_busy(ide, MAX_TIMEOUT);	/* does the drive accept the command? */
	if (rc != 0) {
		pr_debug("%s: Drive fails on soft reset\n", __func__);
		return rc;
	}
	ata_wr_dev_ctrl(ide, ATA_DEVCTL_INTR_DISABLE);
	rc = ata_wait_ready(ide, MAX_TIMEOUT);
	if (rc != 0) {
		pr_debug("%s: Drive fails after soft reset\n", __func__);
		return rc;
	}

	reg = ata_rd_status(ide) & 0xf;

	if (reg == 0xf) {
		pr_debug("%s: Seems no drive connected!\n", __func__);
		return -ENODEV;
	}

	return 0;
}

/**
 * Read a chunk of sectors from the drive
 * @param blk All info about the block device we need
 * @param buffer Buffer to read into
 * @param block Sector's LBA number to start read from
 * @param num_blocks Sector count to read
 * @return 0 on success, anything else on failure
 *
 * This routine expects the buffer has the correct size to store all data!
 *
 * @note Due to 'block' is of type 'int' only small disks can be handled!
 * @todo Optimize the read loop
 */
static int ide_read(struct ata_port *port, void *buffer, unsigned int block,
				int num_blocks)
{
	int rc;
	uint64_t sector = block;
	struct ide_port *ide = to_ata_drive_access(port);

	while (num_blocks) {
		uint8_t cmd;

		rc = ata_set_lba_sector(port, DISK_MASTER, sector);
		if (rc != 0)
			return rc;

		if (port->lba48)
			cmd = ATA_CMD_PIO_READ_EXT;
		else
			cmd = ATA_CMD_READ;

		rc = ata_wr_cmd(ide, cmd);
		if (rc != 0)
			return rc;
		rc = ata_wait_ready(ide, MAX_TIMEOUT);
		if (rc != 0)
			return rc;
		ata_rd_sector(ide, buffer);
		num_blocks--;
		sector++;
		buffer += SECTOR_SIZE;
	}

	return 0;
}

/**
 * Write a chunk of sectors into the drive
 * @param blk All info about the block device we need
 * @param buffer Buffer to write from
 * @param block Sector's number to start write to
 * @param num_blocks Sector count to write
 * @return 0 on success, anything else on failure
 *
 * This routine expects the buffer has the correct size to read all data!
 *
 * @note Due to 'block' is of type 'int' only small disks can be handled!
 * @todo Optimize the write loop
 */
static int __maybe_unused ide_write(struct ata_port *port,
				const void *buffer, unsigned int block, int num_blocks)
{
	int rc;
	uint64_t sector = block;
	struct ide_port *ide = to_ata_drive_access(port);

	while (num_blocks) {
		uint8_t cmd;

		rc = ata_set_lba_sector(port, DISK_MASTER, sector);
		if (rc != 0)
			return rc;

		if (port->lba48)
			cmd = ATA_CMD_PIO_WRITE_EXT;
		else
			cmd = ATA_CMD_WRITE;

		rc = ata_wr_cmd(ide, cmd);
		if (rc != 0)
			return rc;
		rc = ata_wait_ready(ide, MAX_TIMEOUT);
		if (rc != 0)
			return rc;
		ata_wr_sector(ide, buffer);
		num_blocks--;
		sector++;
		buffer += SECTOR_SIZE;
	}

	return 0;
}

static struct ata_port_operations ide_ops = {
	.read_id = ide_read_id,
	.read = ide_read,
#ifdef CONFIG_BLOCK_WRITE
	.write = ide_write,
#endif
	.reset = ide_reset,
};

int ide_port_register(struct ide_port *ide)
{
	int ret;

	ide->port.ops = &ide_ops;

	ret = ata_port_register(&ide->port);
	if (ret)
		return ret;

	ata_port_detect(&ide->port);

	return 0;
}
