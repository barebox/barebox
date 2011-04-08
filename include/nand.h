
#ifndef __NAND_H__
#define __NAND_H__

struct nand_bb;

#ifdef CONFIG_NAND
int dev_add_bb_dev(char *filename, const char *name);
int dev_remove_bb_dev(const char *name);
#else
static inline int dev_add_bb_dev(char *filename, const char *name) {
	return 0;
}
static inline int dev_remove_bb_dev(const char *name)
{
	return 0;
}
#endif

#endif /* __NAND_H__ */

