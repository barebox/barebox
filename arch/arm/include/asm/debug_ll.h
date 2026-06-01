/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __ASM_DEBUG_LL_H__
#define __ASM_DEBUG_LL_H__

#if defined CONFIG_DEBUG_IMX_UART
#include <mach/imx/debug_ll.h>
#elif defined CONFIG_DEBUG_ROCKCHIP_UART
#include <mach/rockchip/debug_ll.h>
#elif defined CONFIG_DEBUG_OMAP_UART
#include <mach/omap/debug_ll.h>
#elif defined CONFIG_DEBUG_ZYNQMP_UART
#include <mach/zynqmp/debug_ll.h>
#elif defined CONFIG_DEBUG_STM32MP_UART
#include <mach/stm32mp/debug_ll.h>
#elif defined CONFIG_DEBUG_VEXPRESS_UART || defined CONFIG_DEBUG_QEMU_ARM32_VIRT
#include <mach/vexpress/debug_ll.h>
#elif defined CONFIG_DEBUG_BCM283X_UART
#include <mach/bcm283x/debug_ll.h>
#elif defined CONFIG_DEBUG_LAYERSCAPE_UART
#include <mach/layerscape/debug_ll.h>
#elif defined CONFIG_DEBUG_SEMIHOSTING
#include <debug_ll/semihosting.h>
#elif defined CONFIG_DEBUG_AT91_UART
#include <mach/at91/debug_ll.h>
#elif defined CONFIG_DEBUG_QEMU_ARM64_VIRT
#define DEBUG_LL_UART_ADDR		0x9000000
#include <debug_ll/pl011.h>
#elif defined CONFIG_DEBUG_AM62X_UART
#include <mach/k3/debug_ll.h>
#elif defined CONFIG_DEBUG_MVEBU_UART
#include <mach/mvebu/debug_ll.h>
#elif defined CONFIG_DEBUG_CLPS711X_UART
#include <mach/clps711x/debug_ll.h>
#elif defined CONFIG_DEBUG_MXS_UART
#include <mach/mxs/debug_ll.h>
#elif defined CONFIG_DEBUG_VERSATILE_UART
#include <mach/versatile/debug_ll.h>
#elif defined CONFIG_DEBUG_TEGRA_UART
#include <mach/tegra/debug_ll.h>
#elif defined CONFIG_DEBUG_ZYNQ_UART
#include <mach/zynq/debug_ll.h>
#elif defined CONFIG_DEBUG_SOCFPGA_UART
#include <mach/socfpga/debug_ll.h>
#endif

#endif
