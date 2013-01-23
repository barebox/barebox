/*
 * Copyright (C) 2012-2013 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 */

#ifndef __HW_REVISION_H__
#define __HW_REVISION_H__

enum vendor_id {
	VENDOR_UNKNOWN	= 0,
	VENDOR_ATMEL	= 1,
	VENDOR_FLEX	= 2,
};

void at91sam9m10ihd_devices_detect_hw(void);

bool at91sam9m10ihd_cm_is_vendor(enum vendor_id vid);
bool at91sam9m10ihd_db_is_vendor(enum vendor_id vid);

#endif /* __HW_REVISION_H__ */
