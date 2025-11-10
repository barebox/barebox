Release v2024.10.0
==================

Fastboot
--------

* ``fastboot getvar all`` is now the only way to list fastboot variables.
  The unintended shortcuts (``"al"``, ``"a"`` and ``""``) are now an error.

* ``fastboot getvar`` no longer aborts the boot countdown as the ``fwupd``
  fastboot plugin was found to probe some variables automatically,
  inadvertently halting device boot.
  Other calls like ``fastboot oem`` or ``fastboot flash`` continue to abort
  the countdown as before.

