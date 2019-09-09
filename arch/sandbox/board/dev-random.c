#include <common.h>
#include <mach/linux.h>

devrandom_t *devrandom_init(void) {
	devrandom_t *fds = xzalloc(sizeof(*fds));

	fds->randomfd = linux_open("/dev/random", false);
	if (fds->randomfd < 0)
		return ERR_PTR(-EPERM);

	fds->urandomfd = linux_open("/dev/urandom", false);
	if (fds->urandomfd < 0)
		return ERR_PTR(-EPERM);

	return fds;
}

int devrandom_read(devrandom_t *devrandom, void *buf, size_t len, int wait)
{
	if (wait)
		return linux_read(devrandom->randomfd, buf, len);

	return linux_read(devrandom->urandomfd, buf, len);
}
