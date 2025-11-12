Release v2025.12.0
==================

Shell
-----

* An optional parameter was added to the `-c` option of `dmesg` allowing
  configuration of the number of lines to remain in the log buffer after
  clearing. When no parameter is provided to `-c`, zero is assumed, and no
  lines are retained. Earlier versions always left 10 lines of logs remain in
  the log buffer.

Configuration options
---------------------

* The syntax of ``CONFIG_CRYPTO_PUBLIC_KEYS`` was updated with the introduction
  of the keyring feature. Previously, keys selected via
  ``CONFIG_CRYPTO_PUBLIC_KEYS`` were only used for fitimage verification. Now,
  fitimage verification uses the "fit" keyring. "fit" will be selected as the
  default keyring for transition but a warning will be emitted when no keyring
  is explicitly provided. Existing users should update their keyspec for
  fitimage public keys to ``keyring=fit[,fit-hint=<fit_hint>]:<crt>``
* The fit-hints in ``CONFIG_CRYPTO_PUBLIC_KEYS`` are now limited to identifiers
  matching the regular expression ``[a-zA-Z][a-zA-Z0-9_-]*``. Public keys with
  a ``fit-hint`` not conforming to this results in an error, affected key-hints
  must be changed. Please reach out to the mailing list if this causes issues.
