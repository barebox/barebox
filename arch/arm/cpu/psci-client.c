// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2017 Masahiro Yamada <yamada.masahiro@socionext.com>
 * Copyright (C) 2019 Ahmad Fatoum, Pengutronix
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <asm/psci.h>
#include <asm/secure.h>
#include <poweroff.h>
#include <restart.h>
#include <linux/arm-smccc.h>
#include <efi/types.h>

static struct restart_handler restart;
static struct poweroff_handler poweroff;

static __efi_runtime_data u32 (*psci_invoke_fn)(ulong, ulong, ulong, ulong);

static void __noreturn psci_invoke_noreturn(ulong function)
{
	int ret;

	ret = psci_invoke(function, 0, 0, 0, NULL);

	pr_err("psci command failed: %pe\n", ERR_PTR(ret));
	hang();
}

static void __noreturn psci_poweroff(struct poweroff_handler *handler,
				     unsigned long flags)
{
	psci_invoke_noreturn(ARM_PSCI_0_2_FN_SYSTEM_OFF);
}

static void __noreturn psci_restart(struct restart_handler *rst,
				    unsigned long flags)
{
	psci_invoke_noreturn(ARM_PSCI_0_2_FN_SYSTEM_RESET);
}

static void __noreturn __efi_runtime rt_psci_poweroff(unsigned long flags)
{
	psci_invoke_fn(ARM_PSCI_0_2_FN_SYSTEM_OFF, 0, 0, 0);
	__hang();
}

static void __noreturn __efi_runtime rt_psci_restart(unsigned long flags)
{
	psci_invoke_fn(ARM_PSCI_0_2_FN_SYSTEM_RESET, 0, 0, 0);
	__hang();
}

static u32 version;
int psci_get_version(void)
{
	if (!version)
		return -EPROBE_DEFER;

	return version;
}


static int psci_xlate_error(s32 errnum)
{
       switch (errnum) {
       case ARM_PSCI_RET_NOT_SUPPORTED:
               return -ENOTSUPP; // Operation not supported
       case ARM_PSCI_RET_INVAL:
               return -EINVAL; // Invalid argument
       case ARM_PSCI_RET_DENIED:
               return -EPERM; // Operation not permitted
       case ARM_PSCI_RET_ALREADY_ON:
               return -EBUSY; // CPU already on
       case ARM_PSCI_RET_ON_PENDING:
               return -EALREADY; // CPU_ON in progress
       case ARM_PSCI_RET_INTERNAL_FAILURE:
               return -EIO; // Internal failure
       case ARM_PSCI_RET_NOT_PRESENT:
	       return -ESRCH; // Trusted OS not present on core
       case ARM_PSCI_RET_DISABLED:
               return -ENODEV; // CPU is disabled
       case ARM_PSCI_RET_INVALID_ADDRESS:
               return -EACCES; // Bad address
       default:
	       return errnum;
       };
}

int psci_invoke(ulong function, ulong arg0, ulong arg1, ulong arg2,
		ulong *result)
{
	ulong ret;
	if (!psci_invoke_fn)
		return -EPROBE_DEFER;

	ret = psci_invoke_fn(function, arg0, arg1, arg2);
	if (result)
		*result = ret;

	switch (function) {
	case ARM_PSCI_0_2_FN_PSCI_VERSION:
	case ARM_PSCI_1_0_FN64_STAT_RESIDENCY:
	case ARM_PSCI_1_0_FN64_STAT_COUNT:
		/* These don't return an error code */
		return 0;
	}

	return psci_xlate_error(ret);
}

static u32 __efi_runtime invoke_psci_fn_hvc(ulong function, ulong arg0, ulong arg1, ulong arg2)
{
	struct arm_smccc_res res;
	arm_smccc_hvc(function, arg0, arg1, arg2, 0, 0, 0, 0, &res);
	return res.a0;
}

static u32 __efi_runtime invoke_psci_fn_smc(ulong function, ulong arg0, ulong arg1, ulong arg2)
{
	struct arm_smccc_res res;
	arm_smccc_smc(function, arg0, arg1, arg2, 0, 0, 0, 0, &res);
	return res.a0;
}

static int of_psci_do_fixup(struct device_node *root, void *method)
{
	return of_psci_fixup(root, version, (const void *)method);
}

static int __init psci_probe(struct device *dev)
{
	struct param_d *param;
	const char *method;
	ulong of_version, actual_version;
	int ret;

	of_version = (uintptr_t)device_get_match_data(dev);
	if (!of_version)
		return -ENODEV;

	ret = of_property_read_string(dev->of_node, "method", &method);
	if (ret) {
		dev_warn(dev, "missing \"method\" property\n");
		return -ENXIO;
	}

	if (!strcmp(method, "hvc")) {
		psci_invoke_fn = invoke_psci_fn_hvc;
	} else if (!strcmp(method, "smc")) {
		psci_invoke_fn = invoke_psci_fn_smc;
	} else {
		pr_warn("invalid \"method\" property: %s\n", method);
		return -EINVAL;
	}


	if (of_version < ARM_PSCI_VER(0,2)) {
		version = of_version;

		dev_info(dev, "assuming version %u.%u\n",
			 version >> 16, version & 0xffff);
		dev_dbg(dev, "Not registering reset handler due to PSCI version\n");

		return 0;
	}

	psci_invoke(ARM_PSCI_0_2_FN_PSCI_VERSION, 0, 0, 0, &actual_version);
	version = actual_version;

	param = dev_add_param_fixed(dev, "version", "%u.%u",
				    version >> 16, version & 0xffff);
	if (!IS_ERR_OR_NULL(param))
		dev_info(dev, "detected version %s\n", get_param_value(param));

	dev_add_param_fixed(dev, "method", "%s", method);

	if (actual_version != of_version)
		of_register_fixup(of_psci_do_fixup, (void *)method);

	poweroff.name = "psci";
	poweroff.poweroff = psci_poweroff;
	poweroff.rt_poweroff = rt_psci_poweroff;
	ret = poweroff_handler_register(&poweroff);
	if (ret)
		dev_warn(dev, "error registering poweroff handler: %pe\n",
			 ERR_PTR(ret));

	restart.name = "psci";
	restart.restart = psci_restart;
	restart.rt_restart = rt_psci_restart;
	restart.priority = 400;

	ret = restart_handler_register(&restart);
	if (ret) {
		dev_warn(dev, "error registering restart handler: %pe\n",
			 ERR_PTR(ret));
		return ret;
	}

	dev_add_alias(dev, "psci");

	return 0;
}

static __maybe_unused struct of_device_id psci_dt_ids[] = {
	{ .compatible = "arm,psci",	.data = (void*)ARM_PSCI_VER(0,1) },
	{ .compatible = "arm,psci-0.2",	.data = (void*)ARM_PSCI_VER(0,2) },
	{ .compatible = "arm,psci-1.0",	.data = (void*)ARM_PSCI_VER(1,0) },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, psci_dt_ids);

static struct driver psci_driver = {
	.name = "psci",
	.probe = psci_probe,
	.of_compatible = DRV_OF_COMPAT(psci_dt_ids),
};
coredevice_platform_driver(psci_driver);
