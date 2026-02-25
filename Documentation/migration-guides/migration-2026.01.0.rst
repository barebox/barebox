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

The ``linux.bootargs`` file system device parameter (e.g. ``ext40.linux.bootargs``)
has been replaced by the two variables ``${fsdev}.linux.bootargs.root`` and
``${fsdev}.linux.bootargs.rootopts``, splitting the previous bootargs
into three parts. For example, the previous::

    ext40.linux.bootargs="root=/dev/nfs nfsroot=192.168.1.1:/rootfs"

becomes::

    ext40.linux.bootargs.root="/dev/nfs"
    ext40.linux.bootargs.rootopts="nfsroot=192.168.1.1:/rootfs"

Scripts that access these parameters will need to be adapted to replace
``{linux.bootargs}`` with ``root={linux.bootargs.root} {linux.bootargs.rootopts}``.

``global.linux.bootargs.*`` is unaffected by this change.

Boards
------

ARM i.MX6 RIoTboard
^^^^^^^^^^^^^^^^^^^

The barebox update handler has been changed to write barebox to the
eMMC boot partitions rather than the user data area.
