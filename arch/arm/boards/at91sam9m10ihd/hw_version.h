// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2012-2013 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>

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
