// SPDX-License-Identifier: GPL-2.0-only
#ifndef __SOC_K3_KEYWRITER_LITE_H
#define __SOC_K3_KEYWRITER_LITE_H

#include <crypto/sha.h>

/*
 * structure layout and magic values are from:
 * https://software-dl.ti.com/tisci/esd/latest/6_topic_user_guides/key_writer_lite.html
 * with corrections from:
 * https://e2e.ti.com/support/processors-group/processors/f/processors-forum/1578568/am62l-am62l-secure-boot-keywriter-lite
 */

#define KEYWRITER_LITE_HEADER_MAGIC	0x9012
struct keywriter_lite_header {
	uint16_t magic;
	uint16_t size;
	uint8_t abi_major;	/* 0x00 */
	uint8_t abi_minor;	/* 0x01 */
	uint16_t reserved0;
	uint32_t cmd_id;	/* PROGRAMMING_MODE_* */
	uint32_t reserved1[2];
} __attribute__((packed));

#define MPK_OPTS_MAGIC	0x4a7e
struct mpk_opts {
	uint32_t field_header;
	uint32_t action_flags;
	uint16_t options;	/* 0x0 */
	uint8_t resv_field[2];	/* 0x0 */
	uint32_t reserved[2];	/* 0x0 */
} __attribute__((packed));

#define SMPKH_MAGIC	0x1234
#define BMPKH_MAGIC	0x9FFC
struct mpkh {
	uint32_t field_header;
	uint32_t action_flags;
	uint8_t mpkh[64];
	uint32_t reserved[2];
} __attribute__((packed));

#define KEY_CNT_MAGIC	0x5678
struct key_cnt {
	uint32_t field_header;
	uint32_t action_flags;
	uint32_t count;
	uint32_t reserved[2];
} __attribute__((packed));

#define KEY_REVISION_MAGIC	0x62c8
struct key_revision {
	uint32_t field_header;
	uint32_t action_flags;
	uint32_t revision; /* The key revision value encoded in bit-position format */
	uint32_t reserved[2];
} __attribute__((packed));

#define SBL_REVISION_MAGIC	0x8bad
struct sbl_revision {
	uint32_t field_header;
	uint32_t action_flags;
	uint64_t revision; /* The SBL software revision value encoded in bit-position format */
	uint32_t reserved[3];
} __attribute__((packed));

#define SYSFW_REVISION_MAGIC	0x45a9	/* must be typo: same as in struct sbl_revision */
struct sysfw_revision {
	uint32_t field_header;
	uint32_t action_flags;
	uint64_t revision; /* The SYSFW software revision value encoded in bit-position format */
	uint32_t reserved[3];
} __attribute__((packed));

#define BRDCFG_REVISION_MAGIC	0x98dc
struct brdcfg_revision {
	uint32_t field_header;
	uint32_t action_flags;
	uint64_t revision; /* The BRDCFG software revision value encoded in bit-position format */
	uint32_t reserved[4];
} __attribute__((packed));

#define MSV_MAGIC		0x1337
struct msv {
	uint32_t field_header;
	uint32_t action_flags;
	uint32_t msv;		/* The 20-bit MSV value */
	uint32_t reserved[2];
} __attribute__((packed));

#define JTAG_DISABLE_MAGIC	0x7421
struct jtag_disable {
	uint32_t field_header;
	uint32_t action_flags;
	uint32_t jtag;		/* The 4-bit jtag unlock disable value */
	uint32_t reserved[2];
} __attribute__((packed));

#define BOOT_MODE_MAGIC		0xa1b2
struct boot_mode {
	uint32_t field_header;
	uint32_t action_flags;
	uint32_t fuse_id;	/* The index of the fuse to program currently accepts only 1,2 */
	uint32_t boot_mode;	/* The 25-bit boot mode value to program */
	uint32_t reserved[2];
} __attribute__((packed));

#define EXT_OTP_MAGIC		0xd0e5
struct ext_otp {
	uint32_t field_header;
	uint32_t action_flags;
	uint16_t size;		/* The number of bits to program */
	uint16_t index;		/* The index in the 1024-bits space to start programming from */
	uint8_t wprp[16];	/* Read protect/write protect array */
	uint8_t otp[128];	/* The 1024 bits extended otp data */
	uint32_t reserved[4];
} __attribute__((packed));

struct keywriter_lite_payload {
	struct mpk_opts mpk_options;
	struct mpkh smpkh;
	struct mpkh bmpkh;
	struct key_cnt key_count;
	struct key_revision key_revision;
	struct sbl_revision sbl_rev;
	struct sysfw_revision sysfw_rev;
	struct brdcfg_revision brdcfg_rev;
	struct msv msv_val;
	struct jtag_disable jtag_disable;
	struct boot_mode boot_mode;
	struct ext_otp ext_otp;
} __attribute__((packed));

#define TISCI_MSG_KEY_WRITER_LITE 0x9045

struct keywriter_lite_blob {
	struct keywriter_lite_header hdr;
	struct keywriter_lite_payload payload;
	unsigned char hash[SHA512_DIGEST_SIZE];
} __attribute__((packed));

struct keywriter_lite_packet {
	uint32_t ver_info_unused;
	uint32_t tisci_id;
	struct keywriter_lite_blob blob;
};

enum action_flags {
	FLAG_WP =	BIT(0),
	FLAG_RP =	BIT(1),
	FLAG_OVRD =	BIT(2),
	FLAG_ACTIVE =	BIT(3),
};

int k3_kwl_set_smpkh(struct mpkh *mpkh, const char *sha, unsigned int flags);
int k3_kwl_set_bmpkh(struct mpkh *mpkh, const char *sha, unsigned int flags);
int k3_kwl_set_keycnt(struct key_cnt *k, unsigned int count, unsigned int flags);
int k3_kwl_set_key_revision(struct key_revision *k, unsigned int revision,
			   unsigned int flags);
int k3_kwl_set_sbl_revision(struct sbl_revision *k, unsigned int revision,
			   unsigned int flags);
int k3_kwl_set_brdcfg_revision(struct brdcfg_revision *k, unsigned int revision,
			   unsigned int flags);
int k3_kwl_set_msv(struct msv *k, unsigned int msv, unsigned int flags);
int k3_kwl_set_jtag_disable(struct jtag_disable *k, unsigned int jtag,
				 unsigned int flags);
int k3_kwl_set_bootmode(struct boot_mode *k, unsigned int fuse_id,
				 unsigned int boot_mode, unsigned int flags);
int k3_kwl_set_ext_otp(struct ext_otp *k, unsigned int bits, unsigned int index,
			    uint8_t *wprp, uint8_t *otp, unsigned int flags);

void k3_kwl_prepare(struct keywriter_lite_packet *p);
int k3_kwl_finalize(struct keywriter_lite_packet *p);
int k3_kwl_fuse_writebuff(struct keywriter_lite_packet *p);


#endif /* __SOC_K3_KEYWRITER_LITE_H */
