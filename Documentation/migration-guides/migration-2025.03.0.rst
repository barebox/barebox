Release v2025.03.0
==================

OMAP console fixup
------------------

The option ``CONFIG_DRIVER_SERIAL_NS16550_OMAP_TTYS`` now controls whether
barebox fixes up ``ttyS`` or ``ttyO`` into the kernel command line
``console=`` option.

fixed-partitions on SD/MMC
--------------------------

Linux v6.13-rc1 added ``CONFIG_OF_PARTITION``, which interprets
``fixed partitions`` nodes in SD/MMC nods differently to the barebox
behavior: It ignores the GPT/MBR if fixed-partitions are specified,
instead of allowing them to co-exist like in barebox.

To accommodate both the existing barebox and the new kernel functionality,
users should replace ``fixed-partitions``-compatible device tree subnodes
of SD/eMMC with ``barebox,fixed-partitions``.

Similarly, the default for ``global.of_partition_binding`` has been changed
to the new ``adaptive`` option, which fixes up ``barebox,fixed-partitions``
except for MTD.
