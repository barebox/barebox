:orphan:

Removal of deprecated CONFIG_BOOTM_OPTEE
----------------------------------------

The support for late loading of OP-TEE had been deprecated and ultimately
removed as it greatly increased the attack surface and was only supported
on 32-bit ARM systems.

OP-TEE loading is now only supported
:ref:`in the prebootloader <optee_early_loading>`.

For i.MX6 boards, this can be enabled by enabling
``CONFIG_FIRMWARE_IMX6_OPTEE``.

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
