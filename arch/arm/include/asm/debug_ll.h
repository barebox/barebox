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
#elif defined CONFIG_ARCH_STM32MP
#include <mach/stm32mp/debug_ll.h>
#elif defined CONFIG_ARCH_ZYNQ
#include <mach/zynq/debug_ll.h>
#elif defined CONFIG_ARCH_VEXPRESS
#include <mach/vexpress/debug_ll.h>
#elif defined CONFIG_ARCH_VERSATILE
#include <mach/versatile/debug_ll.h>
#elif defined CONFIG_ARCH_LAYERSCAPE
#include <mach/layerscape/debug_ll.h>
#elif defined CONFIG_ARCH_TEGRA
#include <mach/tegra/debug_ll.h>
#elif defined CONFIG_ARCH_UEMD
#include <mach/uemd/debug_ll.h>
#elif defined CONFIG_ARCH_SOCFPGA
#include <mach/socfpga/debug_ll.h>
#elif defined CONFIG_ARCH_PXA
#include <mach/pxa/debug_ll.h>
#elif defined CONFIG_ARCH_OMAP
#include <mach/omap/debug_ll.h>
#elif defined CONFIG_ARCH_NOMADIK
#include <mach/nomadik/debug_ll.h>
#elif defined CONFIG_ARCH_MXS
#include <mach/mxs/debug_ll.h>
#elif defined CONFIG_ARCH_EP93XX
#include <mach/ep93xx/debug_ll.h>
#elif defined CONFIG_ARCH_DIGIC
#include <mach/digic/debug_ll.h>
#elif defined CONFIG_ARCH_CLPS711X
#include <mach/clps711x/debug_ll.h>
#else
#ifndef CONFIG_ARCH_ARM64_VIRT
#include <mach/debug_ll.h>
#endif
#endif

#endif
