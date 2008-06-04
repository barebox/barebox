
#ifndef __NAND_H__
#define __NAND_H__

struct nand_chip;

struct nand_platform_data {
	void            (*hwcontrol)(struct nand_chip *, int cmd);
	int		eccmode;
	int             (*dev_ready)(struct nand_chip *);
	int		chip_delay;
};

#endif /* __NAND_H__ */

