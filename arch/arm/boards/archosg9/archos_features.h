#ifndef __ARCHOS_FEATURES_H
#define __ARCHOS_FEATURES_H

/* bootloader version */
#define ATAG_BOOT_VERSION	0x5441000A

struct tag_boot_version {
	u32		major;
	u32		minor;
	u32		extra;
};

#define ATAG_FEATURE_LIST	0x5441000B

struct tag_feature_list {
	u32	size;
	u8	data[0];
};

struct tag *archos_append_atags(struct tag * params);

#endif /* __ARCHOS_FEATURES_H */
