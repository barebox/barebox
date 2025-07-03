Release v2025.07.0
==================

Configuration options
---------------------

* The legacy ARM 32-bit ATAGS mechanism is now disabled by default.
  It can be re-enabled using the newly added ``CONFIG_BOOT_ATAGS`` option.
  Use of ATAGS is deprecated. Users should migrate to OpenFirmware device trees.

* ``CONFIG_MALLOC_DLMALLOC`` has been removed in favor of ``CONFIG_MALLOC_TLSF``.
  This should be a drop-in replacement, but it may unearth memory safety issues
  that went unnoticed before. If this is suspected, ``CONFIG_KASAN`` should
  help in debugging them on ARM.

Board support
-------------

* barebox now warns at runtime about boards that disable the option
  ``CONFIG_DEEP_PROBE_DEFAULT`` and list neither the ``barebox,deep-probe``
  nor ``barebox,disable-deep-probe`` property in the top-level device tree node.
  It's recommended that all boards switch to deep probe.
  If deep probe breaks your platform, please report to the mailing list
  and set ``barebox,disable-deep-probe`` in your device tree.

* The hardcoded i.MX6ULL 14x14 EVK partition layout for barebox and its
  environment on USDHC2 has changed. The split moved from 768/256 to 896/128 KiB.
