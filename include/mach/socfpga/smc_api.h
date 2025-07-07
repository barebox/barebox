/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2020 Intel Corporation
 */

#ifndef _SMC_API_H_
#define _SMC_API_H_

int invoke_smc(u32 func_id, u64 arg0, u64 arg1, u64 arg2, u32 *ret_payload);
int smc_get_usercode(u32 *usercode);

#endif /* _SMC_API_H_ */
