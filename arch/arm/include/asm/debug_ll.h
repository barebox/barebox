/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __ASM_DEBUG_LL_H__
#define __ASM_DEBUG_LL_H__

#ifdef CONFIG_DEBUG_QEMU_ARM64_VIRT
#define DEBUG_LL_UART_ADDR		0x9000000
#include <debug_ll/pl011.h>
#elif defined CONFIG_ARCH_IMX
#include <mach/imx/debug_ll.h>
#elif defined CONFIG_ARCH_ROCKCHIP
#include <mach/rockchip/debug_ll.h>
#elif defined CONFIG_ARCH_ZYNQMP
#include <mach/zynqmp/debug_ll.h>
#elif defined CONFIG_ARCH_MVEBU
#include <mach/mvebu/debug_ll.h>
#elif defined CONFIG_ARCH_DAVINCI
#include <mach/davinci/debug_ll.h>
#elif defined CONFIG_ARCH_BCM283X
#include <mach/bcm283x/debug_ll.h>
#else
#ifndef CONFIG_ARCH_ARM64_VIRT
#include <mach/debug_ll.h>
#endif
#endif

#endif
