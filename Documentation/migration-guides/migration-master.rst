:orphan:

Removal of bootm -c/-s options
------------------------------

The :ref:`command_bootm` options ``-c`` and ``-s`` used to selectively
enable checksum/hash and signature verification, respectively.

They have been removed in favor of the global toggle
:ref:`global.bootm.verify <magicvar_global_bootm_verify>`.
This can be restricted at build-time via setting ``CONFIG_BOOTM_FORCE_SIGNED_IMAGES``
or loosened :ref:`at runtime <use_security-policies>`
via setting ``SCONFIG_BOOT_UNSIGNED_IMAGES``.

The removal is motivated by making it easier to reason about what the active
verification level is, especially as there are now other uses for verified
images like when :ref:`global.of.overlay.path <magicvar_global_of_overlay_path>`
points at a FIT.

Existing users, if any, will fail-secure: The command will now exit with a failure::

  bootm: invalid option -- s

Default environment overlay order
---------------------------------

Releases since v2026.01.0 applied board specific default environment overlays
added with ``defaultenv_append_directory()`` after the environment given
with ``CONFIG_DEFAULT_ENVIRONMENT_PATH`` and thus overwrote its files.

The documented order has been restored: ``CONFIG_DEFAULT_ENVIRONMENT_PATH``
is applied after the board specific overlays again and can overwrite their
files. Overlays only available at runtime, like an environment passed via
QEMU fw_cfg, are still applied last.

BSPs that started to rely on the board specific overlay taking precedence
need to remove the conflicting files from their external environment.

EFI payload reported inverted Secure Boot state
-----------------------------------------------

As EFI payload, barebox reported the inverse of the firmware's Secure Boot
state in ``efi.secure_boot`` and a wrong ``efi.secure_mode`` derived from it.
Both are now ``1`` if Secure Boot is enabled, ``efi.secure_mode`` only if the
firmware is additionally not in Setup Mode.

In general, this should have no security implications: barebox doesn't
evaluate the parameters itself, the firmware enforces Secure Boot according
to its enrolled keys and the parameters were undocumented so far. It may
however invalidate assumptions made by scripts.
