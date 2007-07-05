
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <libgen.h>
#include <sys/mman.h>
#include <asm/arch/linux.h>
#include <asm/arch/hostfile.h>
#include <errno.h>

static struct termios term_orig, term_vi;
static char erase_char;         // the users erase character

static void rawmode(void)
{
	tcgetattr(0, &term_orig);
	term_vi = term_orig;
	term_vi.c_lflag &= (~ICANON & ~ECHO);	// leave ISIG ON- allow intr's
	term_vi.c_iflag &= (~IXON & ~ICRNL);
	term_vi.c_oflag &= (~ONLCR);
	term_vi.c_cc[VMIN] = 1;
	term_vi.c_cc[VTIME] = 0;
	erase_char = term_vi.c_cc[VERASE];
	tcsetattr(0, TCSANOW, &term_vi);
}

static void cookmode(void)
{
	fflush(stdout);
	tcsetattr(0, TCSANOW, &term_orig);
}

void serial_putc (const char c)
{
	fputc(c, stdout);

	/* If \n, also do \r */
	if (c == '\n')
		serial_putc ('\r');
}

int serial_tstc (void)
{
	return 0;
}

void serial_puts (const char *s)
{
	while (*s) {
		serial_putc (*s++);
	}
}

uint64_t linux_get_time(void)
{
	struct timespec ts;
	uint64_t now;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	now = ts.tv_sec * 1000 * 1000 * 1000 + ts.tv_nsec;

	return now;
}

int do_reset (void *unused, int flag, int argc, char *argv[])
{
	cookmode();
	exit(0);
}

void enable_interrupts (void)
{
}

int serial_getc (void)
{
	return fgetc(stdin);
}

int linux_read(int fd, void *buf, size_t count)
{
	return read(fd, buf, count);
}

int linux_read_nonblock(int fd, void *buf, size_t count)
{
	int oldflags, ret;

	oldflags = fcntl(fd, F_GETFL);
	if (oldflags == -1)
		goto err_out;

	if (fcntl(fd, F_SETFL, oldflags | O_NONBLOCK) == -1)
		goto err_out;

	ret = read(fd, buf, count);

	if (fcntl(fd, F_SETFL, oldflags) == -1)
		goto err_out;

	if (ret == -1) {
//		printf("errno\n");
		usleep(1000);
	}

//	if (ret == -1 && errno == EAGAIN) {
//		printf("delay\n");
//	}

	return ret;

err_out:
	perror("fcntl");
	return -1;
}

ssize_t linux_write(int fd, const void *buf, size_t count)
{
	return write(fd, buf, count);
}

off_t linux_lseek(int fildes, off_t offset)
{
	return lseek(fildes, offset, SEEK_SET);
}

void  flush_cache (unsigned long dummy1, unsigned long dummy2)
{
	/* why should we? */
}

extern void start_uboot(void);
extern void mem_malloc_init (void *start, void *end);

int add_image(char *str, char *name_template)
{
	char *file;
	int readonly = 0, map = 1;
	struct stat s;
	char *opt;
	int fd, ret;
	struct hf_platform_data *hf = malloc(sizeof(struct hf_platform_data));

	if (!hf)
		return -1;

	file = strtok(str, ",");
	while ((opt = strtok(NULL, ","))) {
		if (!strcmp(opt, "ro"))
			readonly = 1;
		if (!strcmp(opt, "map"))
			map = 1;
	}

	printf("add file %s(%s)\n", file, readonly ? "ro" : "");

	fd = open(file, readonly ? O_RDONLY : O_RDWR);
	hf->fd = fd;
	hf->filename = file;

	if (fd < 0) {
		perror("open");
		goto err_out;
	}

	if (fstat(fd, &s)) {
		perror("fstat");
		goto err_out;
	}

	hf->size = s.st_size;

	if (map) {
		hf->map_base = (unsigned long)mmap(0, hf->size,
				PROT_READ | (readonly ? 0 : PROT_WRITE),
				MAP_SHARED, fd, 0);
		if ((void *)hf->map_base == MAP_FAILED)
			printf("warning: mmapping %s failed\n", file);
	}


	ret = u_boot_register_filedev(hf, name_template);
	if (ret)
		goto err_out;
	return 0;

err_out:
	if (fd > 0)
		close(fd);
	free(hf);
	return -1;
}

void print_usage(const char *prgname)
{
	printf("usage\n");
}

int main(int argc, char *argv[])
{
	void *ram;
	int opt;
	int malloc_size = 1024 * 1024;
	int ret;

	ram = malloc(malloc_size);
	if (!ram) {
		printf("unable to get malloc space\n");
		exit(1);
	}
	mem_malloc_init(ram, ram + malloc_size);

	while ((opt = getopt(argc, argv, "hi:m:e:")) != -1) {
		switch (opt) {
		case 'h':
			print_usage(basename(argv[0]));
			exit(0);
		case 'i':
			ret = add_image(optarg, "fd");
			if (ret)
				exit(1);
			break;
		case 'm':
			malloc_size = strtoul(optarg, NULL, 0);
			break;
		case 'e':
			ret = add_image(optarg, "env");
			if (ret)
				exit(1);
			break;
		}
	}

	rawmode();
	start_uboot();

	/* never reached */
	return 0;
}

