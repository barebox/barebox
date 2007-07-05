#ifndef __INCLUDE_ASM_ARCH_COMMON_H
#define __INCLUDE_ASM_ARCH_COMMON_H

ulong	get_OPB_freq (void);
ulong	get_PCI_freq (void);
void	cpu_init_f    (void);

#if defined(CONFIG_4xx) || defined(CONFIG_IOP480)
#  if defined(CONFIG_440)
    typedef PPC440_SYS_INFO sys_info_t;
#	if defined(CONFIG_440SPE)
	 unsigned long determine_sysper(void);
	 unsigned long determine_pci_clock_per(void);
	 int ppc440spe_revB(void);
#	endif
#  else
    typedef PPC405_SYS_INFO sys_info_t;
#  endif
void	get_sys_info  ( sys_info_t * );
#endif

#endif  /* __INCLUDE_ASM_ARCH_COMMON_H */
