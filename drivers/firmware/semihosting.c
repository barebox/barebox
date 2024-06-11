// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2015 Zodiac Inflight Innovations
// SPDX-FileCopyrightText: 2014 Broadcom Corporation

/*
 * semihosting.c -- ARM Semihoting API implementation
 *
 * Author: Andrey Smirnov <andrew.smirnov@gmail.com>
 * based on a smiliar code from U-Boot
 */

#include <common.h>
#include <command.h>
#include <fcntl.h>
#include <asm/semihosting.h>

#ifndef O_BINARY
#define O_BINARY	0
#endif


enum {
	SEMIHOSTING_SYS_OPEN	= 0x01,
	SEMIHOSTING_SYS_CLOSE	= 0x02,
	SEMIHOSTING_SYS_WRITEC	= 0x03,
	SEMIHOSTING_SYS_WRITE0	= 0x04,
	SEMIHOSTING_SYS_WRITE	= 0x05,
	SEMIHOSTING_SYS_READ	= 0x06,
	SEMIHOSTING_SYS_READC	= 0x07,
	/* SYS_ISERROR is not implemented  */
	SEMIHOSTING_SYS_ISATTY	= 0x09,
	SEMIHOSTING_SYS_SEEK	= 0x0a,
	SEMIHOSTING_SYS_FLEN	= 0x0c,
	SEMIHOSTING_SYS_REMOVE	= 0x0e,
	SEMIHOSTING_SYS_RENAME	= 0x0f,
	SEMIHOSTING_SYS_TIME	= 0x11,
	SEMIHOSTING_SYS_ERRNO	= 0x13,
	/* SYS_GET_CMDLINE is not implemented */
	/* SYS_HEAPINFO is not implemented */
	/* angel_SWIreason_ReportException is not implemented */
	SEMIHOSTING_SYS_SYSTEM	= 0x12,
};

long semihosting_trap(ulong sysnum, void *addr);

static ulong semihosting_flags_to_mode(int flags)
{
	static const int semihosting_open_modeflags[12] = {
		O_RDONLY,
		O_RDONLY | O_BINARY,
		O_RDWR,
		O_RDWR | O_BINARY,
		O_WRONLY | O_CREAT | O_TRUNC,
		O_WRONLY | O_CREAT | O_TRUNC | O_BINARY,
		O_RDWR | O_CREAT | O_TRUNC,
		O_RDWR | O_CREAT | O_TRUNC | O_BINARY,
		O_WRONLY | O_CREAT | O_APPEND,
		O_WRONLY | O_CREAT | O_APPEND | O_BINARY,
		O_RDWR | O_CREAT | O_APPEND,
		O_RDWR | O_CREAT | O_APPEND | O_BINARY
	};

	int i;
	for (i = 0; i < ARRAY_SIZE(semihosting_open_modeflags); i++) {
		if (semihosting_open_modeflags[i] == flags)
			return i;
	}

	return 0;
}

int semihosting_open(const char *fname, int flags)
{
	struct __packed {
		ulong fname;
		ulong mode;
		ulong len;
	} open = {
		.fname = virt_to_phys(fname),
		.len = strlen(fname),
		.mode = semihosting_flags_to_mode(flags),
	};

	return semihosting_trap(SEMIHOSTING_SYS_OPEN, &open);
}
EXPORT_SYMBOL(semihosting_open);

int semihosting_close(int fd)
{
	return semihosting_trap(SEMIHOSTING_SYS_CLOSE, &fd);
}
EXPORT_SYMBOL(semihosting_close);

int semihosting_writec(char c)
{
	return semihosting_trap(SEMIHOSTING_SYS_WRITEC, &c);
}
EXPORT_SYMBOL(semihosting_writec);

int semihosting_write0(const char *str)
{
	return semihosting_trap(SEMIHOSTING_SYS_WRITE0, (void *)str);
}
EXPORT_SYMBOL(semihosting_write0);

struct __packed semihosting_file_io {
	ulong fd;
	ulong memp;
	ulong len;
};

ssize_t semihosting_write(int fd, const void *buf, size_t count)
{
	struct semihosting_file_io write = {
		.fd = fd,
		.memp = virt_to_phys(buf),
		.len = count,
	};

	return semihosting_trap(SEMIHOSTING_SYS_WRITE, &write);
}
EXPORT_SYMBOL(semihosting_write);

ssize_t semihosting_read(int fd, void *buf, size_t count)
{
	struct semihosting_file_io read = {
		.fd = fd,
		.memp = virt_to_phys(buf),
		.len = count,
	};

	return semihosting_trap(SEMIHOSTING_SYS_READ, &read);
}
EXPORT_SYMBOL(semihosting_read);

int semihosting_readc(void)
{
	return semihosting_trap(SEMIHOSTING_SYS_READC, NULL);
}
EXPORT_SYMBOL(semihosting_readc);

int semihosting_isatty(int fd)
{
	return semihosting_trap(SEMIHOSTING_SYS_ISATTY, &fd);
}
EXPORT_SYMBOL(semihosting_isatty);

off_t semihosting_seek(int fd, off_t pos)
{
	struct __packed {
		ulong fd;
		ulong pos;
	} seek = {
		.fd = fd,
		.pos = pos,
	};

	return semihosting_trap(SEMIHOSTING_SYS_SEEK, &seek);
}
EXPORT_SYMBOL(semihosting_seek);
off_t semihosting_flen(int fd)
{
	return semihosting_trap(SEMIHOSTING_SYS_FLEN, &fd);
}
EXPORT_SYMBOL(semihosting_flen);

int semihosting_remove(const char *fname)
{
	struct __packed {
		ulong fname;
		ulong fname_length;
	} remove = {
		.fname = virt_to_phys(fname),
		.fname_length = strlen(fname),
	};

	return semihosting_trap(SEMIHOSTING_SYS_REMOVE, &remove);
}
EXPORT_SYMBOL(semihosting_remove);

int semihosting_rename(const char *fname1, const char *fname2)
{
	struct __packed {
		ulong fname1;
		ulong fname1_length;
		ulong fname2;
		ulong fname2_length;
	} rename = {
		.fname1 = virt_to_phys(fname1),
		.fname1_length = strlen(fname1),
		.fname2 = virt_to_phys(fname2),
		.fname2_length = strlen(fname2),
	};

	return semihosting_trap(SEMIHOSTING_SYS_RENAME, &rename);
}
EXPORT_SYMBOL(semihosting_rename);

int semihosting_errno(void)
{
	return semihosting_trap(SEMIHOSTING_SYS_ERRNO, NULL);
}
EXPORT_SYMBOL(semihosting_errno);


int semihosting_system(const char *command)
{
	struct __packed {
		ulong cmd;
		ulong cmd_len;
	} system = {
		.cmd = virt_to_phys(command),
		.cmd_len = strlen(command),
	};

	return semihosting_trap(SEMIHOSTING_SYS_SYSTEM, &system);
}
EXPORT_SYMBOL(semihosting_system);
