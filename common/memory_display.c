#include <common.h>
#include <errno.h>
#include <abort.h>

#define DISP_LINE_LEN	16

int memory_display(const void *addr, loff_t offs, unsigned nbytes, int size, int swab)
{
	ulong linebytes, i;
	u_char	*cp;

	/* Print the lines.
	 *
	 * We buffer all read data, so we can make sure data is read only
	 * once, and all accesses are with the specified bus width.
	 */
	do {
		char linebuf[DISP_LINE_LEN];
		uint32_t *uip = (uint   *)linebuf;
		uint16_t *usp = (ushort *)linebuf;
		uint8_t *ucp = (u_char *)linebuf;
		unsigned count = 52;

		printf("%08llx:", offs);
		linebytes = (nbytes > DISP_LINE_LEN) ? DISP_LINE_LEN : nbytes;

		for (i = 0; i < linebytes; i += size) {
			if (size == 4) {
				u32 res;
				data_abort_mask();
				res = *((uint *)addr);
				if (swab)
					res = __swab32(res);
				if (data_abort_unmask()) {
					res = 0xffffffff;
					count -= printf(" xxxxxxxx");
				} else {
					count -= printf(" %08x", res);
				}
				*uip++ = res;
			} else if (size == 2) {
				u16 res;
				data_abort_mask();
				res = *((ushort *)addr);
				if (swab)
					res = __swab16(res);
				if (data_abort_unmask()) {
					res = 0xffff;
					count -= printf(" xxxx");
				} else {
					count -= printf(" %04x", res);
				}
				*usp++ = res;
			} else {
				u8 res;
				data_abort_mask();
				res = *((u_char *)addr);
				if (data_abort_unmask()) {
					res = 0xff;
					count -= printf(" xx");
				} else {
					count -= printf(" %02x", res);
				}
				*ucp++ = res;
			}
			addr += size;
			offs += size;
		}

		while (count--)
			putchar(' ');

		cp = (uint8_t *)linebuf;
		for (i = 0; i < linebytes; i++) {
			if ((*cp < 0x20) || (*cp > 0x7e))
				putchar('.');
			else
				printf("%c", *cp);
			cp++;
		}

		putchar('\n');
		nbytes -= linebytes;
		if (ctrlc())
			return -EINTR;
	} while (nbytes > 0);

	return 0;
}
