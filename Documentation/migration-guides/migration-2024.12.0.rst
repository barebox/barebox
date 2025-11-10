Release v2024.12.0
==================

spi-gpio DT bindings
--------------------

Support for the old deprecated device tree binding has been removed.
If spi-gpio binding is to be used in barebox, the new binding
(e.g. ``sck-gpios`` instead of ``gpio-sck``) must be used.

Configuration options
---------------------

* ``CONFIG_CRYPTO_RSA_KEY`` has been renamed to ``CONFIG_CRYPTO_PUBLIC_KEYS``
  to reflect that it now accepts multiple entries and is no longer limited
  to RSA with the addition of ECDSA support.

* ``CONFIG_CRYPTO_RSA_KEY_NAME_HINT`` has been removed. To specify a hint, add it
  in front of the key into ``CRYPTO_PUBLIC_KEYS``, e.g. ``hint1:file1 hint2:file2``.

LZ4 compression
---------------

barebox now expects ``lz4`` to be available instead of ``lz4c`` if
the barebox proper binary is configured to be LZ4-compressed.
Customizing the name is possible via the ``LZ4`` environment variable.
