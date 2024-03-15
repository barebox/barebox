/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_PCI_H
#define __ASM_PCI_H

#include <efi/efi-mode.h>

#define pcibios_assign_all_busses()	(!efi_is_payload())

#endif
