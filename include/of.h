#ifndef __OF_H
#define __OF_H

#include <fdt.h>

extern struct fdt_header *barebox_fdt;

int fdt_print(struct fdt_header *working_fdt, const char *pathp);

struct fdt_header *of_get_fixed_tree(void);
int of_register_fixup(int (*fixup)(struct fdt_header *));

int fdt_find_and_setprop(struct fdt_header *fdt, const char *node, const char *prop,
			 const void *val, int len, int create);
void do_fixup_by_path(struct fdt_header *fdt, const char *path, const char *prop,
		      const void *val, int len, int create);
void do_fixup_by_path_u32(struct fdt_header *fdt, const char *path, const char *prop,
			  u32 val, int create);
int fdt_get_path_or_create(struct fdt_header *fdt, const char *path);

#endif /* __OF_H */
