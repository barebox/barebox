#ifndef __MACH_IMX_ROMAPI_H
#define __MACH_IMX_ROMAPI_H

struct rom_api {
	u16 ver;
	u16 tag;
	u32 reserved1;
	u32 (*download_image)(u8 *dest, u32 offset, u32 size,  u32 xor);
	u32 (*query_boot_infor)(u32 info_type, u32 *info, u32 xor);
};

enum boot_dev_type_e {
	BT_DEV_TYPE_SD = 1,
	BT_DEV_TYPE_MMC = 2,
	BT_DEV_TYPE_NAND = 3,
	BT_DEV_TYPE_FLEXSPINOR = 4,
	BT_DEV_TYPE_SPI_NOR = 6,

	BT_DEV_TYPE_USB = 0xE,
	BT_DEV_TYPE_MEM_DEV = 0xF,

	BT_DEV_TYPE_INVALID = 0xFF
};

#define QUERY_ROM_VER		1
#define QUERY_BT_DEV		2
#define QUERY_PAGE_SZ		3
#define QUERY_IVT_OFF		4
#define QUERY_BT_STAGE		5
#define QUERY_IMG_OFF		6

#define ROM_API_OKAY		0xF0

int imx8mp_bootrom_load_image(void);
int imx8mn_bootrom_load_image(void);

#endif /* __MACH_IMX_ROMAPI_H */
