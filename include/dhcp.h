/*
 * Copyright (C) 2015 PHYTEC Messtechnik GmbH,
 * Author: Wadim Egorov <w.egorov@phytec.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __DHCP_H__
#define __DHCP_H__

#define DHCP_DEFAULT_RETRY 20

struct dhcp_req_param {
	char *hostname;
	char *vendor_id;
	char *client_id;
	char *user_class;
	char *client_uuid;
	char *option224;
	int retries;
};

struct dhcp_result {
	IPaddr_t ip;
	IPaddr_t netmask;
	IPaddr_t gateway;
	IPaddr_t nameserver;
	IPaddr_t serverip;
	IPaddr_t dhcp_serverip;
	char *hostname;
	char *domainname;
	char *rootpath;
	char *devicetree;
	char *bootfile;
	char *tftp_server_name;
	uint32_t leasetime;
};

struct eth_device;

int dhcp_request(struct eth_device *edev, const struct dhcp_req_param *param,
		 struct dhcp_result **res);
int dhcp_set_result(struct eth_device *edev, struct dhcp_result *res);
void dhcp_result_free(struct dhcp_result *res);
int dhcp(struct eth_device *edev, const struct dhcp_req_param *param);

#endif
