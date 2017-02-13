/*
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
 * Inspired by: https://github.com/simu/usbboot-omap4.git
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <libusb.h>
#include <pthread.h>
#include <termios.h>

#define USBBOOT_FS_MAGIC     0x5562464D
#define USBBOOT_FS_CMD_OPEN  0x46530000
#define USBBOOT_FS_CMD_CLOSE 0x46530001
#define USBBOOT_FS_CMD_READ  0x46530002
#define USBBOOT_FS_CMD_END   0x4653FFFF
#define MAX_OPEN_FILES       128

#define RESET   0
#define BRIGHT  1
#define WHITE   8
#define RED     1
#define BLACK   0
#define TFORMAT       "%c[%d;%dm"
#define HFORMAT       "%c[%dm"
#define TARGET_FORMAT 0x1B, BRIGHT, RED+30
#define HOST_FORMAT   0x1B, RESET
#define host_print(fmt, arg...)	printf(HFORMAT fmt TFORMAT, \
					HOST_FORMAT, ##arg, TARGET_FORMAT)

int usb_write(void *h, void const *data, int len)
{
	int actual;
	return libusb_bulk_transfer(h, 0x01, (void *)data, len, &actual, 5000) ?
		0 : actual;
}

int usb_read(void *h, void *data, int len)
{
	int actual;
	return libusb_bulk_transfer(h, 0x81, data, len, &actual, 5000) ?
		0 : actual;
}

void panic(struct termios *t_restore)
{
	tcsetattr(STDIN_FILENO, TCSANOW, t_restore);
	printf(HFORMAT, HOST_FORMAT);
	exit(1);
}

struct thread_vars {
	struct libusb_device_handle *usb;
	pthread_mutex_t usb_mutex;
	struct termios t_restore;
};

void *listenerTask(void *argument)
{
	struct thread_vars *vars = argument;
	int c;
	for (;;) {
		c = getchar();
		if (c == EOF)
			return NULL;
		pthread_mutex_lock(&vars->usb_mutex);
		if (usb_write(vars->usb, &c, 4) != 4) {
			host_print("could not send '%c' to target\n", c);
			panic(&vars->t_restore);
		}
		pthread_mutex_unlock(&vars->usb_mutex);
	}
	return NULL;
}

int read_asic_id(struct libusb_device_handle *usb)
{
#define LINEWIDTH 16
	const uint32_t msg_getid = 0xF0030003;
	int i, j, k, ret;
	uint8_t id[81];
	char line[LINEWIDTH*3+5];

	printf("reading ASIC ID\n");
	memset(id , 0xee, sizeof(id));
	if (usb_write(usb, &msg_getid, sizeof(msg_getid)) !=
		sizeof(msg_getid)
	) {
		printf("Could not send msg_getid request\n");
		return -1;
	}
	if (usb_read(usb, id, sizeof(id)) != sizeof(id)) {
		printf("Could not read msg_getid answer\n");
		return -1;
	}
	for (i = 0; i < sizeof(id); i += LINEWIDTH) {
		sprintf(line, "%02X: ", i);
		for (j = 0; j < LINEWIDTH && j < sizeof(id)-i; j++)
			sprintf(line+4+j*3, "%02X ", id[i+j]);
		line[4+j*3] = 0;
		puts(line);
	}
	ret = 0;
	for (i = 1, j = 0; i < sizeof(id) && j < id[0]; i += 2+id[i+1], j++) {
		if (i+2+id[i+1] > sizeof(id)) {
			printf("Truncated subblock\n");
			ret++;
			continue;
		}
		switch (id[i]) {
		case 0x01: /* ID subblock */
			if (id[i+1] != 0x05) {
				printf("Unexpected ID subblock size\n");
				ret++;
				continue;
			}
			if (id[i+2] != 0x01)
				printf("Unexpected fixed value\n");
			k = (id[i+3]<<8) | id[i+4];
			switch (k) {
			case 0x4440:
				printf("OMAP 4460 Device\n");
				break;
			default:
				printf("Unknown Device\n");
				break;
			}
			switch (id[i+5]) {
			case 0x07:
				printf("CH enabled (read from eFuse)\n");
				break;
			case 0x17:
				printf("CH disabled (read from eFuse)\n");
				break;
			default:
				printf("Unknown CH setting\n");
				break;
			}
			printf("Rom version: %hhu\n", id[i+6]);
			break;
		case 0x15: /* Checksum subblock */
			if (id[i+1] != 0x09) {
				printf("Unexpected Checksum subblock size\n");
				ret++;
				continue;
			}
			if (id[i+2] != 0x01)
				printf("Unexpected fixed value\n");
			k = (id[i+3]<<24) | (id[i+4]<<16) |
				(id[i+5]<<8) | id[i+6];
			printf("Rom CRC: 0x%08X\n", k);
			k = (id[i+7]<<24) | (id[i+8]<<16) |
				(id[i+9]<<8) | id[i+10];
			switch (k) {
			case  0:
				printf("A GP device\n");
				break;
			default:
				printf("Unknown device\n");
				break;
			}
			break;
		}
	}
	if (i != sizeof(id) || j != id[0]) {
		printf("Unexpected ASIC ID structure size.\n");
		ret++;
	}
	return ret;
}

struct file_data {
	size_t size;
	void *data;
};

int process_file(struct libusb_device_handle *usb, const char *rootfs,
	struct file_data *fd_vector, struct termios *t_restore)
{
	uint32_t i, j, pos, size;
	struct stat s;
	int fd, ret;
	char fname[256];

	if (usb_read(usb, &i, 4) != 4) {
		host_print("USB error\n");
		panic(t_restore);
	}
	ret = 0;
	switch (i) {
	case USBBOOT_FS_CMD_OPEN:
		for (j = 0; rootfs[j]; j++)
			fname[j] = rootfs[j];
		for (;; j++) {
			if (usb_read(usb, &i, 4) != 4) {
				host_print("USB error\n");
				panic(t_restore);
			}
			if (i == USBBOOT_FS_CMD_END) {
				fname[j] = 0;
				break;
			} else if (i > 0xFF) {
				host_print("Error in filename\n");
				ret++;
				fname[j] = 0;
				break;
			} else
				fname[j] = i;
		}
		for (i = 0; i < MAX_OPEN_FILES && fd_vector[i].data; i++)
			;
		if (i >= MAX_OPEN_FILES) {
			host_print("MAX_OPEN_FILES exceeded\n");
			ret++;
			goto open_error_1;
		}
		fd = open(fname, O_RDONLY);
		if (fd < 0) {
			host_print("cannot open '%s'\n", fname);
			ret++;
			goto open_error_1;
		}
		if (fstat(fd, &s)) {
			host_print("cannot stat '%s'\n", fname);
			ret++;
			goto open_error_2;
		}
		fd_vector[i].data = mmap(NULL, s.st_size, PROT_READ,
			MAP_PRIVATE, fd, 0);
		if (fd_vector[i].data == MAP_FAILED) {
			host_print("cannot mmap '%s'\n", fname);
			ret++;
			goto open_error_2;
		}
		close(fd);
		fd_vector[i].size = size = s.st_size;
		fd = i;
		goto open_ok;

open_error_2:
		close(fd);
open_error_1:
		fd_vector[i].size = size = 0;
		fd_vector[i].data = NULL;
		fd = -1;
open_ok:
		if (usb_write(usb, &fd, 4) != 4 ||
			usb_write(usb, &size, 4) != 4
		) {
			host_print("could not send file size to target\n");
			panic(t_restore);
		}
		break;
	case USBBOOT_FS_CMD_CLOSE:
		if (usb_read(usb, &i, 4) != 4) {
			host_print("USB error\n");
			panic(t_restore);
		}
		if (i >= MAX_OPEN_FILES || !fd_vector[i].data) {
			host_print("invalid close index\n");
			ret++;
			break;
		}
		if (usb_read(usb, &j, 4) != 4) {
			host_print("USB error\n");
			panic(t_restore);
		}
		if (j != USBBOOT_FS_CMD_END) {
			host_print("invalid close\n");
			ret++;
			break;
		}
		munmap(fd_vector[i].data, fd_vector[i].size);
		fd_vector[i].data = NULL;
		break;
	case USBBOOT_FS_CMD_READ:
		if (usb_read(usb, &i, 4) != 4) {
			host_print("USB error\n");
			panic(t_restore);
		}
		if (i >= MAX_OPEN_FILES || !fd_vector[i].data) {
			host_print("invalid read index\n");
			ret++;
			break;
		}
		if (usb_read(usb, &pos, 4) != 4) {
			host_print("USB error\n");
			panic(t_restore);
		}
		if (pos >= fd_vector[i].size) {
			host_print("invalid read pos\n");
			ret++;
			break;
		}
		if (usb_read(usb, &size, 4) != 4) {
			host_print("USB error\n");
			panic(t_restore);
		}
		if (pos+size > fd_vector[i].size) {
			host_print("invalid read size\n");
			ret++;
			break;
		}
		if (usb_read(usb, &j, 4) != 4) {
			host_print("USB error\n");
			panic(t_restore);
		}
		if (j != USBBOOT_FS_CMD_END) {
			host_print("invalid read\n");
			ret++;
			break;
		}
		if (usb_write(usb, fd_vector[i].data+pos, size) != size) {
			host_print("could not send file to target\n");
			panic(t_restore);
		}
		break;
	case USBBOOT_FS_CMD_END:
	default:
		host_print("Unknown filesystem command\n");
		ret++;
		break;
	}
	return ret;
}

int usb_boot(struct libusb_device_handle *usb,
	void *data, unsigned sz, const char *rootfs)
{
	const uint32_t msg_boot  = 0xF0030002;
	uint32_t msg_size = sz;
	int i;
	pthread_t thread;
	struct thread_vars vars;
	struct termios tn;
	struct file_data fd_vector[MAX_OPEN_FILES];

	if (read_asic_id(usb))
		return -1;

	printf("sending xload to target...\n");
	usleep(1000);
	usb_write(usb, &msg_boot, sizeof(msg_boot));
	usleep(1000);
	usb_write(usb, &msg_size, sizeof(msg_size));
	usleep(1000);
	usb_write(usb, data, sz);
	usleep(100000);
	munmap(data, msg_size);
	for (i = 0; i < MAX_OPEN_FILES; i++)
		fd_vector[i].data = NULL;

	vars.usb = usb;
	pthread_mutex_init(&vars.usb_mutex, NULL);
	tcgetattr(STDIN_FILENO, &vars.t_restore);
	tn = vars.t_restore;
	tn.c_lflag &= ~(ICANON | ECHO);
	printf(TFORMAT, TARGET_FORMAT);
	tcsetattr(STDIN_FILENO, TCSANOW, &tn);
	if (pthread_create(&thread, NULL, listenerTask, &vars))
		host_print("listenerTask failed\n");
	for (;;) {
		usleep(100);
		if (usb_read(usb, &i, 4) != 4)
			break;
		if (i == USBBOOT_FS_MAGIC) {
			usleep(100);
			pthread_mutex_lock(&vars.usb_mutex);
			process_file(usb, rootfs, fd_vector, &vars.t_restore);
			pthread_mutex_unlock(&vars.usb_mutex);
			continue;
		}
		printf("%c", i);
		fflush(stdout);
	}
	pthread_mutex_destroy(&vars.usb_mutex);
	tcsetattr(STDIN_FILENO, TCSANOW, &vars.t_restore);
	printf(HFORMAT, HOST_FORMAT);
	return 0;
}

int main(int argc, char **argv)
{
	void *data;
	unsigned sz;
	struct stat s;
	int fd;
	int ret;
	struct libusb_context       *ctx = NULL;
	struct libusb_device_handle *usb = NULL;

	if (argc != 3) {
		printf("usage: %s <xloader> <rootfs>\n", argv[0]);
		return 0;
	}
	argv++;
	fd = open(argv[0], O_RDONLY);
	if (fd < 0 || fstat(fd, &s)) {
		printf("cannot open '%s'\n", argv[0]);
		return -1;
	}
	data = mmap(NULL, s.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (data == MAP_FAILED) {
		printf("cannot mmap '%s'\n", argv[0]);
		return -1;
	}
	sz = s.st_size;
	close(fd);
	argv++;
	if (libusb_init(&ctx)) {
		printf("cannot initialize libusb\n");
		return -1;
	}
	printf(HFORMAT, HOST_FORMAT);
	printf("waiting for OMAP44xx device...\n");
	while (1) {
		if (!usb)
			usb = libusb_open_device_with_vid_pid(
				ctx, 0x0451, 0xD010);
		if (!usb)
			usb = libusb_open_device_with_vid_pid(
				ctx, 0x0451, 0xD00F);
		if (usb) {
			libusb_detach_kernel_driver(usb, 0);
			ret = libusb_set_configuration(usb, 1);
			if (ret)
				break;
			ret = libusb_claim_interface(usb, 0);
			if (ret)
				break;
			ret = usb_boot(usb, data, sz, argv[0]);
			break;
		}
		usleep(250000);
	}
	libusb_release_interface(usb, 0);
	libusb_close(usb);
	libusb_exit(ctx);
	return ret;
}
