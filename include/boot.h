#ifndef __BOOT_H
#define __BOOT_H

#ifdef CONFIG_OF_FLAT_TREE
int do_bootm_linux(struct image_handle *os, struct image_handle *initrd,
	const char *oftree);
#else
int do_bootm_linux(struct image_handle *os, struct image_handle *initrd);
#endif

#endif /* __BOOT_H */

