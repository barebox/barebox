#ifndef _LINUX_TIME_H
#define _LINUX_TIME_H

#include <linux/types.h>

#define NSEC_PER_SEC	1000000000L

struct timespec {
	time_t	tv_sec;		/* seconds */
	long	tv_nsec;	/* nanoseconds */
};

#endif
