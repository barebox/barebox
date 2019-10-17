#ifndef __MACH_IMX_OCOTP_H
#define __MACH_IMX_OCOTP_H

#include <linux/bitfield.h>
#include <linux/types.h>

#define OCOTP_SHADOW_OFFSET		0x400
#define OCOTP_SHADOW_SPACING		0x10

/*
 * Trivial shadow register offset -> ocotp register index.
 *
 * NOTE: Doesn't handle special mapping quirks. See
 * imx6q_addr_to_offset and vf610_addr_to_offset for more details. Use
 * with care
 */
#define OCOTP_OFFSET_TO_INDEX(o)		\
	(((o) - OCOTP_SHADOW_OFFSET) / OCOTP_SHADOW_SPACING)

#define OCOTP_WORD_MASK		GENMASK( 7,  0)
#define OCOTP_BIT_MASK		GENMASK(12,  8)
#define OCOTP_WIDTH_MASK 	GENMASK(17, 13)

#define OCOTP_WORD(n)		FIELD_PREP(OCOTP_WORD_MASK, \
					   OCOTP_OFFSET_TO_INDEX(n))
#define OCOTP_BIT(n)		FIELD_PREP(OCOTP_BIT_MASK, n)
#define OCOTP_WIDTH(n)		FIELD_PREP(OCOTP_WIDTH_MASK, (n) - 1)

#define OCOTP_OFFSET_CFG0	0x410
#define OCOTP_OFFSET_CFG1	0x420


int imx_ocotp_read_field(uint32_t field, unsigned *value);
int imx_ocotp_write_field(uint32_t field, unsigned value);
int imx_ocotp_permanent_write(int enable);
bool imx_ocotp_sense_enable(bool enable);

static inline u64 imx_ocotp_read_uid(void __iomem *ocotp)
{
	u64 uid;

	uid  = readl(ocotp + OCOTP_OFFSET_CFG0);
	uid <<= 32;
	uid |= readl(ocotp + OCOTP_OFFSET_CFG1);

	return uid;
}

#endif /* __MACH_IMX_OCOTP_H */
