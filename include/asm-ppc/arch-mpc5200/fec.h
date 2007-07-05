#ifndef __INCLUDE_ASM_ARCH_FEC_H
#define __INCLUDE_ASM_ARCH_FEC_H

struct mpc5xxx_fec_platform_data {
        ulong  xcv_type;
};

typedef enum {
	SEVENWIRE,			/* 7-wire       */
	MII10,				/* MII 10Mbps   */
	MII100				/* MII 100Mbps  */
} xceiver_type;

#endif /* __INCLUDE_ASM_ARCH_FEC_H */
