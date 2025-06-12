Release v2025.05.0
==================

Configuration options
---------------------

* ``CONFIG_CMD_NFS`` and the ``nfs`` command have been removed.
  Users should use the NFS filesystem implementation instead via
  :ref:`command_mount`.

* ``CONFIG_STORAGE_BY_UUID`` has been renamed to ``CONFIG_STORAGE_BY_ALIAS``.
  No functional change for the ``diskuuid`` binding use-case.

* ``CONFIG_ROOTWAIT_BOOTARG`` is now enabled by default and will instruct
  barebox to append ``rootwait`` when booting from SD/eMMC/USB/NFS if
  barebox is already configured to generate the ``root=`` option dynamically.

  This can be customized via setting the ``global`` ``linux.rootwait``
  variable or by disabling the option.

Bootloader Specification
------------------------

* Special handling for ``devicetree none`` has been finally removed in the
  bootloader specification parser after triggering an error message for 4 years.
  To migrate, remove the line.

* GPT partitions with ``DPS_TYPE_FLAG_NO_AUTO`` are now ignored in accordance
  with the UAPI discoverable partition specification.

* All XBOOTLDR partitions (without ``DPS_TYPE_FLAG_NO_AUTO``) are now
  considered for bootloader spec files.

  Previously only the first partition with the XBOOTLDR partition type
  was considered.

* The XBOOTLDR GPT Partition Type UUID is now parsed and handled
  identically to the MBR partition type.

  If an XBOOTLDR partition exists, barebox will no longer walk
  all partitions to find further
  bootloader spec entries.

* Any existing ESP partitions are now searched for entries after
  XBOOTLDR partitions if any and before iterating over the full
  list of partitions.

Board support
-------------

* Legacy STM32MP1 images have been finally dropped from barebox after
  having been removed from TF-A in 2022.

  Users should migrate to FIP images with barebox as nontrusted firmware
  (BL33) and the barebox device tree as hardware config.

  Use of multiple device trees is not support directly by the FIP format
  and can be substituted by a barebox image embedding multiple device trees
  and using the hardware config FIP slot only as dummy.
  See the lxa-tac board for an example.

* ``barebox-sama5d27-som1-ek-xload-mmc.img`` has been renamed to
  ``barebox-sama5d27-som1-ek-xload.img`` and
  ``barebox-groboards-sama5d27-giantboard-xload-mmc.img`` has been renamed to
  ``barebox-groboards-sama5d27-giantboard-xload.img`` to reflect
  that the same image is now also bootable from QSPI flash.

* Clock initialization for RK356x is no longer hardcoded, but is now
  specified in the device tree. barebox device trees not including
  ``#include rk356x.dtsi`` (in ``arch/arm/dts``) may regress.

Shell and magic variables
-------------------------

* ``/dev/fw_cfg0`` has been renamed to ``/dev/fw_cfg``

* ``global.boot.default`` is now ``bootsource net`` by default.

  This does not affect users setting ``nv boot.default``, but for
  others, barebox will look for bootloader spec on the bootsource
  medium first.

Build system
------------

* When building for ``ARCH=sandbox``, ``libftdi1`` and ``sdl2``
  libraries are now looked up with ``${CROSS_PKG_CONFIG}`` instead
  of ``${PKG_CONFIG}``. This is not relevant when cross-building
  just the barebox tools, which is the usual use case for sandbox
  cross compilation.

Bisectability
-------------

Following regressions had a wider impact and could falsify
falsify git bisect results:

* Many users of ``read*_poll_is_timeout`` are broken between
  ``554bbc479c09..dea0ad02bbd6``.

* Handling of compressed kernel images on ARM64/RISC-V was broken between
  ``a1248198fa3a..95aeb8d47d10``.
