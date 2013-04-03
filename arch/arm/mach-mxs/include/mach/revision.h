#ifndef __MACH_REVISION_H__
#define __MACH_REVISION_H__

/* silicon revisions  */
enum silicon_revision {
	SILICON_REVISION_1_0 = 0x10,
	SILICON_REVISION_1_1 = 0x11,
	SILICON_REVISION_1_2 = 0x12,
	SILICON_REVISION_1_3 = 0x13,
	SILICON_REVISION_1_4 = 0x14,
	SILICON_REVISION_2_0 = 0x20,
	SILICON_REVISION_2_1 = 0x21,
	SILICON_REVISION_2_2 = 0x22,
	SILICON_REVISION_2_3 = 0x23,
	SILICON_REVISION_3_0 = 0x30,
	SILICON_REVISION_3_1 = 0x31,
	SILICON_REVISION_3_2 = 0x32,
	SILICON_REVISION_UNKNOWN =0xff
};

int silicon_revision_get(void);
void silicon_revision_set(const char *soc, int revision);

#endif /* __MACH_REVISION_H__ */
