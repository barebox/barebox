/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __ASM_SANDBOX_IO_H
#define __ASM_SANDBOX_IO_H

#define	IO_SPACE_LIMIT 0xffff
/* pacify static analyzers */
#define PCI_IOBASE     ((void __iomem *)__pci_iobase)

extern unsigned char __pci_iobase[IO_SPACE_LIMIT];

#include <asm-generic/io.h>

#endif /* __ASM_SANDBOX_IO_H */
