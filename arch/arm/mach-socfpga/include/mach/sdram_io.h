/*
 * Copyright Altera Corporation (C) 2012-2014. All rights reserved
 *
 * SPDX-License-Identifier:    BSD-3-Clause
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of Altera Corporation nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ALTERA CORPORATION BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <mach/cyclone5-sdram.h>

#define MGR_SELECT_MASK   0xf8000

#define APB_BASE_SCC_MGR	SDR_PHYGRP_SCCGRP_ADDRESS
#define APB_BASE_PHY_MGR	SDR_PHYGRP_PHYMGRGRP_ADDRESS
#define APB_BASE_RW_MGR		SDR_PHYGRP_RWMGRGRP_ADDRESS
#define APB_BASE_DATA_MGR	SDR_PHYGRP_DATAMGRGRP_ADDRESS
#define APB_BASE_REG_FILE	SDR_PHYGRP_REGFILEGRP_ADDRESS
#define APB_BASE_MMR		SDR_CTRLGRP_ADDRESS

#define __AVL_TO_APB(ADDR) \
	((((ADDR) & MGR_SELECT_MASK) == (BASE_PHY_MGR))  ? (APB_BASE_PHY_MGR)  | (((ADDR) >> (14-6)) & (0x1<<6))  | ((ADDR) & 0x3f) : \
	 (((ADDR) & MGR_SELECT_MASK) == (BASE_RW_MGR))   ? (APB_BASE_RW_MGR)   | ((ADDR) & 0x1fff) : \
 	 (((ADDR) & MGR_SELECT_MASK) == (BASE_DATA_MGR)) ? (APB_BASE_DATA_MGR) | ((ADDR) & 0x7ff) : \
	 (((ADDR) & MGR_SELECT_MASK) == (BASE_SCC_MGR))  ? (APB_BASE_SCC_MGR)  | ((ADDR) & 0xfff) : \
	 (((ADDR) & MGR_SELECT_MASK) == (BASE_REG_FILE)) ? (APB_BASE_REG_FILE) | ((ADDR) & 0x7ff) : \
	 (((ADDR) & MGR_SELECT_MASK) == (BASE_MMR))      ? (APB_BASE_MMR)      | ((ADDR) & 0xfff) : \
	 -1)

#define IOWR_32DIRECT(BASE, OFFSET, DATA) \
	write_register(HPS_SDR_BASE, __AVL_TO_APB((uint32_t)((BASE) + (OFFSET))), DATA)

#define IORD_32DIRECT(BASE, OFFSET) \
	read_register(HPS_SDR_BASE, __AVL_TO_APB((uint32_t)((BASE) + (OFFSET))))
	#define write_register(BASE, OFFSET, DATA) \
		writel(DATA, ((BASE) + (OFFSET)))
	#define read_register(BASE, OFFSET) \
		readl((BASE) + (OFFSET))
	#define HPS_SDR_BASE 0xffc20000
