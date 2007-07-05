#ifndef __BOOT_H
#define __BOOT_H

#ifdef CONFIG_OF_FLAT_TREE
int do_bootm_linux(image_header_t *os, image_header_t *initrd,
	const char *oftree);
#else
int do_bootm_linux(image_header_t *os, image_header_t *initrd);
#endif

#endif /* __BOOT_H */
#ifndef __BOOT_H
#define __BOOT_H

#ifdef CONFIG_OF_FLAT_TREE
int do_bootm_linux(image_header_t *os, image_header_t *initrd,
	const char *oftree);
#else
int do_bootm_linux(image_header_t *os, image_header_t *initrd);
#endif

#endif /* __BOOT_H */
