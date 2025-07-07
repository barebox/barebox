// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2020-2024 Intel Corporation <www.intel.com>
 *
 */

#include <common.h>
#include <mach/socfpga/intel-smc.h>
#include <mach/socfpga/smc_api.h>
#include <linux/arm-smccc.h>
#include <linux/kernel.h>

int invoke_smc(u32 func_id, u64 arg0, u64 arg1, u64 arg2, u32 *ret_payload)
{
	struct arm_smccc_res res;

	arm_smccc_smc(func_id, arg0, arg1, arg2, 0, 0, 0, 0, &res);

	if (ret_payload) {
		ret_payload[0] = lower_32_bits(res.a0);
		ret_payload[1] = upper_32_bits(res.a0);
		ret_payload[2] = lower_32_bits(res.a1);
		ret_payload[3] = upper_32_bits(res.a1);
	}

	return res.a0;
}

int smc_get_usercode(u32 *usercode)
{
	u32 res;
	int ret;

	if (!usercode)
		return -EINVAL;

	ret = invoke_smc(INTEL_SIP_SMC_GET_USERCODE, 0, 0, 0, &res);

	if (ret == INTEL_SIP_SMC_STATUS_OK)
		*usercode = res;

	return ret;
}
