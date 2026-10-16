.. _public_keys:

Built-in public keys
====================

barebox verifies signed images like FIT images against public keys compiled
into it. With ``CONFIG_CRYPTO_BUILTIN_KEYS`` enabled, these keys are listed in
``CONFIG_CRYPTO_PUBLIC_KEYS``.

Key specification
-----------------

``CONFIG_CRYPTO_PUBLIC_KEYS`` is a space-separated list of entries of the
form::

  keyring=<keyring>[,keyring=<keyring>...][,fit-hint=<key-name-hint>]:<key>

``<key>`` is a PEM file containing an X.509 certificate or a public key, or a
PKCS#11 URI starting with ``pkcs11:``. Relative paths are resolved from the
build output directory. RSA and ECDSA keys are supported.

``<keyring>`` selects what the key is trusted for. Repeat ``keyring=`` to add
the key to several keyrings. barebox itself uses ``fit`` for FIT images and
``tlv-generic`` for barebox TLV blobs. Board code may define more, e.g. for
:doc:`custom TLV formats <barebox-tlv>`.

``fit-hint`` optionally sets the FIT key-name-hint of the key, which is only
used for FIT image verification: barebox first tries the key whose FIT
key-name-hint matches the ``key-name-hint`` property of the signature and only
then all other keys of the ``fit`` keyring. FIT key-name-hints must be unique
within a keyring.

Keyring names and FIT key-name-hints may contain letters, digits, ``_`` and
``-``, but must start with a letter or ``_``.

The older forms ``<key-name-hint>:<key>`` and ``<key>`` are deprecated. They
add the key to the ``fit`` keyring and print a warning during the build.

With ``CONFIG_CRYPTO_BUILTIN_DEVELOPMENT_KEYS``, the publicly known
development keys are added to the ``fit`` and ``tlv-generic`` keyrings with
the FIT key-name-hints ``rsa-devel`` and ``ecdsa-devel``. Never use them in
production.

Examples::

  CONFIG_CRYPTO_PUBLIC_KEYS="keyring=fit:/path/to/fit.crt"
  CONFIG_CRYPTO_PUBLIC_KEYS="keyring=fit,keyring=tlv-generic:/path/to/a.pem keyring=fit:/path/to/b.pem"
  CONFIG_CRYPTO_PUBLIC_KEYS="keyring=fit,fit-hint=prod:pkcs11:object=fit-key"

Passing keys via the environment
--------------------------------

To keep key locations out of the configuration, a ``<key>`` of the form
``__ENV__<VAR>`` is replaced with the value of the environment variable
``<VAR>`` at build time::

  CONFIG_CRYPTO_PUBLIC_KEYS="keyring=fit:__ENV__FIT_KEY"

An entry of the form ``__ENV__<VAR>`` is replaced with all entries in
``<VAR>``. These are separated by spaces, so literal spaces and backslashes
within an entry must be escaped with a backslash::

  export FIT_KEYS="keyring=fit:/path/to/a.pem keyring=fit:/path/to/my\ key.pem"
  CONFIG_CRYPTO_PUBLIC_KEYS="__ENV__FIT_KEYS"

The PIN for accessing a PKCS#11 token is taken from ``KBUILD_SIGN_PIN``.

.. note::

   See :ref:`keytoc` for how the keys are converted for inclusion into
   barebox.
