#ifndef __NAND_H
#define __NAND_H

int nand_read_oob_std(struct mtd_info *mtd, struct nand_chip *chip,
			     int page, int sndcmd);
int nand_write_oob_std(struct mtd_info *mtd, struct nand_chip *chip,
			      int page);
int nand_default_block_markbad(struct mtd_info *mtd, loff_t ofs);
int nand_block_checkbad(struct mtd_info *mtd, loff_t ofs, int getchip,
			       int allowbbt);
int nand_block_isbad(struct mtd_info *mtd, loff_t offs);
int nand_block_markbad(struct mtd_info *mtd, loff_t ofs);
void nand_write_buf(struct mtd_info *mtd, const uint8_t *buf, int len);
void nand_write_buf16(struct mtd_info *mtd, const uint8_t *buf, int len);
void single_erase_cmd(struct mtd_info *mtd, int page);
void multi_erase_cmd(struct mtd_info *mtd, int page);
void nand_write_page_raw(struct mtd_info *mtd, struct nand_chip *chip,
				const uint8_t *buf);
int nand_write_page(struct mtd_info *mtd, struct nand_chip *chip,
		uint32_t offset, int data_len, const uint8_t *buf,
		int oob_required, int page, int cached, int raw);
int nand_erase(struct mtd_info *mtd, struct erase_info *instr);
int nand_write(struct mtd_info *mtd, loff_t to, size_t len,
			  size_t *retlen, const uint8_t *buf);
int nand_write_oob(struct mtd_info *mtd, loff_t to,
			  struct mtd_oob_ops *ops);

void nand_init_ecc_hw(struct nand_chip *chip);
void nand_init_ecc_soft(struct nand_chip *chip);
void nand_init_ecc_hw_syndrome(struct nand_chip *chip);

#endif /* __NAND_H */
