// SPDX-License-Identifier: GPL-2.0

#include <common.h>
#include <command.h>
#include <getopt.h>

#include <asm/psci.h>
#include <asm/secure.h>
#include <linux/arm-smccc.h>

void second_entry(void)
{
	struct arm_smccc_res res;

	psci_printf("2nd CPU online, now turn off again\n");

	arm_smccc_smc(ARM_PSCI_0_2_FN_CPU_OFF,
		      0, 0, 0, 0, 0, 0, 0, &res);

	psci_printf("2nd CPU still alive?\n");

	while (1);
}

static const char *psci_xlate_str(long err)
{
       static char errno_string[sizeof "error 0x123456789ABCDEF0"];

       switch(err)
       {
       case ARM_PSCI_RET_SUCCESS:
	       return "Success";
       case ARM_PSCI_RET_NOT_SUPPORTED:
	       return "Operation not supported";
       case ARM_PSCI_RET_INVAL:
	       return "Invalid argument";
       case ARM_PSCI_RET_DENIED:
	       return "Operation not permitted";
       case ARM_PSCI_RET_ALREADY_ON:
	       return "CPU already on";
       case ARM_PSCI_RET_ON_PENDING:
	       return "CPU_ON in progress";
       case ARM_PSCI_RET_INTERNAL_FAILURE:
	       return "Internal failure";
       case ARM_PSCI_RET_NOT_PRESENT:
	       return "Trusted OS not present on core";
       case ARM_PSCI_RET_DISABLED:
	       return "CPU is disabled";
       case ARM_PSCI_RET_INVALID_ADDRESS:
	       return "Bad address";
       }

       sprintf(errno_string, "error 0x%lx", err);
       return errno_string;
}

static int do_smc(int argc, char *argv[])
{
	long ret;
	int opt;
	struct arm_smccc_res res = {
		.a0 = 0xdeadbee0,
		.a1 = 0xdeadbee1,
		.a2 = 0xdeadbee2,
		.a3 = 0xdeadbee3,
	};

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	while ((opt = getopt(argc, argv, "nic")) > 0) {
		switch (opt) {
		case 'n':
			armv7_secure_monitor_install();
			break;
		case 'i':
			arm_smccc_smc(ARM_PSCI_0_2_FN_PSCI_VERSION,
				      0, 0, 0, 0, 0, 0, 0, &res);
			printf("found psci version %ld.%ld\n", res.a0 >> 16, res.a0 & 0xffff);
			break;
		case 'c':
			arm_smccc_smc(ARM_PSCI_0_2_FN_CPU_ON,
				      1, (unsigned long)second_entry, 0, 0, 0, 0, 0, &res);
			ret = (long)res.a0;
			printf("CPU_ON returns with: %s\n", psci_xlate_str(ret));
			if (ret)
				return COMMAND_ERROR;
		}
	}


	return 0;
}
BAREBOX_CMD_HELP_START(smc)
BAREBOX_CMD_HELP_TEXT("Secure monitor code test command")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT ("-n",  "Install secure monitor and switch to nonsecure mode")
BAREBOX_CMD_HELP_OPT ("-i",  "Show information about installed PSCI version")
BAREBOX_CMD_HELP_OPT ("-c",  "Start secondary CPU core")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(smc)
	.cmd = do_smc,
	BAREBOX_CMD_DESC("secure monitor test command")
	BAREBOX_CMD_HELP(cmd_smc_help)
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
BAREBOX_CMD_END
