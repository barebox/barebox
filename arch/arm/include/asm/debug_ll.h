/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __ASM_DEBUG_LL_H__
#define __ASM_DEBUG_LL_H__

#ifdef CONFIG_DEBUG_IMX_UART
#include <mach/imx/debug_ll.h>
#endif

#ifdef CONFIG_DEBUG_ROCKCHIP_UART
#include <mach/rockchip/debug_ll.h>
#endif

#ifdef CONFIG_DEBUG_OMAP_UART
#include <mach/omap/debug_ll.h>
#endif

#ifdef CONFIG_DEBUG_ZYNQMP_UART
#include <mach/zynqmp/debug_ll.h>
#endif

#ifdef CONFIG_DEBUG_STM32MP_UART
#include <mach/stm32mp/debug_ll.h>
#endif

#ifdef CONFIG_DEBUG_VEXPRESS_UART
#include <mach/vexpress/debug_ll.h>
#endif

#ifdef CONFIG_DEBUG_BCM283X_UART
#include <mach/bcm283x/debug_ll.h>
#endif

#ifdef CONFIG_DEBUG_LAYERSCAPE_UART
#include <mach/layerscape/debug_ll.h>
#endif

#ifdef CONFIG_DEBUG_QEMU_ARM64_VIRT
#define DEBUG_LL_UART_ADDR		0x9000000
#include <debug_ll/pl011.h>
#elif defined CONFIG_ARCH_MVEBU
#include <mach/mvebu/debug_ll.h>
#elif defined CONFIG_ARCH_ZYNQ
#include <mach/zynq/debug_ll.h>
#elif defined CONFIG_ARCH_VERSATILE
#include <mach/versatile/debug_ll.h>
#elif defined CONFIG_ARCH_TEGRA
#include <mach/tegra/debug_ll.h>
#elif defined CONFIG_ARCH_SOCFPGA
#include <mach/socfpga/debug_ll.h>
#elif defined CONFIG_ARCH_PXA
#include <mach/pxa/debug_ll.h>
#elif defined CONFIG_ARCH_MXS
#include <mach/mxs/debug_ll.h>
#elif defined CONFIG_ARCH_CLPS711X
#include <mach/clps711x/debug_ll.h>
#elif defined CONFIG_ARCH_AT91
#include <mach/at91/debug_ll.h>
#elif defined CONFIG_ARCH_K3
#include <mach/k3/debug_ll.h>
#endif

#endif
