:orphan:

global.partitions.first_usable_lba removed
------------------------------------------

The ``global.partitions.first_usable_lba`` variable has been removed.
Use ``global.partitions.first_partition_offset`` instead.

The new variable is a byte offset used by free-space searches for new
partitions, for example ``parted mkpart_size``. The default is
``8388608`` bytes (8 MiB). To keep an old configuration, multiply the
old ``first_usable_lba`` value by 512.

ARCH=arm64
----------

Use of ``ARCH=arm`` for 64-bit ARM builds is deprecated and now emits
a warning. Users should change build scripts to use ``ARCH=arm64``
instead when targetting ARMv8.

Removal of deprecated CONFIG_BOOTM_OPTEE
----------------------------------------

The support for late loading of OP-TEE had been deprecated and ultimately
removed as it greatly increased the attack surface and was only supported
on 32-bit ARM systems.

OP-TEE loading is now only supported
:ref:`in the prebootloader <optee_early_loading>`.

For i.MX6 boards, this can be enabled by enabling
``CONFIG_FIRMWARE_IMX6_OPTEE``.
