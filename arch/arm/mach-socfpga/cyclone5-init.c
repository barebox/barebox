#include <debug_ll.h>
#include <common.h>
#include <init.h>
#include <io.h>
#include <mach/cyclone5-freeze-controller.h>
#include <mach/cyclone5-system-manager.h>
#include <mach/cyclone5-clock-manager.h>
#include <mach/cyclone5-reset-manager.h>
#include <mach/cyclone5-scan-manager.h>
#include <mach/generic.h>

void socfpga_lowlevel_init(struct socfpga_cm_config *cm_config,
			   struct socfpga_io_config *io_config)
{
	uint32_t val;

	val = 0xffffffff;
	val &= ~(1 << RSTMGR_PERMODRST_L4WD0_LSB);
	val &= ~(1 << RSTMGR_PERMODRST_OSC1TIMER0_LSB);
	writel(val, CYCLONE5_RSTMGR_ADDRESS + RESET_MGR_PER_MOD_RESET_OFS);

	/* freeze all IO banks */
	sys_mgr_frzctrl_freeze_req(FREEZE_CHANNEL_0);
	sys_mgr_frzctrl_freeze_req(FREEZE_CHANNEL_1);
	sys_mgr_frzctrl_freeze_req(FREEZE_CHANNEL_2);
	sys_mgr_frzctrl_freeze_req(FREEZE_CHANNEL_3);

	writel(~0, CYCLONE5_RSTMGR_ADDRESS + RESET_MGR_BRG_MOD_RESET_OFS);

	debug("Reconfigure Clock Manager\n");

	/* reconfigure the PLLs */
	socfpga_cm_basic_init(cm_config);

	debug("Configure IOCSR\n");
	/* configure the IOCSR through scan chain */
	scan_mgr_io_scan_chain_prg(IO_SCAN_CHAIN_0, CONFIG_HPS_IOCSR_SCANCHAIN0_LENGTH, io_config->iocsr_emac_mixed2);
	scan_mgr_io_scan_chain_prg(IO_SCAN_CHAIN_1, CONFIG_HPS_IOCSR_SCANCHAIN1_LENGTH, io_config->iocsr_mixed1_flash);
	scan_mgr_io_scan_chain_prg(IO_SCAN_CHAIN_2, CONFIG_HPS_IOCSR_SCANCHAIN2_LENGTH, io_config->iocsr_general);
	scan_mgr_io_scan_chain_prg(IO_SCAN_CHAIN_3, CONFIG_HPS_IOCSR_SCANCHAIN3_LENGTH, io_config->iocsr_ddr);

	/* configure the pin muxing through system manager */
	socfpga_sysmgr_pinmux_init(io_config->pinmux, io_config->num_pin);

	writel(RSTMGR_PERMODRST_L4WD0 | RSTMGR_PERMODRST_L4WD1,
			CYCLONE5_RSTMGR_ADDRESS + RESET_MGR_PER_MOD_RESET_OFS);

	/* unfreeze / thaw all IO banks */
	sys_mgr_frzctrl_thaw_req(FREEZE_CHANNEL_0);
	sys_mgr_frzctrl_thaw_req(FREEZE_CHANNEL_1);
	sys_mgr_frzctrl_thaw_req(FREEZE_CHANNEL_2);
	sys_mgr_frzctrl_thaw_req(FREEZE_CHANNEL_3);

	writel(0x18, CYCLONE5_L3REGS_ADDRESS);
	writel(0x1, 0xfffefc00);

	INIT_LL();
}
