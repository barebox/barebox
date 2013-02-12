/*
 * Copyright (C) 2013 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * GPLv2 only
 */

#ifndef __AMBA_SP804_H__
#define __AMBA_SP804_H__

#include <linux/amba/bus.h>
#include <sizes.h>

#define AMBA_ARM_SP804_ID	0x00141804
#define AMBA_ARM_SP804_ID_MASK	0x00ffffff

static inline bool amba_is_arm_sp804(void __iomem *base)
{
	u32 pid, cid;
	u32 size = SZ_4K;

	cid = amba_device_get_cid(base, size);

	if (cid != AMBA_CID)
		return false;

	pid = amba_device_get_pid(base, size);

	return (pid & AMBA_ARM_SP804_ID_MASK) == AMBA_ARM_SP804_ID;
}
#endif /* __AMBA_SP804_H__ */
