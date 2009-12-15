#include <common.h>

int init_board_data(bd_t *bd)
{
	cpu_init_board_data(bd);

	bd->bi_memstart  = CFG_SDRAM_BASE;	/* start of  DRAM memory	*/
//	bd->bi_memsize   = gd->ram_size;	/* size  of  DRAM memory in bytes */ /* FIXME */

#ifdef CONFIG_IP860
	bd->bi_sramstart = SRAM_BASE;	/* start of  SRAM memory	*/
	bd->bi_sramsize  = SRAM_SIZE;	/* size  of  SRAM memory	*/
#else
	bd->bi_sramstart = 0;		/* FIXME */ /* start of  SRAM memory	*/
	bd->bi_sramsize  = 0;		/* FIXME */ /* size  of  SRAM memory	*/
#endif

#if defined(CONFIG_8xx) || defined(CONFIG_8260) || defined(CONFIG_5xx) || \
    defined(CONFIG_E500) || defined(CONFIG_MPC86xx)
	bd->bi_immr_base = CFG_IMMR;	/* base  of IMMR register     */
#endif

#ifdef CONFIG_LYNXOS
	bd->bi_bootflags = bootflag;	/* boot / reboot flag (for LynxOS)    */
#endif

#if defined(CONFIG_CPM2)
	bd->bi_cpmfreq = gd->cpm_clk;
	bd->bi_brgfreq = gd->brg_clk;
	bd->bi_sccfreq = gd->scc_clk;
	bd->bi_vco     = gd->vco_out;
#endif /* CONFIG_CPM2 */
//	bd->bi_baudrate = gd->baudrate;	/* Console Baudrate     */

#ifdef CFG_EXTBDINFO
	strncpy ((char *)bd->bi_s_version, "1.2", sizeof (bd->bi_s_version));
	strncpy ((char *)bd->bi_r_version, BAREBOX_VERSION, sizeof (bd->bi_r_version));

	bd->bi_procfreq = gd->cpu_clk;	/* Processor Speed, In Hz */
	bd->bi_plb_busfreq = gd->bus_clk;
#if defined(CONFIG_405GP) || defined(CONFIG_405EP) || defined(CONFIG_440EP) || defined(CONFIG_440GR)
	bd->bi_pci_busfreq = get_PCI_freq ();
	bd->bi_opbfreq = get_OPB_freq ();
#elif defined(CONFIG_XILINX_ML300)
	bd->bi_pci_busfreq = get_PCI_freq ();
#endif
#endif
	return 0;
}
