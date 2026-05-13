/* SPDX-License-Identifier: GPL-2.0 */
#ifndef SOCFPGA_BAREBOX_ARM_H_
#define SOCFPGA_BAREBOX_ARM_H_

#include <asm/barebox-arm.h>
#include <mach/socfpga/soc64-handoff.h>

/*
 * The SDM firmware uses the last page in the OCRAM for handoff data. Put the
 * stack below the handoff data.
 *
 * Note: U-Boot puts the stack at 0x71000 (0x80000 - 0xf000) and reserves even
 * more space.
 */
#define AGILEX5_STACKTOP SOC64_HANDOFF_BASE

#define ENTRY_FUNCTION_AGILEX5(name)					\
	ENTRY_FUNCTION_WITHSTACK(name, AGILEX5_STACKTOP, r0, r1, r2)

#endif
