/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __IO_H
#define __IO_H

#include <asm/io.h>

#define IOMEM_ERR_PTR(err) (__force void __iomem *)ERR_PTR(err)

#endif /* __IO_H */
