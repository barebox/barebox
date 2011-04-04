#ifndef __NAND_H
#define __NAND_H

#define CONFIG_NAND_WRITE

int nand_read_oob_std(struct mtd_info *mtd, struct nand_chip *chip,
			     int page, int sndcmd);
int nand_write_oob_std(struct mtd_info *mtd, struct nand_chip *chip,
			      int page);

void nand_init_ecc_hw(struct nand_chip *chip);
void nand_init_ecc_soft(struct nand_chip *chip);

#endif /* __NAND_H */
