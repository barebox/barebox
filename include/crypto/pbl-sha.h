/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __PBL_SHA_H_

#define __PBL_SHA_H_

#include <crypto/sha.h>
#include <digest.h>
#include <types.h>

int sha256_init(struct digest *desc);
int sha256_update(struct digest *desc, const void *data, unsigned long len);
int sha256_final(struct digest *desc, u8 *out);

/* One-shot SHA-256 that picks the best transform available in the PBL. */
void pbl_sha256(const void *buf, size_t len, u8 out[SHA256_DIGEST_SIZE]);

#endif /* __PBL-SHA_H_ */
