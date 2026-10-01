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

Deep probe is now the default on STM32MP13 and STM32MP15
--------------------------------------------------------

barebox now enables deep probe for every ``st,stm32mp1xx`` compatible, even
over ``barebox,disable-deep-probe``. Report to the mailing list if that
breaks your board.
