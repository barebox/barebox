/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_TIME64_H
#define __LINUX_TIME64_H

#define PSEC_PER_NSEC	1000L

/* Parameters used to convert the timespec values: */
#define MSEC_PER_SEC	1000L
#define USEC_PER_MSEC	1000L
#define NSEC_PER_USEC	1000L
#define NSEC_PER_MSEC	1000000L
#define USEC_PER_SEC	1000000L
#define NSEC_PER_SEC	1000000000L
#define PSEC_PER_SEC	1000000000000LL
#define FSEC_PER_SEC	1000000000000000LL

#endif /* __LINUX_TIME64_H */
