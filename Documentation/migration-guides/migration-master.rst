:orphan:

CONFIG_CRYPTO_PUBLIC_KEYS
-------------------------

The syntax of keytoc keyspecs when fully provided via an environment variable
was slightly changed to allow any number of keyspecs to be provided via an
environment variable. Such environment variables are now split at spaces to be
interpreted as multiple keyspecs. Any literal spaces and backslashes contained
in such keyspecs need to be escaped with a backslash.

This only applies to the form:

  CONFIG_CRYPTO_PUBLIC_KEYS="__ENV__A"

While the interpretation of environment variables specifying hint or URI remains unchanged:

  CONFIG_CRYPTO_PUBLIC_KEYS="keyring=kr:__ENV__B"

Fit hints can no longer be specified by environment variables using the __ENV__
syntax. This functionality was broken since the last change to the keyspec
syntax in 2025.12.

register_simplefb parameter
---------------------------

The ``register_simplefb`` parameter on framebuffer devices has changed from
boolean to an enum with values ``disabled``, ``enabled``, and ``stdout-path``.

Scripts that **read** the parameter will now receive ``"disabled"`` or
``"enabled"`` instead of ``"0"`` or ``"1"``.

Scripts that **write** ``"0"`` or ``"1"`` continue to work.
