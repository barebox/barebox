/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2019 Kalray Inc.
 */

#ifndef _ASM_KVX_FTU_H
#define _ASM_KVX_FTU_H

/* FTU reset cause register definitions */
#define KVX_FTU_RESET_CAUSE_OFFSET                 0x48
#define KVX_FTU_RESET_CAUSE_CODE_MPPA              0x0
#define KVX_FTU_RESET_CAUSE_CODE_DSU               0x1
#define KVX_FTU_RESET_CAUSE_CODE_SW                0x2
#define KVX_FTU_RESET_CAUSE_CODE_PCIE              0x3
#define KVX_FTU_RESET_CAUSE_CODE_WDOG_0            0x8
#define KVX_FTU_RESET_CAUSE_CODE_WDOG_1            0x9
#define KVX_FTU_RESET_CAUSE_CODE_WDOG_2            0xA
#define KVX_FTU_RESET_CAUSE_CODE_WDOG_3            0xB
#define KVX_FTU_RESET_CAUSE_CODE_WDOG_4            0xC

/* FTU reset register definitions */
#define KVX_FTU_SW_RESET_OFFSET                    0x50

#endif /* _ASM_KVX_FTU_H */
