#ifndef __BOOT_H
#define __BOOT_H

#include <image.h>

#ifdef CONFIG_OF_FLAT_TREE
extern int do_bootm_linux(struct image_handle *os, struct image_handle *initrd,
	const char *oftree);
#else
extern int do_bootm_linux(struct image_handle *os, struct image_handle *initrd);
#endif

#endif /* __BOOT_H */

