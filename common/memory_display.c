#include <common.h>
#include <errno.h>
#include <abort.h>

#define DISP_LINE_LEN	16


int __pr_memory_display(int level, const void *addr, loff_t offs, unsigned nbytes,
			int size, int swab, const char *fmt, ...)
{
	unsigned long linebytes, i;
	unsigned char *cp;
	unsigned char line[sizeof("00000000: 0000 0000 0000 0000  0000 0000 0000 0000            ................")];
	struct va_format vaf;
	int ret;
	va_list args;

	va_start(args, fmt);

	vaf.fmt = fmt;
	vaf.va = &args;

	/* Print the lines.
	 *
	 * We buffer all read data, so we can make sure data is read only
	 * once, and all accesses are with the specified bus width.
	 */
	do {
		unsigned char linebuf[DISP_LINE_LEN];
		uint64_t *ullp = (uint64_t *)linebuf;
		uint32_t *uip = (uint32_t *)linebuf;
		uint16_t *usp = (uint16_t *)linebuf;
		uint8_t *ucp = (uint8_t *)linebuf;
		unsigned char *pos = line;

		pos += sprintf(pos, "%08llx:", offs);
		linebytes = (nbytes > DISP_LINE_LEN) ? DISP_LINE_LEN : nbytes;

		for (i = 0; i < linebytes; i += size) {
			if (size == 8) {
				uint64_t res;
				data_abort_mask();
				res = *((uint64_t *)addr);
				if (swab)
					res = __swab64(res);
				if (data_abort_unmask()) {
					res = 0xffffffffffffffffULL;
					pos += sprintf(pos, " xxxxxxxxxxxxxxxx");
				} else {
					pos += sprintf(pos, " %016llx", res);
				}
				*ullp++ = res;
			} else if (size == 4) {
				uint32_t res;
				data_abort_mask();
				res = *((uint32_t *)addr);
				if (swab)
					res = __swab32(res);
				if (data_abort_unmask()) {
					res = 0xffffffff;
					pos += sprintf(pos, " xxxxxxxx");
				} else {
					pos += sprintf(pos, " %08x", res);
				}
				*uip++ = res;
			} else if (size == 2) {
				uint16_t res;
				data_abort_mask();
				res = *((uint16_t *)addr);
				if (swab)
					res = __swab16(res);
				if (i > 1 && i % 8 == 0)
					pos += sprintf(pos, " ");
				if (data_abort_unmask()) {
					res = 0xffff;
					pos += sprintf(pos, " xxxx");
				} else {
					pos += sprintf(pos, " %04x", res);
				}
				*usp++ = res;
			} else {
				uint8_t res;
				data_abort_mask();
				res = *((uint8_t *)addr);
				if (i > 1 && i % 8 == 0)
					pos += sprintf(pos, " ");
				if (data_abort_unmask()) {
					res = 0xff;
					pos += sprintf(pos, " xx");
				} else {
					pos += sprintf(pos, " %02x", res);
				}
				*ucp++ = res;
			}
			addr += size;
			offs += size;
		}

		pos += sprintf(pos, "%*s", 61 - (pos - line), "");

		cp = linebuf;
		for (i = 0; i < linebytes; i++) {
			if ((*cp < 0x20) || (*cp > 0x7e))
				sprintf(pos, ".");
			else
				sprintf(pos, "%c", *cp);
			pos++;
			cp++;
		}

		if (level >= MSG_EMERG)
			pr_print(level, "%pV%s\n", &vaf, line);
		else
			printf("%s\n", line);

		nbytes -= linebytes;
		if (ctrlc()) {
			ret = -EINTR;
			goto out;
		}

	} while (nbytes > 0);

	va_end(args);
	ret = 0;
out:

	return ret;
}

int memory_display(const void *addr, loff_t offs, unsigned nbytes,
		   int size, int swab)
{
	return pr_memory_display(-1, addr, offs, nbytes, size, swab);
}