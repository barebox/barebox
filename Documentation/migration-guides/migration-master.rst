:orphan:

global.partitions.first_usable_lba removed
------------------------------------------

The ``global.partitions.first_usable_lba`` variable has been removed.
Use ``global.partitions.first_partition_offset`` instead.

The new variable is a byte offset used by free-space searches for new
partitions, for example ``parted mkpart_size``. The default is
``8388608`` bytes (8 MiB). To keep an old configuration, multiply the
old ``first_usable_lba`` value by 512.