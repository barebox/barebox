/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __PBL_SHA_H_

#define __PBL_SHA_H_

#include <digest.h>
#include <types.h>

int sha256_init(struct digest *desc);
int sha256_update(struct digest *desc, const void *data, unsigned long len);
int sha256_final(struct digest *desc, u8 *out);

#endif /* __PBL-SHA_H_ */
