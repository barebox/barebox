Release v2025.07.0
==================

Configuration options
---------------------

* The legacy ARM 32-bit ATAGS mechanism is now disabled by default.
  It can be re-enabled using the newly added ``CONFIG_BOOT_ATAGS`` option.
  Use of ATAGS is deprecated. Users should migrate to OpenFirmware device trees.

  Board support
-------------

* barebox now warns at runtime about boards that disable the option
  ``CONFIG_DEEP_PROBE_DEFAULT`` and list neither the ``barebox,deep-probe``
  nor ``barebox,disable-deep-probe`` property in the top-level device tree node.
  It's recommended that all boards switch to deep probe.
  If deep probe breaks your platform, please report to the mailing list
  and set ``barebox,disable-deep-probe`` in your device tree.
