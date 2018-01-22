#ifndef __MACH_REVISION_H__
#define __MACH_REVISION_H__

/* silicon revisions  */
#define IMX_CHIP_REV_1_0	0x10
#define IMX_CHIP_REV_1_1	0x11
#define IMX_CHIP_REV_1_2	0x12
#define IMX_CHIP_REV_1_3	0x13
#define IMX_CHIP_REV_1_4	0x14
#define IMX_CHIP_REV_1_5	0x15
#define IMX_CHIP_REV_1_6	0x16
#define IMX_CHIP_REV_2_0	0x20
#define IMX_CHIP_REV_2_1	0x21
#define IMX_CHIP_REV_2_2	0x22
#define IMX_CHIP_REV_2_3	0x23
#define IMX_CHIP_REV_3_0	0x30
#define IMX_CHIP_REV_3_1	0x31
#define IMX_CHIP_REV_3_2	0x32
#define IMX_CHIP_REV_UNKNOWN	0xff

int imx_silicon_revision(void);

void imx_set_silicon_revision(const char *soc, int revision);

#endif /* __MACH_REVISION_H__ */
