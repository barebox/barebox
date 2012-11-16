#ifndef __AMBA_PL061_H__
#define __AMBA_PL061_H__

#include <linux/types.h>

/* platform data for the PL061 GPIO driver */

struct pl061_platform_data {
	/* number of the first GPIO */
	unsigned	gpio_base;
};
#endif /* __AMBA_PL061_H__ */
