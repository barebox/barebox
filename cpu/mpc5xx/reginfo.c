#include <common.h>

void reginfo(void)
{
	volatile immap_t     	*immap  = (immap_t *)CFG_IMMR;
	volatile memctl5xx_t	*memctl = &immap->im_memctl;
	volatile sysconf5xx_t	*sysconf = &immap->im_siu_conf;
	volatile sit5xx_t	*timers = &immap->im_sit;
	volatile car5xx_t	*car = &immap->im_clkrst;
	volatile uimb5xx_t	*uimb = &immap->im_uimb;

	puts ("\nSystem Configuration registers\n");
	printf("\tIMMR\t0x%08X\tSIUMCR\t0x%08X \n", get_immr(0), sysconf->sc_siumcr);
	printf("\tSYPCR\t0x%08X\tSWSR\t0x%04X \n" ,sysconf->sc_sypcr, sysconf->sc_swsr);
	printf("\tSIPEND\t0x%08X\tSIMASK\t0x%08X \n", sysconf->sc_sipend, sysconf->sc_simask);
	printf("\tSIEL\t0x%08X\tSIVEC\t0x%08X \n", sysconf->sc_siel, sysconf->sc_sivec);
	printf("\tTESR\t0x%08X\n", sysconf->sc_tesr);

	puts ("\nMemory Controller Registers\n");
	printf("\tBR0\t0x%08X\tOR0\t0x%08X \n", memctl->memc_br0, memctl->memc_or0);
	printf("\tBR1\t0x%08X\tOR1\t0x%08X \n", memctl->memc_br1, memctl->memc_or1);
	printf("\tBR2\t0x%08X\tOR2\t0x%08X \n", memctl->memc_br2, memctl->memc_or2);
	printf("\tBR3\t0x%08X\tOR3\t0x%08X \n", memctl->memc_br3, memctl->memc_or3);
	printf("\tDMBR\t0x%08X\tDMOR\t0x%08X \n", memctl->memc_dmbr, memctl->memc_dmor );
	printf("\tMSTAT\t0x%08X\n", memctl->memc_mstat);

	puts ("\nSystem Integration Timers\n");
	printf("\tTBSCR\t0x%08X\tRTCSC\t0x%08X \n", timers->sit_tbscr, timers->sit_rtcsc);
	printf("\tPISCR\t0x%08X \n", timers->sit_piscr);

	puts ("\nClocks and Reset\n");
	printf("\tSCCR\t0x%08X\tPLPRCR\t0x%08X \n", car->car_sccr, car->car_plprcr);

	puts ("\nU-Bus to IMB3 Bus Interface\n");
	printf("\tUMCR\t0x%08X\tUIPEND\t0x%08X \n", uimb->uimb_umcr, uimb->uimb_uipend);
	puts ("\n\n");
}
