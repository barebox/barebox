// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2023 Ahmad Fatoum <a.fatoum@pengutronix.de>, Pengutronix
/*
 * mmuinfo_64.c - Show MMU/cache information via AT instruction
 */

#include <common.h>
#include <asm/mmuinfo.h>
#include <asm/system.h>
#include <asm/sysreg.h>
#include <linux/bitfield.h>

#define at_par(reg, addr) ({ \
		asm volatile("at " reg ", %0\n" :: "r" (addr)); \
		isb(); \
		read_sysreg_par(); \
})

#define BITS(from, to, val) FIELD_GET(GENMASK(from, to), val)

static const char *decode_devmem_attr(u8 attr)
{
	switch (attr & ~0x1) {
	case 0b00000000:
		return "0b0000 Device-nGnRnE memory";
	case 0b00000100:
		return "0b0100 Device-nGnRE memory";
	case 0b00001000:
		return "0b1000 Device-nGRE memory";
	case 0b00001100:
		return "0b1100 Device-GRE memory";
	default:
		return "unknown";
	};
}

static char *cache_attr[] = {
	"0b0000 Wrongly decoded",
	"0b0001 Write-Through Transient, Write-Allocate, no Read-Allocate",
	"0b0010 Write-Through Transient, no Write-Allocate",
	"0b0011 Write-Through Transient, Write-Allocate",
	"0b0100 Non-Cacheable",
	"0b0101 Write-Back Transient, Write-Allocate, no Read-Allocate",
	"0b0110 Write-Back Transient, no Write-Allocate",
	"0b0111 Write-Back Transient, Write-Allocate",
	"0b1000 Write-Through Non-transient, no Write-Allocate no Read-Allocate",
	"0b1001 Write-Through Non-transient, Write-Allocate no Read-Allocate",
	"0b1010 Write-Through Non-transient, no Write-Allocate",
	"0b1011 Write-Through Non-transient, Write-Allocate",
	"0b1100 Write-Back Non-transient, no Write-Allocate no Read-Allocate",
	"0b1101 Write-Back Non-transient, Write-Allocate no Read-Allocate",
	"0b1110 Write-Back Non-transient, no Write-Allocate",
	"0b1111 Write-Back Non-transient, Write-Allocate",
};

static char *share_attr[] = {
	"0b00 Non-Shareable",
	"0b01 Reserved",
	"0b10 Outer Shareable",
	"0b11 Inner Shareable",
};

static char *stage_fault[] = {
	"stage 1 translation",
	"stage 2 translation",
};

static char *fault_status_leveled[] = {
	"Address size fault",  /* of translation or translation table base register */
	"Translation fault",
	"Access flag fault",
	"Permission fault",
	"Synchronous External abort", /* level -1 */
	"Synchronous External abort", /* on translation table walk or hardware update of translation table */
	"Synchronous parity or ECC error", /* level -1 */
	"Synchronous parity or ECC error", /* on memory access on translation table walk or hardware update of translation table */
};

static const char *decode_fault_status_level(u8 fst)
{
	if (!(fst & BIT(5)))
		return "";

	switch (BITS(5, 0, fst)) {
	case 0b010011:
	case 0b011011:
		return ", level -1";
	}

	switch (BITS(1, 0, fst)) {
	case 0b00:
		return ", level 0";
	case 0b01:
		return ", level 1";
	case 0b10:
		return ", level 2";
	case 0b11:
		return ", level 3";
	}

	BUG();
}

static const char *decode_fault_status(u8 fst)
{

	switch (BITS(5, 0, fst)) {
	case 0b101001: /* When FEAT_LPA2 is implemented */
		return "Address size fault, level -1";
	case 0b101011: /* When FEAT_LPA2 is implemented */
		return "Translation fault, level -1";
	case 0b110000:
		return "TLB conflict abort";
	case 0b110001: /* When FEAT_HAFDBS is implemented */
		return "Unsupported atomic hardware update fault";
	case 0b111101: /* When EL1 is capable of using AArch32 */
		return "Section Domain fault, from an AArch32 stage 1 EL1&0 "
			"translation regime using Short-descriptor translation "
			"table format";
	case 0b111110: /* When EL1 is capable of using AArch32 */
		return "Page Domain fault, from an AArch32 stage 1 EL1&0 "
			"translation regime using Short-descriptor translation "
			"table format";
	default:
		if (fst & BIT(5))
			return fault_status_leveled[BITS(4, 2, fst)];

		return "Reserved";
	}
};

static void decode_par(unsigned long par)
{
	u8 devmem_attr = BITS(63, 56, par);

	if (par & 1) {
		printf("  Translation aborted [9:8]: because of a fault in the %s%s\n",
		       stage_fault[BITS(9, 9, par)],
		       BITS(8, 8, par) ? " during a stage 1 translation table walk" : "");
		printf("  Fault Status Code [6:1]:   0x%02lx (%s%s)\n", BITS(6, 1, par),
		       decode_fault_status(BITS(6, 1, par)),
		       decode_fault_status_level(BITS(6, 1, par)));
		printf("  Failure [0]:                0x1\n");
	} else {
		if ((devmem_attr & 0xf0) && (devmem_attr & 0x0f)) {
			printf("  Outer mem. attr. [63:60]:   0x%02lx (%s)\n", BITS(63, 60, par),
			       cache_attr[BITS(63, 60, par)]);
			printf("  Inner mem. attr. [59:56]:   0x%02lx (%s)\n", BITS(59, 56, par),
			       cache_attr[BITS(59, 56, par)]);
		} else if ((devmem_attr & 0b11110010) == 0) {
			printf("  Memory attr. [63:56]:     0x%02x (%s)\n",
			       devmem_attr, decode_devmem_attr(devmem_attr));
			if (devmem_attr & 1)
				printf("  (XS == 0 if FEAT_XS implemented)\n");
		} else if (devmem_attr == 0b01000000) {
			printf("  Outer mem. attr. [63:56]:   0x%02lx (%s)\n", BITS(63, 56, par),
			       "Non-Cacheable");
			printf("  Inner mem. attr. [63:56]:   0x%02lx (%s)\n", BITS(63, 56, par),
			       "Non-Cacheable");
			printf("  (XS == 0 if FEAT_XS implemented)\n");
		} else if (devmem_attr == 0b10100000) {
			printf("  Outer mem. attr. [63:56]:   0x%02lx (%s)\n", BITS(63, 56, par),
			       "Write-Through, No Write-Allocate");
			printf("  Inner mem. attr. [63:56]:   0x%02lx (%s)\n", BITS(63, 56, par),
			       "Write-Through");
			printf("  (XS == 0 if FEAT_XS implemented)\n");
		} else if (devmem_attr == 0b11110000) {
			printf("  Outer mem. attr. [63:56]:   0x%02lx (%s)\n", BITS(63, 56, par),
			       "Write-Back");
			printf("  Inner mem. attr. [63:56]:   0x%02lx (%s)\n", BITS(63, 56, par),
			       "Write-Back, Write-Allocate, Non-transient");
			printf("  (if FEAT_MTE2 implemented)\n");
		}
		printf("  Physical Address [51:12]: 0x%08lx\n", par & GENMASK(51, 12));
		printf("  Non-Secure [9]:           0x%lx\n", BITS(9, 9, par));
		printf("  Shareability attr. [8:7]: 0x%02lx (%s)\n", BITS(8, 7, par),
			share_attr[BITS(8, 7, par)]);
		printf("  Failure [0]:              0x0\n");
	}
}

int mmuinfo_v8(void *_addr)
{
	unsigned long addr = (unsigned long)_addr;
	unsigned long priv_read, priv_write;

	switch (current_el()) {
	case 3:
		priv_read  = at_par("s1e3r", addr);
		priv_write = at_par("s1e3w", addr);
		break;
	case 2:
		priv_read  = at_par("s1e2r", addr);
		priv_write = at_par("s1e2w", addr);
		break;
	case 1:
		priv_read  = at_par("s1e1r", addr);
		priv_write = at_par("s1e1w", addr);
		break;
	case 0:
		priv_read  = at_par("s1e0r", addr);
		priv_write = at_par("s1e0w", addr);
		break;
	default:
		return -EINVAL;
	}

	printf("PAR result for 0x%08lx: \n", addr);
	printf(" privileged read: 0x%08lx\n", priv_read);
	decode_par(priv_read);
	printf(" privileged write: 0x%08lx\n", priv_write);
	decode_par(priv_write);

	return 0;
}
