#ifndef __NET_DAVINCI_EMAC_H__
#define __NET_DAVINCI_EMAC_H__

struct davinci_emac_platform_data {
	int phy_addr;
	bool force_link;
	bool interface_rmii;
};

#endif /* __NET_DAVINCI_EMAC_H__ */
