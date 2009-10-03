#ifndef __AT91SAM_ETHER_H
#define __AT91SAM_ETHER_H

#define AT91SAM_ETHER_MII		(0 << 0)
#define AT91SAM_ETHER_RMII		(1 << 0)
#define AT91SAM_ETHER_FORCE_LINK	(1 << 1)

struct at91sam_ether_platform_data {
	unsigned int flags;
	int phy_addr;
};

#endif /* __AT91SAM_ETHER_H */
