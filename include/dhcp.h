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
};

int dhcp(int retries, struct dhcp_req_param *param);

#endif
