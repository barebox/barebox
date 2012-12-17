/*
 * mtd/utils.h - helper functions for various MTD utilities
 *
 * Copyright (C) 2012 by Wolfram Sang <w.sang@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef INCLUDE_MTD_UTILS_H
# define INCLUDE_MTD_UTILS_H

/* Messages as used in mtd-utils */

#define bareverbose(verbose, fmt, ...) do {                        \
	if (verbose)                                               \
		printf(fmt, ##__VA_ARGS__);                        \
} while(0)
#define verbose(verbose, fmt, ...) \
	bareverbose(verbose, "%s: " fmt "\n", PROGRAM_NAME, ##__VA_ARGS__)

#define normsg_cont(fmt, ...) do {                                 \
	printf("%s: " fmt, PROGRAM_NAME, ##__VA_ARGS__);           \
} while(0)

#define normsg(fmt, ...) do {                                      \
	normsg_cont(fmt "\n", ##__VA_ARGS__);                      \
} while(0)

#define errmsg(fmt, ...)  ({                                                \
	printf("%s: error!: " fmt "\n", PROGRAM_NAME, ##__VA_ARGS__); \
	-1;                                                                 \
})
#define sys_errmsg errmsg

#define warnmsg(fmt, ...) do {                                                \
	fprintf(stderr, "%s: warning!: " fmt "\n", PROGRAM_NAME, ##__VA_ARGS__); \
} while(0)

#endif /* INCLUDE_MTD_UTILS_H */
