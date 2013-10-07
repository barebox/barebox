#ifndef __DWC_UNIMAC_H
#define __DWC_UNIMAC_H

#include <linux/phy.h>

struct dwc_ether_platform_data {
	int phy_addr;
	phy_interface_t interface;
	void (*fix_mac_speed)(int speed);
	bool enh_desc;	/* use Alternate/Enhanced Descriptor configurations */
};

#endif
