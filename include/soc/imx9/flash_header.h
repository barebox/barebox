#ifndef SOC_IMX_FLASH_HEADER_H
#define SOC_IMX_FLASH_HEADER_H

#define HASH_MAX_LEN			64
#define IV_MAX_LEN			32
#define MAX_NUM_IMGS			8
#define MAX_NUM_OF_CONTAINER		3
#define MAX_HW_CFG_SIZE_V2		359

struct img_flags {
	char type;
	char core_id;
	char hash_type;
	bool encrypted;
	uint16_t boot_flags;
};

struct sig_blk_hdr {
	uint8_t version;
	uint16_t length;
	uint8_t tag;
	uint16_t srk_table_offset;
	uint16_t cert_offset;
	uint16_t blob_offset;
	uint16_t signature_offset;
	uint32_t reserved;
} __attribute__((packed));

struct boot_img {
	uint32_t offset;
	uint32_t size;
	uint64_t dst;
	uint64_t entry;
	uint32_t hab_flags;
	uint32_t meta;
	uint8_t hash[HASH_MAX_LEN];
	uint8_t iv[IV_MAX_LEN];
} __attribute__((packed));

struct flash_header_v3 {
	uint8_t version;
	uint16_t length;
	uint8_t tag;
	uint32_t flags;
	uint16_t sw_version;
	uint8_t fuse_version;
	uint8_t num_images;
	uint16_t sig_blk_offset;
	uint16_t reserved;
	struct boot_img img[MAX_NUM_IMGS];
	struct sig_blk_hdr sig_blk_hdr;
	uint32_t sigblk_size;
	uint32_t padding;
} __attribute__((packed));

struct ivt_header {
	uint8_t tag;
	uint16_t length;
	uint8_t version;
} __attribute__((packed));

struct write_dcd_command {
	uint8_t tag;
	uint16_t length;
	uint8_t param;
} __attribute__((packed));

struct dcd_addr_data {
	uint32_t addr;
	uint32_t value;
};

struct dcd_v2_cmd {
	struct write_dcd_command write_dcd_command; /*4*/
	struct dcd_addr_data addr_data[MAX_HW_CFG_SIZE_V2]; /*2872*/
} __attribute__((packed));

struct dcd_v2 {
	struct ivt_header header;   /*4*/
	struct dcd_v2_cmd dcd_cmd; /*2876*/
} __attribute__((packed)) ;                     /*2880*/

struct imx_header_v3 {
	struct flash_header_v3 fhdr[MAX_NUM_OF_CONTAINER];
	struct dcd_v2 dcd_table;
}  __attribute__((packed));

#endif /* SOC_IMX_FLASH_HEADER_H */
