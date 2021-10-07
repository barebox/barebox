// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Protonic Holland
 * Copyright (C) 2020 Oleksij Rempel, Pengutronix
 */

#include <asm/barebox-arm.h>
#include <common.h>
#include <mach/esdctl.h>
#include <mach/generic.h>

extern char __dtb_z_imx6q_prti6q_start[];
extern char __dtb_z_imx6q_prtwd2_start[];
extern char __dtb_z_imx6q_vicut1_start[];
extern char __dtb_z_imx6dl_alti6p_start[];
extern char __dtb_z_imx6dl_lanmcu_start[];
extern char __dtb_z_imx6dl_plybas_start[];
extern char __dtb_z_imx6dl_plym2m_start[];
extern char __dtb_z_imx6dl_prtmvt_start[];
extern char __dtb_z_imx6dl_prtrvt_start[];
extern char __dtb_z_imx6dl_prtvt7_start[];
extern char __dtb_z_imx6dl_victgo_start[];
extern char __dtb_z_imx6dl_vicut1_start[];
extern char __dtb_z_imx6qp_prtwd3_start[];
extern char __dtb_z_imx6qp_vicutp_start[];
extern char __dtb_z_imx6ul_prti6g_start[];
extern char __dtb_z_imx6ull_jozacp_start[];

ENTRY_FUNCTION(start_imx6q_prti6q, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_z_imx6q_prti6q_start + get_runtime_offset();

	imx6q_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_imx6q_prtwd2, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_z_imx6q_prtwd2_start + get_runtime_offset();

	imx6q_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_imx6q_vicut1, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_z_imx6q_vicut1_start + get_runtime_offset();

	imx6q_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_imx6dl_alti6p, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_z_imx6dl_alti6p_start + get_runtime_offset();

	imx6q_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_imx6dl_lanmcu, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_z_imx6dl_lanmcu_start + get_runtime_offset();

	imx6q_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_imx6dl_plybas, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_z_imx6dl_plybas_start + get_runtime_offset();

	imx6q_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_imx6dl_plym2m, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_z_imx6dl_plym2m_start + get_runtime_offset();

	imx6q_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_imx6dl_prtmvt, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_z_imx6dl_prtmvt_start + get_runtime_offset();

	imx6q_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_imx6dl_prtrvt, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_z_imx6dl_prtrvt_start + get_runtime_offset();

	imx6q_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_imx6dl_prtvt7, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_z_imx6dl_prtvt7_start + get_runtime_offset();

	imx6q_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_imx6dl_victgo, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_z_imx6dl_victgo_start + get_runtime_offset();

	imx6q_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_imx6dl_vicut1, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_z_imx6dl_vicut1_start + get_runtime_offset();

	imx6q_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_imx6qp_prtwd3, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_z_imx6qp_prtwd3_start + get_runtime_offset();

	imx6q_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_imx6qp_vicutp, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	fdt = __dtb_z_imx6qp_vicutp_start + get_runtime_offset();

	imx6q_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_imx6ul_prti6g, r0, r1, r2)
{
	void *fdt;

	imx6ul_cpu_lowlevel_init();

	fdt = __dtb_z_imx6ul_prti6g_start + get_runtime_offset();

	imx6ul_barebox_entry(fdt);
}

ENTRY_FUNCTION(start_imx6ull_jozacp, r0, r1, r2)
{
	void *fdt;

	imx6ul_cpu_lowlevel_init();

	/* Disconnect USDHC2 from SD card */
	writel(0x5, 0x020e0178);
	writel(0x5, 0x020e017c);
	writel(0x5, 0x020e0180);
	writel(0x5, 0x020e0184);
	writel(0x5, 0x020e0188);
	writel(0x5, 0x020e018c);

	fdt = __dtb_z_imx6ull_jozacp_start + get_runtime_offset();

	imx6ul_barebox_entry(fdt);
}
