// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "k3-keywriter: " fmt

#include <digest.h>
#include <string.h>
#include <stdio.h>
#include <dma.h>
#include <memory.h>
#include <linux/arm-smccc.h>
#include <linux/bits.h>
#include <linux/bitfield.h>
#include <crypto/sha.h>
#include <soc/k3/keywriter-lite.h>

/* from https://software-dl.ti.com/tisci/esd/latest/6_topic_user_guides/key_writer_lite.html */

#define PROGRAMMING_MODE_ONESHOT 	0 /* all supported fields at once */
#define PROGRAMMING_MODE_MULTISHOT 	1 /* a subset of supported fields at once */
#define PROGRAMMING_MODE_SMPKH	 	2 /* ust SMPKH */
#define PROGRAMMING_MODE_BMPKH 		3 /* just BMPKH */
#define PROGRAMMING_MODE_KEY_COUNT 	4 /* just key count */
#define PROGRAMMING_MODE_KEY_REVISION 	5 /* just key revision */
#define PROGRAMMING_MODE_SBL_SWREV 	6 /* just SBL swrev */
#define PROGRAMMING_MODE_SYSFW_SWREV 	7 /* just SYSFW swrev */
#define PROGRAMMING_MODE_BRDCFG_SWREV 	8 /* just BRDCFG swrev */
#define PROGRAMMING_MODE_MSV 		9 /* just MSV */
#define PROGRAMMING_MODE_JTAG 		10 /* just JTAG unlock disable */
#define PROGRAMMING_MODE_BOOT_MODE 	11 /* just efuse boot modes */
#define PROGRAMMING_MODE_EXTENDED_OTP 	12 /* just extended otp */

/*
 * action_flags
 * see gen_keywr_cert_helpers.sh:
 * 16#${wp_flag}${rp_flag}${ovrd_flag}${active_flag}
 */
#define ACTION_FLAG_ENABLE 0x5a
#define ACTION_FLAG_DISABLE 0xa5

#define ACTION_FLAG_WP		GENMASK(31, 24)
#define ACTION_FLAG_RP		GENMASK(23, 16)
#define ACTION_FLAG_OVRD	GENMASK(15, 8)
#define ACTION_FLAG_ACTIVE	GENMASK(7, 0)

static unsigned int k3_kwl_set_flags(unsigned int f)
{
	unsigned int en = ACTION_FLAG_ENABLE;
	unsigned int dis = ACTION_FLAG_DISABLE;

	return FIELD_PREP(ACTION_FLAG_WP, f & FLAG_WP ? en : dis) |
		FIELD_PREP(ACTION_FLAG_RP, f & FLAG_RP ? en : dis) |
		FIELD_PREP(ACTION_FLAG_OVRD, f & FLAG_OVRD ? en : dis) |
		FIELD_PREP(ACTION_FLAG_ACTIVE, f & FLAG_ACTIVE ? en : dis);
}

int k3_kwl_set_smpkh(struct mpkh *mpkh, const char *sha, unsigned int flags)
{
	mpkh->field_header = SMPKH_MAGIC;
	mpkh->action_flags = k3_kwl_set_flags(flags);
	memcpy(mpkh->mpkh, sha, SHA512_DIGEST_SIZE);

	return 0;
}

int k3_kwl_set_bmpkh(struct mpkh *mpkh, const char *sha, unsigned int flags)
{
	mpkh->field_header = BMPKH_MAGIC;
	mpkh->action_flags = k3_kwl_set_flags(flags);
	memcpy(mpkh->mpkh, sha, SHA512_DIGEST_SIZE);

	return 0;
}

int k3_kwl_set_keycnt(struct key_cnt *k, unsigned int count, unsigned int flags)
{
	if (count < 1 || count > 2) {
		pr_err("Only 1 or 2 allowed for key count\n");
		return -EINVAL;
	}

	k->field_header = KEY_CNT_MAGIC;
	k->action_flags = k3_kwl_set_flags(flags);
	k->count = BIT(count) - 1;

	return 0;
}

int k3_kwl_set_key_revision(struct key_revision *k, unsigned int revision,
			   unsigned int flags)
{
	if (revision < 1 || revision > 2) {
		pr_err("Only 1 or 2 allowed for key revision\n");
		return -EINVAL;
	}

	k->field_header = KEY_REVISION_MAGIC;
	k->action_flags = k3_kwl_set_flags(flags);
	k->revision = BIT(revision) - 1;

	return 0;
}

int k3_kwl_set_sbl_revision(struct sbl_revision *k, unsigned int revision,
			   unsigned int flags)
{
	k->field_header = SBL_REVISION_MAGIC;
	k->action_flags = k3_kwl_set_flags(flags);
	k->revision = BIT(revision) - 1;

	return 0;
}

int k3_kwl_set_brdcfg_revision(struct brdcfg_revision *k, unsigned int revision,
			   unsigned int flags)
{
	k->field_header = BRDCFG_REVISION_MAGIC;
	k->action_flags = k3_kwl_set_flags(flags);
	k->revision = BIT(revision) - 1;

	return 0;
}

int k3_kwl_set_msv(struct msv *k, unsigned int msv, unsigned int flags)
{
	k->field_header = MSV_MAGIC;
	k->action_flags = k3_kwl_set_flags(flags);
	k->msv = msv;

	return 0;
}

int k3_kwl_set_jtag_disable(struct jtag_disable *k, unsigned int jtag,
				 unsigned int flags)
{
	k->field_header = JTAG_DISABLE_MAGIC;
	k->action_flags = k3_kwl_set_flags(flags);
	k->jtag = jtag;

	return 0;
}

int k3_kwl_set_bootmode(struct boot_mode *k, unsigned int fuse_id,
				 unsigned int boot_mode, unsigned int flags)
{
	if (fuse_id < 1 || fuse_id > 2) {
		pr_err("Only 1 or 2 allowed for fuse_id\n");
		return -EINVAL;
	}

	k->field_header = BOOT_MODE_MAGIC;
	k->action_flags = k3_kwl_set_flags(flags);
	k->fuse_id = fuse_id;
	k->boot_mode = boot_mode;

	return 0;
}

int k3_kwl_set_ext_otp(struct ext_otp *k, unsigned int bits, unsigned int index,
			    uint8_t *wprp, uint8_t *otp, unsigned int flags)
{
	k->field_header = EXT_OTP_MAGIC;
	k->action_flags = k3_kwl_set_flags(flags);
	k->size = bits;
	k->index = index;
	memcpy(k->wprp, wprp, sizeof(k->wprp));
	memcpy(k->otp, otp, sizeof(k->otp));

	return 0;
}

#define FD FIELD_PREP_CONST(ACTION_FLAG_ACTIVE, ACTION_FLAG_DISABLE)

static struct keywriter_lite_packet keywrite_lite_template = {
	.tisci_id = TISCI_MSG_KEY_WRITER_LITE,
	.blob = {
		.hdr = {
			.magic = KEYWRITER_LITE_HEADER_MAGIC,
			.size = sizeof(struct keywriter_lite_payload),
			.abi_major = 0x0,
			.abi_minor = 0x1,
			.cmd_id = PROGRAMMING_MODE_MULTISHOT,
		},

		.payload = {
			.mpk_options = {
				.field_header = MPK_OPTS_MAGIC,
				.action_flags = FD,
			},
			.smpkh = {
				.field_header = SMPKH_MAGIC,
				.action_flags = FD,
			},
			.bmpkh = {
				.field_header = BMPKH_MAGIC,
				.action_flags = FD,
			},
			.key_count = {
				.field_header = KEY_CNT_MAGIC,
				.action_flags = FD,
			},
			.key_revision = {
				.field_header = KEY_REVISION_MAGIC,
				.action_flags = FD,
			},
			.sbl_rev = {
				.field_header = SBL_REVISION_MAGIC,
				.action_flags = FD,
			},
			.sysfw_rev = {
				.field_header = SYSFW_REVISION_MAGIC,
				.action_flags = FD,
			},
			.brdcfg_rev = {
				.field_header = BRDCFG_REVISION_MAGIC,
				.action_flags = FD,
			},
			.msv_val = {
				.field_header = MSV_MAGIC,
				.action_flags = FD,
			},
			.jtag_disable = {
				.field_header = JTAG_DISABLE_MAGIC,
				.action_flags = FD,
			},
			.boot_mode = {
				.field_header = BOOT_MODE_MAGIC,
				.action_flags = FD,
			},
			.ext_otp = {
				.field_header = EXT_OTP_MAGIC,
				.action_flags = FD,
			},
		},
	},
};
#undef FD

void k3_kwl_prepare(struct keywriter_lite_packet *p)
{
	memcpy(p, &keywrite_lite_template, sizeof(keywrite_lite_template));
}

int k3_kwl_finalize(struct keywriter_lite_packet *p)
{
	struct digest *sha512;
	int ret;

	sha512 = digest_alloc("sha512");
	if (!sha512)
		return -ENOENT;

	ret = digest_digest(sha512, &p->blob, offsetof(typeof(p->blob), hash),
			    p->blob.hash);
	if (ret)
		return ret;

	digest_free(sha512);

	return 0;
}

#define K3_SIP_OTP_WRITEBUFF 0xC2000000

int k3_kwl_fuse_writebuff(struct keywriter_lite_packet *p)
{
	struct resource *resource;
	unsigned long keywriter_lite_data = 0x82000000;
	struct arm_smccc_res res;

	resource = request_sdram_region("keywriter-lite", keywriter_lite_data,
					4096, MEMTYPE_LOADER_DATA, MEMATTRS_RW);
	if (!resource)
		return -ENOMEM;

	/*
	 * Yes, we are passing the memory address of the buffer along with
	 * the SMC call, but TF-A will crash when it is different from this
	 * address on AM62L.
	 */

	memcpy((void *)keywriter_lite_data, p, sizeof(keywrite_lite_template));

	/* Make SiP SMC call and send the addr in the parameter register */
	arm_smccc_smc(K3_SIP_OTP_WRITEBUFF, (unsigned long)keywriter_lite_data,
		      0, 0, 0, 0, 0, 0, &res);

	if (res.a0 != 0)
		printf("SMC call failed: Error code %ld\n", res.a0);

	release_sdram_region(resource);

	return res.a0;
}
