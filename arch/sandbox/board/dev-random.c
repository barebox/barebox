#include <common.h>
#include <mach/linux.h>

devrandom_t *devrandom_init(void) {
	devrandom_t *fds = xzalloc(sizeof(*fds));

	fds->urandomfd = linux_open("/dev/urandom", false);
	if (fds->urandomfd < 0)
		return ERR_PTR(-EPERM);

	return fds;
}

int devrandom_read(devrandom_t *devrandom, void *buf, size_t len, int wait)
{
	(void)wait; /* /dev/urandom won't block */

	return linux_read(devrandom->urandomfd, buf, len);
}
