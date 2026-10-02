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

Options
-------

-o FILE  Write the output to FILE instead of standard output.
-d       Emit a device tree fragment instead of C source.
-s       Emit stand-alone keys, which are not added to any keyring.

Output
------

For every key, keytoc emits a ``struct rsa_public_key`` or
``struct ecdsa_public_key``, a ``struct public_key`` with the FIT
key-name-hint and the SHA-256 hash of the DER-encoded public key, and a
``struct public_key_record`` per keyring. The records are placed in the
linker list, from which barebox registers the keys on startup.
All identifiers have internal linkage by default.

With ``-s``, no ``struct public_key`` and no records are emitted, and the
key is exported globally as ``struct rsa_public_key __key_key_<n>`` or
``struct ecdsa_public_key __key_key_<n>``, so that code can reference it
directly.

With ``-d``, RSA keys are emitted as ``key-key_<n>`` nodes below
``/signature``, or ``/signature-standalone`` with ``-s``, in the format barebox
reads keys from its device tree. ECDSA public keys can only be included via
``CONFIG_CRYPTO_PUBLIC_KEYS`` and not via the device tree.

Example
-------

::

  keytoc -o keys.h keyring=fit,fit-hint=rsa-devel:crypto/snakeoil-4096-development.pem
