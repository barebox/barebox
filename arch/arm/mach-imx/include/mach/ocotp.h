#ifndef __MACH_IMX_OCOTP_H
#define __MACH_IMX_OCOTP_H

#define OCOTP_WORD_MASK_WIDTH	8
#define OCOTP_WORD_MASK_SHIFT	0
#define OCOTP_WORD(n)		((((n) - 0x400) >> 4) & ((1 << OCOTP_WORD_MASK_WIDTH) - 1))

#define OCOTP_BIT_MASK_WIDTH	5
#define OCOTP_BIT_MASK_SHIFT	(OCOTP_WORD_MASK_SHIFT + OCOTP_WORD_MASK_WIDTH)
#define OCOTP_BIT(n)		(((n) & ((1 << OCOTP_BIT_MASK_WIDTH) - 1)) << OCOTP_BIT_MASK_SHIFT)

#define OCOTP_WIDTH_MASK_WIDTH	5
#define OCOTP_WIDTH_MASK_SHIFT	(OCOTP_BIT_MASK_SHIFT + OCOTP_BIT_MASK_WIDTH)
#define OCOTP_WIDTH(n)		((((n) - 1) & ((1 << OCOTP_WIDTH_MASK_WIDTH) - 1)) << OCOTP_WIDTH_MASK_SHIFT)

int imx_ocotp_read_field(uint32_t field, unsigned *value);
int imx_ocotp_write_field(uint32_t field, unsigned value);
int imx_ocotp_permanent_write(int enable);
bool imx_ocotp_sense_enable(bool enable);

#endif /* __MACH_IMX_OCOTP_H */
