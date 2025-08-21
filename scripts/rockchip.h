#ifndef __ROCKCHIP_H
#define __ROCKCHIP_H

#define NEWIDB_MAGIC_RKNS 0x534e4b52
#define NEWIDB_MAGIC_RKSS 0x53534b52

#define NEWIDB_FLAGS_SHA256	(1U << 0)
#define NEWIDB_FLAGS_SHA512	(1U << 1)
#define NEWIDB_FLAGS_RSA2048	(1U << 4)
#define NEWIDB_FLAGS_RSA4096	(1U << 5)
#define NEWIDB_FLAGS_SIGNED	(1U << 13)

struct newidb_entry {
	uint32_t sector;
	uint32_t unknown_ffffffff;
	uint32_t unknown1;
	uint32_t image_number;
	unsigned char unknown2[8];
	unsigned char hash[64];
};

struct newidb_key {
	unsigned char modulus[512];
	unsigned char exponent[16];
	unsigned char np[32];
};

struct newidb {
	uint32_t magic;
	unsigned char unknown1[4];
	uint32_t n_files;
	uint32_t flags;
	unsigned char unknown2[8];
	unsigned char unknown3[8];
	unsigned char unknown4[88];
	struct newidb_entry entries[4];
	unsigned char unknown5[40];
	struct newidb_key key;
	unsigned char unknown9[464];
	unsigned char hash[512];
};

#define RK_SECTOR_SIZE 512
#define RK_PAGE_SIZE 2048

#endif /* __ROCKCHIP_H */
