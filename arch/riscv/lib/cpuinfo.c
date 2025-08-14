// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <command.h>
#include <asm/sbi.h>
#include <asm/system.h>
#include <structio.h>

static const char *implementations[] = {
	[0] = "Berkeley Boot Loader (BBL)",
	[1] = "OpenSBI",
	[2] = "Xvisor",
	[3] = "KVM",
	[4] = "RustSBI",
	[5] = "Diosix",
};

static const char *modes[] = {
	[RISCV_U_MODE] = "U",
	[RISCV_S_MODE] = "S",
	[RISCV_HS_MODE] = "HS",
	[RISCV_M_MODE] = "M",
};

static int do_cpuinfo(int argc, char *argv[])
{
	const char *implementation = NULL;
	enum riscv_mode mode;
	unsigned long impid;

	mode = riscv_mode() & RISCV_MODE_MASK;

	stprintf("Architecture", "%s",
		 IS_ENABLED(CONFIG_ARCH_RV64I) ? "RV64I" : "RV32I");
	stprintf("Mode", "%s", modes[mode]);

	switch (mode) {
	case RISCV_S_MODE:
		stprintf("Hart ID", "%lu", riscv_hartid());
		if (!IS_ENABLED(CONFIG_RISCV_SBI))
			break;
		stprintf("SBI version", "v%lu.%lu",
		       sbi_major_version(), sbi_minor_version());

		if (sbi_spec_is_0_1())
			return 0;

		impid = __sbi_base_ecall(SBI_EXT_BASE_GET_IMP_ID);
		if (impid < ARRAY_SIZE(implementations))
			implementation = implementations[impid];

		stprintf("SBI implementation ID", "0x%lx", impid);
		if (implementation)
			stprintf("SBI implementation name", "%s", implementation);

		stprintf("SBI implementation version", "0x%lx",
			 __sbi_base_ecall(SBI_EXT_BASE_GET_IMP_VERSION));

		stprintf("SBI machine VENDORID", "0x%lx",
			 __sbi_base_ecall(SBI_EXT_BASE_GET_MVENDORID));

		stprintf("SBI machine ARCHID", "0x%lx",
			 __sbi_base_ecall(SBI_EXT_BASE_GET_MARCHID));

		stprintf("SBI machine MIMPID", "0x%lx",
		       __sbi_base_ecall(SBI_EXT_BASE_GET_MIMPID));
		break;
	default:
		break;
	}

	return 0;
}

BAREBOX_CMD_START(cpuinfo)
	.cmd            = do_cpuinfo,
	BAREBOX_CMD_DESC("show CPU information")
	BAREBOX_CMD_GROUP(CMD_GRP_INFO)
BAREBOX_CMD_END
