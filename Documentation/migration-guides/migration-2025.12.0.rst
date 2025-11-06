Release v2025.12.0
==================

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
