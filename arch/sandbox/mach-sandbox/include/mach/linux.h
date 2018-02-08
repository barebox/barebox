#ifndef __ASM_ARCH_LINUX_H
#define __ASM_ARCH_LINUX_H

struct device_d;

int sandbox_add_device(struct device_d *dev);

struct fb_bitfield;

int linux_register_device(const char *name, void *start, void *end);
int tap_alloc(char *dev);
uint64_t linux_get_time(void);
int linux_read(int fd, void *buf, size_t count);
int linux_read_nonblock(int fd, void *buf, size_t count);
ssize_t linux_write(int fd, const void *buf, size_t count);
off_t linux_lseek(int fildes, off_t offset);
int linux_tstc(int fd);
void __attribute__((noreturn)) linux_exit(void);

int linux_execve(const char * filename, char *const argv[], char *const envp[]);

int barebox_register_console(int stdinfd, int stdoutfd);

int barebox_register_dtb(const void *dtb);

struct linux_console_data {
	int stdinfd;
	int stdoutfd;
};

extern int sdl_xres;
extern int sdl_yres;
void sdl_close(void);
int sdl_open(int xres, int yres, int bpp, void* buf);
void sdl_stop_timer(void);
void sdl_start_timer(void);
void sdl_get_bitfield_rgba(struct fb_bitfield *r, struct fb_bitfield *g,
			    struct fb_bitfield *b, struct fb_bitfield *a);
void sdl_setpixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

struct ft2232_bitbang;
struct ft2232_bitbang *barebox_libftdi1_open(int vendor_id, int device_id,
						const char *serial);
void barebox_libftdi1_gpio_direction(struct ft2232_bitbang *ftbb,
						unsigned off, unsigned dir);
int barebox_libftdi1_gpio_get_value(struct ft2232_bitbang *ftbb,
						unsigned off);
void barebox_libftdi1_gpio_set_value(struct ft2232_bitbang *ftbb,
						unsigned off, unsigned val);
int barebox_libftdi1_update(struct ft2232_bitbang *ftbb);
void barebox_libftdi1_close(void);

#endif /* __ASM_ARCH_LINUX_H */
