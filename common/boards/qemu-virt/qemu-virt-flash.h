/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __QEMU_VIRT_FLASH_H__
#define __QEMU_VIRT_FLASH_H__

#include <linux/stringify.h>

#ifdef CONFIG_RISCV
#define PARTS_TARGET_PATH	/flash@20000000
#define ENV_DEVICE_PATH		/flash@20000000/partitions/partition@3c00000
#elif defined(CONFIG_ARM) && defined(USE_NONSECURE_SECOND_FLASH)
#define PARTS_TARGET_PATH	/flash@4000000
#define ENV_DEVICE_PATH		/flash@4000000/partitions/partition@0
#elif defined CONFIG_ARM
#define PARTS_TARGET_PATH	/flash@0
#define ENV_DEVICE_PATH		/flash@0/partitions/partition@3c00000
#define VIRTUAL_REG		0x1000
#else
#define PARTS_TARGET_PATH
#define ENV_DEVICE_PATH
#endif

#define PARTS_TARGET_PATH_STR	__stringify(PARTS_TARGET_PATH)
#define ENV_DEVICE_PATH_STR	__stringify(ENV_DEVICE_PATH)

#endif
