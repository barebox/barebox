// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020-2022 Intel Corporation <www.intel.com>
 *
 */

#include <common.h>
#include <io.h>
#include <mach/socfpga/intel-smc.h>
#include <mach/socfpga/secure_reg_helper.h>
#include <mach/socfpga/smc_api.h>
#include <mach/socfpga/soc64-regs.h>
#include <mach/socfpga/soc64-system-manager.h>
#include <linux/errno.h>

static int socfpga_secure_convert_reg_id_to_addr(u32 id, phys_addr_t *reg_addr)
{
	switch (id) {
	case SOCFPGA_SECURE_REG_SYSMGR_SOC64_EMAC0:
		*reg_addr = SOCFPGA_SYSMGR_ADDRESS + SYSMGR_SOC64_EMAC0;
		break;
	case SOCFPGA_SECURE_REG_SYSMGR_SOC64_EMAC1:
		*reg_addr = SOCFPGA_SYSMGR_ADDRESS + SYSMGR_SOC64_EMAC1;
		break;
	case SOCFPGA_SECURE_REG_SYSMGR_SOC64_EMAC2:
		*reg_addr = SOCFPGA_SYSMGR_ADDRESS + SYSMGR_SOC64_EMAC2;
		break;
	default:
		return -EADDRNOTAVAIL;
	}
	return 0;
}

int socfpga_secure_reg_read32(u32 id, u32 *val)
{
	int ret;
	u32 ret_arg[4];
	phys_addr_t reg_addr;

	ret = socfpga_secure_convert_reg_id_to_addr(id, &reg_addr);
	if (ret)
		return ret;

	ret = invoke_smc(INTEL_SIP_SMC_REG_READ, reg_addr, 0, 0, ret_arg);
	if (ret)
		return ret;

	*val = ret_arg[0];

	return 0;
}

int socfpga_secure_reg_write32(u32 id, u32 val)
{
	int ret;
	phys_addr_t reg_addr;

	ret = socfpga_secure_convert_reg_id_to_addr(id, &reg_addr);
	if (ret)
		return ret;

	return invoke_smc(INTEL_SIP_SMC_REG_WRITE, reg_addr, (u64) val, 0, 0);
}

int socfpga_secure_reg_update32(u32 id, u32 mask, u32 val)
{
	int ret;
	phys_addr_t reg_addr;

	ret = socfpga_secure_convert_reg_id_to_addr(id, &reg_addr);
	if (ret)
		return ret;

	return invoke_smc(INTEL_SIP_SMC_REG_UPDATE, reg_addr, (u64) mask, (u64) val, 0);
}
