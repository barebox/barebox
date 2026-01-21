/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2025 Pengutronix, Michael Tretter <m.tretter@pengutronix.de>
 */

#ifndef __SOC_ROCKCHIP_SECURE_BOOT_H
#define __SOC_ROCKCHIP_SECURE_BOOT_H

#include <linux/types.h>

struct rk_secure_boot_info {
	bool lockdown;
	bool simulation;
	u8 hash[32];
};

int rk_secure_boot_get_info(struct rk_secure_boot_info *info);
int rk_secure_boot_burn_hash(const u8 *hash, u32 key_size_bits);
int rk_secure_boot_lockdown_device(void);

#endif /* __SOC_ROCKCHIP_SECURE_BOOT_H */
