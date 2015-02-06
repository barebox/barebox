#ifndef __NAND_H__
#define __NAND_H__

struct nand_bb;

#ifdef CONFIG_NAND
int dev_add_bb_dev(const char *filename, const char *name);
int dev_remove_bb_dev(const char *name);
struct cdev *mtd_add_bb(struct mtd_info *mtd, const char *name);
void mtd_del_bb(struct mtd_info *mtd);
#else
static inline int dev_add_bb_dev(const char *filename, const char *name) {
	return 0;
}
static inline int dev_remove_bb_dev(const char *name)
{
	return 0;
}

static inline struct cdev *mtd_add_bb(struct mtd_info *mtd, const char *name)
{
	return NULL;
}

static inline void mtd_del_bb(struct mtd_info *mtd)
{
}
#endif

#endif /* __NAND_H__ */
