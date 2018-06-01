/*
 * Boot a Marvell SoC, with Xmodem over UART0.
 *  supports Kirkwood, Dove, Armada 370, Armada XP
 *
 * (c) 2012 Daniel Stodden <daniel.stodden@gmail.com>
 *
 * References: marvell.com, "88F6180, 88F6190, 88F6192, and 88F6281
 *   Integrated Controller: Functional Specifications" December 2,
 *   2008. Chapter 24.2 "BootROM Firmware".
 */

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <libgen.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <termios.h>
#include <sys/mman.h>
#include <sys/stat.h>

/*
 * Marvell BootROM UART Sensing
 */

static unsigned char kwboot_msg_boot[] = {
	0xBB, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77
};

static unsigned char kwboot_msg_debug[] = {
	0xDD, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77
};

#define KWBOOT_MSG_REQ_DELAY	1000 /* ms */
#define KWBOOT_MSG_RSP_TIMEO	1 /* ms */

/*
 * Xmodem Transfers
 */

#define SOH	1	/* sender start of block header */
#define EOT	4	/* sender end of block transfer */
#define ACK	6	/* target block ack */
#define NAK	21	/* target block negative ack */
#define CAN	24	/* target/sender transfer cancellation */

struct kwboot_block {
	uint8_t soh;
	uint8_t pnum;
	uint8_t _pnum;
	uint8_t data[128];
	uint8_t csum;
} __attribute((packed));

#define KWBOOT_BLK_RSP_TIMEO 1000 /* ms */

static int kwboot_verbose;

static void
kwboot_printv(const char *fmt, ...)
{
	va_list ap;

	if (kwboot_verbose) {
		va_start(ap, fmt);
		vprintf(fmt, ap);
		va_end(ap);
		fflush(stdout);
	}
}

static void
__spinner(void)
{
	const char seq[] = { '-', '\\', '|', '/' };
	const int div = 8;
	static int state, bs;

	if (state % div == 0) {
		fputc(bs, stdout);
		fputc(seq[state / div % sizeof(seq)], stdout);
		fflush(stdout);
	}

	bs = '\b';
	state++;
}

static void
kwboot_spinner(void)
{
	if (kwboot_verbose)
		__spinner();
}

static void
__progress(int pct, char c)
{
	const int width = 70;
	static const char *nl = "";
	static int pos;

	if (pos % width == 0)
		printf("%s%3d %% [", nl, pct);

	fputc(c, stdout);

	nl = "]\n";
	pos++;

	if (pct == 100) {
		while (pos++ < width)
			fputc(' ', stdout);
		fputs(nl, stdout);
	}

	fflush(stdout);

}

static void
kwboot_progress(int _pct, char c)
{
	static int pct;

	if (_pct != -1)
		pct = _pct;

	if (kwboot_verbose)
		__progress(pct, c);
}

static int
kwboot_tty_recv(int fd, void *buf, size_t len, int timeo)
{
	int rc, nfds;
	fd_set rfds;
	struct timeval tv;
	ssize_t n;

	rc = -1;

	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);

	tv.tv_sec = 0;
	tv.tv_usec = timeo * 1000;
	if (tv.tv_usec > 1000000) {
		tv.tv_sec += tv.tv_usec / 1000000;
		tv.tv_usec %= 1000000;
	}

	do {
		nfds = select(fd + 1, &rfds, NULL, NULL, &tv);
		if (nfds < 0)
			goto out;
		if (!nfds) {
			errno = ETIMEDOUT;
			goto out;
		}

		n = read(fd, buf, len);
		if (n < 0)
			goto out;

		buf = (char *)buf + n;
		len -= n;
	} while (len > 0);

	rc = 0;
out:
	return rc;
}

static int
kwboot_tty_send(int fd, const void *buf, size_t len)
{
	int rc;
	ssize_t n;

	if (!buf)
		return 0;

	rc = -1;

	do {
		n = write(fd, buf, len);
		if (n < 0)
			goto out;

		buf = (char *)buf + n;
		len -= n;
	} while (len > 0);

	rc = tcdrain(fd);
out:
	return rc;
}

static int
kwboot_tty_send_char(int fd, unsigned char c)
{
	return kwboot_tty_send(fd, &c, 1);
}

static speed_t
kwboot_tty_speed(int baudrate)
{
	switch (baudrate) {
	case 115200:
		return B115200;
	case 57600:
		return B57600;
	case 38400:
		return B38400;
	case 19200:
		return B19200;
	case 9600:
		return B9600;
	}

	return -1;
}

static int
kwboot_open_tty(const char *path, speed_t speed)
{
	int rc, fd;
	struct termios tio;

	rc = -1;

	fd = open(path, O_RDWR|O_NOCTTY|O_NDELAY);
	if (fd < 0)
		goto out;

	memset(&tio, 0, sizeof(tio));

	tio.c_iflag = 0;
	tio.c_cflag = CREAD|CLOCAL|CS8;

	tio.c_cc[VMIN] = 1;
	tio.c_cc[VTIME] = 10;

	cfsetospeed(&tio, speed);
	cfsetispeed(&tio, speed);

	rc = tcsetattr(fd, TCSANOW, &tio);
	if (rc)
		goto out;

	rc = fd;
out:
	if (rc < 0) {
		if (fd >= 0)
			close(fd);
	}

	return rc;
}

static int
kwboot_bootmsg(int tty, void *msg, unsigned num_nacks)
{
	int rc;
	char c;
	unsigned saw_nacks = 0;

	if (msg == NULL)
		kwboot_printv("Please reboot the target into UART boot mode...");
	else
		kwboot_printv("Sending boot message. Please reboot the target...");

	rc = tcflush(tty, TCIOFLUSH);
	if (rc)
		return rc;

	do {
		rc = kwboot_tty_send(tty, msg, 8);
		if (rc) {
			usleep(KWBOOT_MSG_REQ_DELAY * 1000);
			continue;
		}

		rc = kwboot_tty_recv(tty, &c, 1, KWBOOT_MSG_RSP_TIMEO);
		while (!rc && c != NAK) {
			if (c == '\\')
				kwboot_printv("\\\\", c);
			else if (isprint(c) || c == '\r' || c == '\n')
				kwboot_printv("%c", c);
			else
				kwboot_printv("\\x%02hhx", c);

			rc = kwboot_tty_recv(tty, &c, 1, KWBOOT_MSG_RSP_TIMEO);

			saw_nacks = 0;
		}

	} while (rc || c != NAK || (++saw_nacks < num_nacks));

	kwboot_printv("\nGot expected NAKs\n");

	return rc;
}

static int
kwboot_debugmsg(int tty, void *msg)
{
	int rc;

	kwboot_printv("Sending debug message. Please reboot the target...");

	rc = tcflush(tty, TCIOFLUSH);
	if (rc)
		return rc;

	do {
		char buf[16];

		rc = kwboot_tty_send(tty, msg, 8);
		if (rc) {
			usleep(KWBOOT_MSG_REQ_DELAY * 1000);
			continue;
		}

		rc = kwboot_tty_recv(tty, buf, 16, KWBOOT_MSG_RSP_TIMEO);

		kwboot_spinner();

	} while (rc);

	kwboot_printv("\n");

	return rc;
}

static int
kwboot_xm_makeblock(struct kwboot_block *block, const void *data,
		    size_t size, int pnum)
{
	const size_t blksz = sizeof(block->data);
	size_t n;
	int i;

	block->soh = SOH;
	block->pnum = pnum;
	block->_pnum = ~block->pnum;

	n = size < blksz ? size : blksz;
	memcpy(&block->data[0], data, n);
	memset(&block->data[n], 0, blksz - n);

	block->csum = 0;
	for (i = 0; i < n; i++)
		block->csum += block->data[i];

	return n;
}

#define min(a, b) ((a) < (b) ? (a) : (b))

static int
kwboot_xm_resync(int fd)
{
	/*
	 * When the SoC has a different perception of where the package boundary
	 * is, just resending the packet doesn't help. To resync send 0xff until
	 * we get another NAK.
	 * The BootROM code (of the Armada XP at least) doesn't interpret 0xff
	 * as a start of a package and sends a NAK for each 0xff when waiting
	 * for SOH, so it's possible to send >1 byte without the SoC starting a
	 * new frame.
	 * When there is no response after sizeof(struct kwboot_block) bytes,
	 * there is another problem.
	 */
	int rc;
	char buf[sizeof(struct kwboot_block)];
	unsigned interval = 1;
	unsigned len;
	char *p = buf;

	memset(buf, 0xff, sizeof(buf));

	while (interval <= sizeof(buf)) {
		len = min(interval, buf + sizeof(buf) - p);
		rc = kwboot_tty_send(fd, p, len);
		if (rc)
			return rc;

		kwboot_tty_recv(fd, p, len, KWBOOT_BLK_RSP_TIMEO);
		if (*p != 0xff)
			/* got at least one char, if it's a NAK we're synced. */
			return (*p == NAK);

		p += len;
		interval *= 2;
	}

	return 0;
}

static int
kwboot_xm_sendblock(int fd, struct kwboot_block *block)
{
	int rc, retries;
	char c;

	retries = 16;
	do {
		rc = kwboot_tty_send(fd, block, sizeof(*block));
		if (rc)
			break;

		do {
			rc = kwboot_tty_recv(fd, &c, 1, KWBOOT_BLK_RSP_TIMEO);
			if (rc)
				break;

			if (c != ACK && c!= NAK && c != CAN)
				printf("%c", c);

		} while (c != ACK && c != NAK && c != CAN);

		if (c == NAK && kwboot_xm_resync(fd))
			kwboot_progress(-1, 'S');
		else if (c != ACK)
			kwboot_progress(-1, '+');

	} while (c == NAK && retries-- > 0);

	if (!rc) {
		rc = -1;

		switch (c) {
		case ACK:
			rc = 0;
			break;
		case NAK:
			errno = EBADMSG;
			break;
		case CAN:
			errno = ECANCELED;
			break;
		default:
			errno = EPROTO;
			break;
		}
	}

	return rc;
}

static int
kwboot_xmodem(int tty, const void *_data, size_t size)
{
	const uint8_t *data = _data;
	int rc, pnum, N, err;

	pnum = 1;
	N = 0;

	kwboot_printv("Sending boot image...\n");

	do {
		struct kwboot_block block;
		int n;

		n = kwboot_xm_makeblock(&block,
					data + N, size - N,
					pnum++);
		if (n < 0)
			goto can;

		if (!n)
			break;

		rc = kwboot_xm_sendblock(tty, &block);
		if (rc)
			goto out;

		N += n;
		kwboot_progress(N * 100 / size, '.');
	} while (1);

	rc = kwboot_tty_send_char(tty, EOT);

out:
	return rc;

can:
	err = errno;
	kwboot_tty_send_char(tty, CAN);
	errno = err;
	goto out;
}

static int
kwboot_term_pipe(int in, int out, char *quit, int *s)
{
	ssize_t nin, nout;
	char _buf[128], *buf = _buf;

	nin = read(in, buf, sizeof(buf));
	if (nin < 0)
		return -1;

	if (quit) {
		int i;

		for (i = 0; i < nin; i++) {
			if (*buf == quit[*s]) {
				(*s)++;
				if (!quit[*s])
					return 0;
				buf++;
				nin--;
			} else
				while (*s > 0) {
					nout = write(out, quit, *s);
					if (nout <= 0)
						return -1;
					(*s) -= nout;
				}
		}
	}

	while (nin > 0) {
		nout = write(out, buf, nin);
		if (nout <= 0)
			return -1;
		nin -= nout;
	}

	return 0;
}

static int
kwboot_terminal(int tty)
{
	int rc, in, s;
	char *quit = "\34c";
	struct termios otio, tio;

	rc = -1;

	in = STDIN_FILENO;
	if (isatty(in)) {
		rc = tcgetattr(in, &otio);
		if (!rc) {
			tio = otio;
			cfmakeraw(&tio);
			rc = tcsetattr(in, TCSANOW, &tio);
		}
		if (rc) {
			perror("tcsetattr");
			goto out;
		}

		kwboot_printv("[Type Ctrl-%c + %c to quit]\r\n",
			      quit[0]|0100, quit[1]);
	} else
		in = -1;

	rc = 0;
	s = 0;

	do {
		fd_set rfds;
		int nfds = 0;

		FD_SET(tty, &rfds);
		nfds = nfds < tty ? tty : nfds;

		if (in >= 0) {
			FD_SET(in, &rfds);
			nfds = nfds < in ? in : nfds;
		}

		nfds = select(nfds + 1, &rfds, NULL, NULL, NULL);
		if (nfds < 0)
			break;

		if (FD_ISSET(tty, &rfds)) {
			rc = kwboot_term_pipe(tty, STDOUT_FILENO, NULL, NULL);
			if (rc)
				break;
		}

		if (FD_ISSET(in, &rfds)) {
			rc = kwboot_term_pipe(in, tty, quit, &s);
			if (rc)
				break;
		}
	} while (quit[s] != 0);

	tcsetattr(in, TCSANOW, &otio);
out:
	return rc;
}

static unsigned char crc(const unsigned char *img, size_t len)
{
	unsigned char ret = 0;
	size_t i;

	for (i = 0; i < len; ++i)
		ret += img[i];

	return ret;
}

static int
kwboot_check_image(unsigned char *img, size_t size)
{
	size_t i;
	size_t header_size, image_size, image_offset;
	unsigned char csum = 0;
	unsigned char imgversion;

	if (size < 0x20) {
		fprintf(stderr,
			"Image too small to even contain a Main Header\n");
		return 1;
	}

	switch (img[0x0]) {
		case 0x69: /* UART0 */
			break;

		case 0x5a: /* SPI/NOR */
		case 0x78: /* SATA */
		case 0x8b: /* NAND */
		case 0x9c: /* PCIe */
			/* change boot source to UART and fix checksum */
			img[0x1f] -= img[0x0];
			img[0x1f] += 0x69;
			img[0x0] = 0x69;

			break;

		default:
			fprintf(stderr,
				"Unknown boot source: 0x%hhx\n", img[0x0]);
			return 1;
	}

	imgversion = img[0x8];
	if (imgversion > 1) {
		fprintf(stderr, "Unknown version: 0x%hhx\n", img[0x8]);
		return 1;
	}

	image_size = img[0x4] | (img[0x5] << 8) |
		(img[0x6] << 16) | (img[0x7] << 24);
	image_offset = img[0xc] | (img[0xd] << 8) |
		(img[0xe] << 16) | (img[0xf] << 24);

	if (imgversion == 0) {
		/* Image format 0 */
		header_size =
			img[0x1e] * 0x200 /* header extensions */ +
			img[0x1d] * 0x800 /* binary header extensions */;
	} else {
		/* Image format 1 */
		header_size = (img[0x9] << 16) | img[0xa] | (img[0xb] << 8);
	}

	if (header_size > image_offset) {
		fprintf(stderr, "Header (%zu) expands over image start (%zu)\n",
			header_size, image_offset);
		return 1;
	}

	if (image_offset + image_size != size) {
		fprintf(stderr, "Image doesn't end at file end (%zu + %zu != %zu)\n",
			image_offset, image_size, size);
		return 1;
	}


	if (imgversion == 0) {
		/* check Main Header */
		csum = crc(img, 0x1f);
		if (csum != img[0x1f]) {
			fprintf(stderr,
				"Main Header checksum mismatch: specified: 0x%02hhx, calculated: 0x%02hhx\n",
				img[0x1f], csum);
			return 1;
		}

		/* check Header Extensions */
		for (i = 0; i < img[0x1e]; ++i) {
			csum = crc(img + 0x20 + i * 0x200, 0x1df);
			if (csum != img[i * 0x200 + 0x1ff]) {
				fprintf(stderr,
					"Extension Header #%zu checksum mismatch: specified: 0x%02hhx, calculated: 0x%02hhx\n",
					i, img[i * 0x200 + 0x1ff], csum);
				return 1;
			}
		}
	} else {
		csum = crc(img, header_size);
		csum -= img[0x1f];

		if (csum != img[0x1f]) {
			fprintf(stderr,
				"Checksum mismatch: specified: 0x%02hhx, calculated: 0x%02hhx\n",
				img[0x1f], csum);
			return 1;
		}

	}

	return 0;
}

static void *
kwboot_mmap_image(const char *path, size_t *size)
{
	int rc, fd;
	struct stat st;
	void *img;

	rc = -1;
	img = NULL;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		goto out;

	rc = fstat(fd, &st);
	if (rc)
		goto out;

	img = mmap(NULL, st.st_size, PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (img == MAP_FAILED) {
		img = NULL;
		goto out;
	}

	rc = 0;
	*size = st.st_size;
out:
	if (rc && img) {
		munmap(img, st.st_size);
		img = NULL;
	}
	if (fd >= 0)
		close(fd);

	return img;
}

static void
kwboot_usage(FILE *stream, char *progname)
{
	fprintf(stream,
		"Usage: %s [-d | -b <image> [ -n <naks> ] | -D <image> ] [ -t ] [-B <baud> ] <TTY>\n",
		progname);
	fprintf(stream, "\n");
	fprintf(stream,
		"  -b <image>: boot <image> with preamble (Kirkwood, Armada 370/XP)\n");
	fprintf(stream,
		"  -D <image>: boot <image> without preamble (Dove)\n");
	fprintf(stream, "  -d: enter debug mode\n");
	fprintf(stream,
		"  -n <naks>: wait for <naks> NAK before uploading image (default: 1)\n");
	fprintf(stream, "\n");
	fprintf(stream, "  -t: mini terminal\n");
	fprintf(stream, "\n");
	fprintf(stream, "  -B <baud>: set baud rate\n");
	fprintf(stream, "\n");
}

int
main(int argc, char **argv)
{
	const char *ttypath, *imgpath;
	int rv, rc, tty, term, force = 0;
	void *bootmsg;
	void *debugmsg;
	void *img;
	unsigned num_nacks = 1;
	size_t size;
	speed_t speed;

	rv = 1;
	tty = -1;
	bootmsg = NULL;
	debugmsg = NULL;
	imgpath = NULL;
	img = NULL;
	term = 0;
	size = 0;
	speed = B115200;

	kwboot_verbose = isatty(STDOUT_FILENO);

	do {
		int c = getopt(argc, argv, "b:dfhtn:B:D:");
		if (c < 0)
			break;

		switch (c) {
		case 'b':
			bootmsg = kwboot_msg_boot;
			imgpath = optarg;
			break;

		case 'D':
			bootmsg = NULL;
			imgpath = optarg;
			break;

		case 'd':
			debugmsg = kwboot_msg_debug;
			break;

		case 't':
			term = 1;
			break;

		case 'f':
			force = 1;
			break;

		case 'n':
			num_nacks = atoi(optarg);
			break;

		case 'B':
			speed = kwboot_tty_speed(atoi(optarg));
			if (speed == -1)
				goto usage;
			break;

		case 'h':
			rv = 0;
		default:
			goto usage;
		}
	} while (1);

	if (!bootmsg && !term && !debugmsg)
		goto usage;

	if (argc - optind < 1)
		goto usage;

	ttypath = argv[optind++];

	tty = kwboot_open_tty(ttypath, speed);
	if (tty < 0) {
		perror(ttypath);
		goto out;
	}

	if (imgpath) {
		img = kwboot_mmap_image(imgpath, &size);
		if (!img) {
			perror(imgpath);
			goto out;
		}

		rc = kwboot_check_image(img, size);
		if (rc && !force) {
			fprintf(stderr,
				"Image check failed, restart with -f to ignore\n");
			goto out;
		}
	}

	if (debugmsg) {
		rc = kwboot_debugmsg(tty, debugmsg);
		if (rc) {
			perror("debugmsg");
			goto out;
		}
	} else {
		rc = kwboot_bootmsg(tty, bootmsg, num_nacks);
		if (rc) {
			perror("bootmsg");
			goto out;
		}
	}

	if (img) {
		rc = kwboot_xmodem(tty, img, size);
		if (rc) {
			perror("xmodem");
			goto out;
		}
	}

	if (term) {
		rc = kwboot_terminal(tty);
		if (rc && !(errno == EINTR)) {
			perror("terminal");
			goto out;
		}
	}

	rv = 0;
out:
	if (tty >= 0)
		close(tty);

	if (img)
		munmap(img, size);

	return rv;

usage:
	kwboot_usage(rv ? stderr : stdout, basename(argv[0]));
	goto out;
}
