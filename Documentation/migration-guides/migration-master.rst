:orphan:

Boards
------

TQMA8MPxL
^^^^^^^^^

The config option has been renamed from ``CONFIG_MACH_TQ_MBA8MPXL`` to
``CONFIG_MACH_TQ_MBA8MPXX`` to accommodate the support for TQMA8MPxS
boards with the same binary.
The binary has been renamed from ``barebox-tqma8mpxl.img``
to ``barebox-tqma8mpxx.img``.

HABv4-enablement
^^^^^^^^^^^^^^^^

When the SRK hash is written, the corresponding fuse bank is now locked automatically.
With that the ``IMX_SRK_HASH_WRITE_LOCK`` flag is removed.
