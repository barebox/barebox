/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/**
 * @file
 * @brief Some common used memory functions
 *
 * Copied from FADS ROM, Dan Malek (dmalek@jlc.net)
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <errno.h>
#include <fs.h>
#include <fcntl.h>
#include <getopt.h>

/** Shared data buffer for all memory commands */
char rw_buf[RW_BUF_SIZE];

/** The default device all memory commands are working on */
char memory_device[] = "/dev/mem";

/**
 * Open the given file and seek to the specified file position
 * @param[in] filename Guess what
 * @param[in] mode Mode of operation (see O_*)
 * @param[in] pos Position to seek to
 * @return Valid filedescriptor on success, or negative value in case of errors
 */
int open_and_lseek(const char *filename, int mode, ulong pos)
{
	int fd, ret;

	fd = open(filename, mode | O_RDONLY);
	if (fd < 0) {
		perror("open");
		return fd;
	}

	ret = lseek(fd, pos, SEEK_SET);
	if (ret < 0) {
		perror("lseek");
		close(fd);
		return ret;
	}

	return fd;
}

/**
 * Parse the options common to all memory related commands
 * @param[in] argc FIXME
 * @param[in] argv FIXME
 * @param[in] optstr FIXME
 * @param[out] mode FIXME
 * @param[out] sourcefile Pointer will be modified if -s param was given
 * @param[out] destfile Pointer will be modified if -d param was given
 * @return 0 on success, negative value in case of errors
 */
int mem_parse_options(int argc, char *argv[], char *optstr, int *mode,
		char **sourcefile, char **destfile)
{
	int opt;

	getopt_reset();

	while((opt = getopt(argc, argv, optstr)) > 0) {
		switch(opt) {
		case 'b':
			*mode = O_RWSIZE_1;
			break;
		case 'w':
			*mode = O_RWSIZE_2;
			break;
		case 'l':
			*mode = O_RWSIZE_4;
			break;
		case 's':
			*sourcefile = optarg;
			break;
		case 'd':
			*destfile = optarg;
			break;
		default:
			return -1;
		}
	}

	return 0;
}

/** The generic memory device to represent the whole physical address space */
static struct device_d mem_dev = {
	.name  = "mem",
	.id    = "mem",
	.map_base = 0,	/**< starts at physical address 0 */
	.size   = ~0,	/**< FIXME: should be 0x100000000, ahem... */
};

static struct driver_d mem_drv = {
	.name  = "mem",
	.probe = dummy_probe,
	.read  = mem_read,
	.write = mem_write,
};

static struct driver_d ram_drv = {
	.name  = "ram",
	.probe = dummy_probe,
	.read  = mem_read,
	.write = mem_write,
	.type  = DEVICE_TYPE_DRAM,
};

static struct driver_d rom_drv = {
	.name  = "rom",
	.probe = dummy_probe,
	.read  = mem_read,
};

static int mem_init(void)
{
	register_device(&mem_dev);
	register_driver(&mem_drv);
	register_driver(&ram_drv);
	register_driver(&rom_drv);
	return 0;
}

device_initcall(mem_init);

/**
 * @page generic_devices Generic devices
 *
 * Most devices in U-Boot-v2 depending on the hardware and this means on the BSP.
 * But some devices are generic for some platforms or architectures or some
 * other are generic for all targets.
 *
 * \b /dev/mem
 *
 * This devices represents the whole physical address space of the running CPU.
 * In the case you want to access some physical address, you can use this device.
 * It starts at the offset 0x0000000 and ends at the highest available physical
 * address of the running CPU.
 *
 * @note All memory commands are working with this device as default.
 */
