Release v2026.01.0
==================

<config.h> removal for PowerPC
------------------------------

PowerPC was the last remaining user of per-board ``<config.h>`` files.
The alternative for out-of-tree boards is now to patch
``arch/powerpc/include/asm/config.h`` to include the board's config.h,
like the in-tree boards are already doing.

Filesystems
-----------

``{linux.bootargs}`` is replaced with ``root={linux.bootargs.root} {linux.bootargs.rootopts}``

The variable linux.bootargs has been replaced by the two variables
linux.bootargs.root and linux.bootargs.rootopts, splitting the previous bootargs
into three parts. A nonexistent fixed "root=", then the root filesystem and then
additional optional params for this particular filesystem.

for example the previous::

    linux.bootargs="root=/dev/nfs nfsroot=192.168.1.1:/rootfs"

becomes::

    linux.bootargs.root="/dev/nfs"
    linux.bootargs.rootopts="nfsroot=192.168.1.1:/rootfs"

Boards
------

ARM i.MX6 RIoTboard
^^^^^^^^^^^^^^^^^^^

The barebox update handler has been changed to write barebox to the
eMMC boot partitions rather than the user data area.
