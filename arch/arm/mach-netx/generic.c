/*
 * See file CREDITS for list of people who contributed to this
 * project.
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
 */

#include <common.h>
#include <command.h>
#include <io.h>
#include <mach/netx-regs.h>
#include "eth_firmware.h"

struct fw_header {
	unsigned int magic;
	unsigned int type;
	unsigned int version;
	unsigned int table_size;
	unsigned int reserved[4];
} __attribute__((packed));

static int xc_check_ptr(int xcno, unsigned long adr, unsigned int size)
{
	if (adr >= NETX_PA_XMAC(xcno) &&
	    adr + size < NETX_PA_XMAC(xcno) + XMAC_MEM_SIZE)
		return 0;

	if (adr >= NETX_PA_XPEC(xcno) &&
	    adr + size < NETX_PA_XPEC(xcno) + XPEC_MEM_SIZE)
		return 0;

	printf("%s: illegal pointer 0x%08lx\n", __func__ ,adr);
	return -1;
}

static int xc_patch(int xcno, const u32 *patch, int count)
{
	unsigned int adr, val;

	int i;
	for (i = 0; i < count; i++) {
		adr = *patch++;
		val = *patch++;
		if (xc_check_ptr(xcno, adr, 1) < 0)
			return -1;
		writel(val, adr);
	}
	return 0;
}

static void memset32(void *s, int c, int n)
{
	int i;
	u32 *t = s;

	for (i = 0; i < (n >> 2); i++)
		*t++ = 0;
}

static void memcpy32(void *trg, const void *src, int size)
{
	int i;
	u32 *t = trg;
	const u32 *s = src;
	for (i = 0; i < (size >> 2); i++)
		*t++ = *s++;
}

int loadxc(int xcno)
{
	/* stop xmac / xpec */
	XMAC_REG(xcno, XMAC_RPU_HOLD_PC) = RPU_HOLD_PC;
	XMAC_REG(xcno, XMAC_TPU_HOLD_PC) = TPU_HOLD_PC;
	XPEC_REG(xcno, XPEC_XPU_HOLD_PC) = XPU_HOLD_PC;

	XPEC_REG(xcno, XPEC_PC) = 0;

	/* load firmware */
	memset32((void*)NETX_PA_XPEC(xcno) + XPEC_RAM_START, 0, 0x2000);
	memset32((void*)NETX_PA_XMAC(xcno), 0, 0x800);

	/* can't use barebox memcpy here, we need 32bit accesses */
	if (xcno == 0) {
		memcpy32((void*)(NETX_PA_XMAC(xcno) + XMAC_RPU_PROGRAM_START), rpu_eth0, sizeof(rpu_eth0));
		memcpy32((void*)(NETX_PA_XMAC(xcno) + XMAC_TPU_PROGRAM_START), tpu_eth0, sizeof(tpu_eth0));
		memcpy32((void*)NETX_PA_XPEC(xcno) + XPEC_RAM_START, xpec_eth0_mac, sizeof(xpec_eth0_mac));
		xc_patch(xcno, rpu_eth0_patch, ARRAY_SIZE(rpu_eth0_patch) >> 1);
		xc_patch(xcno, tpu_eth0_patch, ARRAY_SIZE(tpu_eth0_patch) >> 1);
		xc_patch(xcno, xpec_eth0_mac_patch, ARRAY_SIZE(xpec_eth0_mac_patch) >> 1);
	} else {
		memcpy32((void*)(NETX_PA_XMAC(xcno) + XMAC_RPU_PROGRAM_START), rpu_eth1, sizeof(rpu_eth1));
		memcpy32((void*)(NETX_PA_XMAC(xcno) + XMAC_TPU_PROGRAM_START), tpu_eth1, sizeof(tpu_eth1));
		memcpy32((void*)NETX_PA_XPEC(xcno) + XPEC_RAM_START, xpec_eth1_mac, sizeof(xpec_eth1_mac));
		xc_patch(xcno, rpu_eth1_patch, ARRAY_SIZE(rpu_eth1_patch) >> 1);
		xc_patch(xcno, tpu_eth1_patch, ARRAY_SIZE(tpu_eth1_patch) >> 1);
		xc_patch(xcno, xpec_eth1_mac_patch, ARRAY_SIZE(xpec_eth1_mac_patch) >> 1);
	}

	/* start xmac / xpec */
	XPEC_REG(xcno, XPEC_XPU_HOLD_PC) = 0;
	XMAC_REG(xcno, XMAC_TPU_HOLD_PC) = 0;
	XMAC_REG(xcno, XMAC_RPU_HOLD_PC) = 0;

	return 0;
}

int do_loadxc(int argc, char *argv[])
{
	int xcno;

	if (argc < 2)
		goto failure;

	xcno = simple_strtoul(argv[1], NULL, 16);

	printf("loading xc%d\n",xcno);

	/* There is a bug in the netx internal firmware. For now we have to call this twice */
	loadxc(xcno);
	loadxc(xcno);

	return 0;

failure:
	return COMMAND_ERROR_USAGE;
}

void __noreturn reset_cpu(unsigned long addr)
{
	SYSTEM_REG(SYSTEM_RES_CR) = 0x01000008;

	/* Not reached */
	while (1);
}


BAREBOX_CMD_START(loadxc)
	.cmd		= do_loadxc,
	.usage		= "load xmac/xpec engine with ethernet firmware",
BAREBOX_CMD_END

