#ifndef __ROCKCHIP_H
#define __ROCKCHIP_H

#define NEWIDB_MAGIC 0x534e4b52 /* 'RKNS' */

struct newidb_entry {
	uint32_t sector;
	uint32_t unknown_ffffffff;
	uint32_t unknown1;
	uint32_t image_number;
	unsigned char unknown2[8];
	unsigned char hash[64];
};

struct newidb {
	uint32_t magic;
	unsigned char unknown1[4];
	uint32_t n_files;
	uint32_t hashtype;
	unsigned char unknown2[8];
	unsigned char unknown3[8];
	unsigned char unknown4[88];
	struct newidb_entry entries[4];
	unsigned char unknown5[40];
	unsigned char unknown6[512];
	unsigned char unknown7[16];
	unsigned char unknown8[32];
	unsigned char unknown9[464];
	unsigned char hash[512];
};

#define RK_SECTOR_SIZE 512
#define RK_PAGE_SIZE 2048

#endif /* __ROCKCHIP_H */
