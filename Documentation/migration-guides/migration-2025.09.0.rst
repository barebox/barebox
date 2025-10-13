Release v2025.09.0
==================

Removal of CONFIG_BOOTM_FITIMAGE_PUBKEY
---------------------------------------

The legacy way of adding FIT keys to the barebox build was having mkimage
generate a device tree snippet and including it in the barebox board device
tree::

#ifdef CONFIG_BOOTM_FITIMAGE_PUBKEY
#include CONFIG_BOOTM_FITIMAGE_PUBKEY
#endif

This has now been removed in favor of ``CONFIG_CRYPTO_PUBLIC_KEYS``.
This option doesn't consume device tree snippets, but file paths to
the PEM or PKCS#11 URIs and thus will require a build-time OpenSSL dependency.

Build dependencies
------------------

* Building barebox for 64-bit Rockchip platforms required OpenSSL v3.0 or later.
  This was unintended and the fix is queued for v2025.10.0.

* ti-linux-firmware 11.00.14 renames ``ti-fs-firmware-am62lx-hs-fs-enc.bin`` to
  to ``ti-fs-firmware-am62lx-hs-enc.bin`` and ``ti-fs-firmware-am62lx-hs-fs-cert.bin``
  is renamed to ``ti-fs-firmware-am62lx-hs-cert.bin`` and barebox has followed suit.

Rename in /dev
--------------

EEPROMs that are pointed at by a device tree alias do no longer have
an extra 0 at the end, e.g., ``/dev/eeprom00`` has become ``/dev/eeprom0``.
