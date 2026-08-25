// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <driver.h>
#include <fuzz.h>
#include <malloc.h>
#include <of.h>
#include <crypto/keystore.h>
#include <linux/sizes.h>

#include "state.h"

/*
 * The device and the format outlive a single fuzzing iteration: creating
 * them per input would dominate the runtime, would credit the first input
 * with the registration and registering the same device name twice fails
 * anyway. Setup therefore happens in the init callback the fuzz drivers
 * call before the first input.
 */
#define FUZZ_STATE_SECRET	"fuzz-state"

/*
 * A raw format with an algo authenticates the data with a keyed digest,
 * which needs a secret in the keystore. Build such a format alongside the
 * plain one so the authenticated paths are reachable as well.
 */
static struct state_backend_format *fuzz_state_hmac_format(struct device *dev)
{
	static const u8 secret[16] = { 0x0f, 0x1e, 0x2d, 0x3c };
	struct state_backend_format *format = NULL;
	struct device_node *node;
	int ret;

	ret = keystore_set_secret(FUZZ_STATE_SECRET, secret, sizeof(secret));
	if (ret)
		return NULL;

	node = of_new_node(NULL, NULL);
	if (!node)
		return NULL;

	if (!of_new_property(node, "algo", "hmac(sha256)",
			     sizeof("hmac(sha256)")))
		goto out;

	if (backend_format_raw_create(&format, node, FUZZ_STATE_SECRET, dev))
		format = NULL;
out:
	of_delete_node(node);

	return format;
}

static int fuzz_state_setup(const char *name,
			    struct state_backend_format **format,
			    struct device **devp)
{
	struct device *dev;
	int ret;

	dev = xzalloc(sizeof(*dev));
	dev_set_name(dev, "%s", name);
	dev->id = DEVICE_ID_SINGLE;

	ret = register_device(dev);
	if (ret) {
		free(dev);
		return ret;
	}

	ret = backend_format_raw_create(format, NULL, "", dev);
	if (ret) {
		unregister_device(dev);
		free(dev);
		return ret;
	}

	*devp = dev;

	return 0;
}

static struct state_backend_format *direct_format, *direct_hmac_format;
static struct device *direct_dev;

static void fuzz_state_direct_init(void)
{
	if (direct_dev)
		return;

	if (fuzz_state_setup("fuzz-state-direct", &direct_format, &direct_dev))
		return;

	direct_hmac_format = fuzz_state_hmac_format(direct_dev);
}

static int fuzz_state_direct(const u8 *data, size_t size)
{
	struct state_backend_format *use;
	enum state_flags flags;
	void *buf;
	ssize_t len;

	if (!size || !direct_format)
		return 0;

	/*
	 * The lowest bit of the input selects between the plain format and
	 * the authenticated one. It is part of the magic, which the header
	 * carries anyway, so no input is consumed for it.
	 */
	if ((data[0] & 1) && direct_hmac_format) {
		use = direct_hmac_format;
		flags = 0;
	} else {
		use = direct_format;
		flags = STATE_FLAG_NO_AUTHENTICATION;
	}

	buf = xmemdup(data, size);
	len = size;

	use->verify(use, 0, buf, &len, flags);

	free(buf);

	return 0;
}
fuzz_test_init("state-direct", fuzz_state_direct, fuzz_state_direct_init);
