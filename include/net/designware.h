#ifndef __DWC_UNIMAC_H
#define __DWC_UNIMAC_H

struct dwc_ether_platform_data {
	u8 phy_addr;
	void (*fix_mac_speed)(int speed);
};

#endif
