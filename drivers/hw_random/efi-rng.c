// SPDX-License-Identifier: GPL-2.0
#include <common.h>
#include <driver.h>
#include <init.h>
#include <linux/hw_random.h>
#include <efi.h>
#include <efi/efi-device.h>

struct efi_rng_priv {
	struct efi_rng_protocol *protocol;
	struct hwrng hwrng;
};

static inline struct efi_rng_priv *to_efi_rng(struct hwrng *hwrng)
{
	return container_of(hwrng, struct efi_rng_priv, hwrng);
}

static int efi_rng_read(struct hwrng *hwrng, void *data, size_t len, bool wait)
{
	struct efi_rng_protocol *protocol = to_efi_rng(hwrng)->protocol;
	efi_status_t efiret;

	efiret = protocol->get_rng(protocol, NULL, len, data);

	return -efi_errno(efiret) ?: len;
}

static int efi_rng_probe(struct efi_device *efidev)
{
	struct efi_rng_priv *priv;

	priv = xzalloc(sizeof(*priv));

	BS->handle_protocol(efidev->handle, &efi_rng_protocol_guid,
			(void **)&priv->protocol);
	if (!priv->protocol)
		return -ENODEV;

	priv->hwrng.name = dev_name(&efidev->dev);
	priv->hwrng.read = efi_rng_read;

	return hwrng_register(&efidev->dev, &priv->hwrng);
}

static struct efi_driver efi_rng_driver = {
        .driver = {
		.name  = "efi-rng",
	},
        .probe = efi_rng_probe,
	.guid = EFI_RNG_PROTOCOL_GUID,
};
device_efi_driver(efi_rng_driver);
