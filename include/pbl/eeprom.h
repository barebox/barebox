/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __PBL_EEPROM_H_
#define __PBL_EEPROM_H_

#include <common.h>
#include <pbl/i2c.h>

static inline void eeprom_read(struct pbl_i2c *i2c, u16 client_addr, u32 addr, void *buf, u16 count)
{
	u8 msgbuf[2];
	struct i2c_msg msg[] = {
		{
			.addr = client_addr,
			.buf = msgbuf,
		}, {
			.addr = client_addr,
			.flags = I2C_M_RD,
			.buf = buf,
			.len = count,
		},
	};
	int ret, i = 0;

	if (addr & I2C_ADDR_16_BIT)
		msgbuf[i++] = addr >> 8;
	msgbuf[i++] = addr;
	msg[0].len = i;

	ret = pbl_i2c_xfer(i2c, msg, ARRAY_SIZE(msg));
	if (ret != ARRAY_SIZE(msg))
		pr_err("Failed to read from eeprom@%x: %d\n", client_addr, ret);
}

#endif
