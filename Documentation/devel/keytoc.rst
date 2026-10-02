.. _keytoc:

keytoc
======

Synopsis
--------

``keytoc [-o FILE] [-d] [-s] KEYSPEC...``

Description
-----------

keytoc converts public keys into C source to be compiled into barebox or into
a device tree fragment. Its main purpose is to process the
``CONFIG_CRYPTO_PUBLIC_KEYS`` option during the barebox build. Each KEYSPEC
is an entry as described in :ref:`public_keys`.

In addition, a KEYSPEC may contain ``symbol=<symbol>`` to name the C structure
holding the key. It must be a C identifier and defaults to ``__key_<n>``, where
``<n>`` is the key's position on the command line, starting at 1.

Options
-------

-o FILE  Write the output to FILE instead of standard output.
-d       Emit a device tree fragment instead of C source.
-s       Export the keys as global symbols instead of adding them to keyrings.

Output
------

For every key, keytoc emits a ``struct rsa_public_key`` or
``struct ecdsa_public_key`` named ``<symbol>``, a ``struct public_key`` with
the FIT key-name-hint and the SHA-256 hash of the DER-encoded public key, and
a ``struct public_key_record`` per keyring. The records are placed in the
linker list, from which barebox registers the keys on startup.
All identifiers have internal linkage by default.

With ``-s``, no ``struct public_key`` and no records are emitted, and
``<symbol>`` is exported globally, so that code can reference the key
directly, e.g. ``symbol=__key_mykey`` defines
``const struct rsa_public_key __key_mykey``. Keyrings and FIT key-name-hints
are ignored.

With ``-d``, RSA keys are emitted as ``key-key_<n>`` nodes below
``/signature``, or ``/signature-standalone`` with ``-s``, in the format barebox
reads keys from its device tree. ECDSA public keys can only be included via
``CONFIG_CRYPTO_PUBLIC_KEYS`` and not via the device tree.

Example
-------

::

  keytoc -o keys.h keyring=fit,fit-hint=rsa-devel:crypto/snakeoil-4096-development.pem
