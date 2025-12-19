// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025 Maud Spierings, GOcontroll B.V.

#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

ssize_t pread(int fd, void *buf, size_t count, off_t offset)
{
	int offset_start, now;

	offset_start = lseek(fd, 0, SEEK_CUR);
	if (offset_start < 0)
		return offset_start;

	now = lseek(fd, offset, SEEK_SET);
	if (now < 0)
		return now;

	now = read(fd, buf, count);

	lseek(fd, offset_start, SEEK_SET);

	return now;
}
