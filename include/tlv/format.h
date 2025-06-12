/* SPDX-License-Identifier: GPL-2.0-or-later OR MIT
 *
 * barebox TLVs are preceded by a 12 byte header: 4 bytes for magic,
 * 4 bytes for TLV sequence length (in bytes) and 4 bytes for
 * the length of the signature. Each tag consists of at least four
 * bytes: 2 bytes for the tag and two bytes for the length (in bytes)
 * and as many bytes as the length. The TLV sequence must be equal
 * to the TLV sequence length in the header. It can be followed by a
 * signature of the length described in the header. At the end
 * is a (big-endian) CRC-32/MPEG-2 of the whole structure. All
 * integers are in big-endian. Tags and magic have their MSB set
 * if they are vendor-specific. The second MSB is 0 for tag
 * and 1 for magic.
 */

#ifndef __TLV_FORMAT_H_
#define __TLV_FORMAT_H_

#include <linux/compiler.h>
#include <linux/types.h>
#include <asm/unaligned.h>
#include <linux/build_bug.h>

#define TLV_MAGIC_BAREBOX_V1		0x61bb95f2
#define TLV_MAGIC_LXA_BASEBOARD_V1	0xbc288dfe
#define TLV_MAGIC_LXA_POWERBOARD_V1	0xc6895c21
#define TLV_MAGIC_LXA_IOBOARD_V1	0xdca5a870

#define TLV_IS_VENDOR_SPECIFIC(val)	((*(u8 *)&(val) & 0x80) == 0x80)
#define TLV_IS_GENERIC(val)		((*(u8 *)&(val) & 0x80) != 0x80)

struct tlv {
	/*
	 * _tag:15 (MSB): product specific
	 */
	__be16 _tag;
	__be16 _len; /* in bytes */
	u8 _payload[];
} __packed;


#define TLV_TAG(tlv) get_unaligned_be16(&(tlv)->_tag)
#define TLV_LEN(tlv) get_unaligned_be16(&(tlv)->_len)
#define TLV_VAL(tlv) ((tlv)->_payload)

struct tlv_header {
	__be32 magic;
	__be32 length_tlv; /* in bytes */
	__be32 length_sig; /* in bytes */
	struct tlv tlvs[];
};
static_assert(sizeof(struct tlv_header) == 3 * 4);

#define for_each_tlv(tlv_head, tlv) \
	for (tlv = tlv_next(tlv_head, NULL); !IS_ERR_OR_NULL(tlv); tlv = tlv_next(tlv_head, tlv))

static inline size_t tlv_total_len(const struct tlv_header *header)
{
	return sizeof(struct tlv_header) + get_unaligned_be32(&header->length_tlv)
		+ get_unaligned_be32(&header->length_sig) + 4;
}

/*
 * Retrieves the CRC-32/MPEG-2 CRC32 at the end of a TLV blob. Parameters:
 * Poly=0x04C11DB7, Init=0xFFFFFFFF, RefIn=RefOut=false, XorOut=0x00000000
 */
static inline u32 tlv_crc(const struct tlv_header *header)
{
	return get_unaligned_be32((void *)header + tlv_total_len(header) - sizeof(__be32));
}

#endif
