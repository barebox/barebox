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

ZynqMP
^^^^^^

The Linux v7.0 device trees imported into this barebox release no
longer feature an unconditional `"linaro,optee-tz"` compatible OF node.

If OP-TEE use in barebox is desired, this node must be added back to the
barebox device tree.

HABv4-enablement
^^^^^^^^^^^^^^^^

When the SRK hash is written, the corresponding fuse bank is now locked automatically.
With that the ``IMX_SRK_HASH_WRITE_LOCK`` flag is removed.


TF-A v2.14 compatibility
------------------------

TF-A v2.14 has `broken compatibility <https://lists.trustedfirmware.org/archives/list/tf-a@lists.trustedfirmware.org/thread/LKJVRDGRH7F73FWSTZC46I7IT3BRYQXC/>`_
with SCMI consumers that implement only version 2.0 of the clock protocol.
This includes barebox v2026.03.1, but also Linux v6.6.

barebox v2026.04.0 is the first release that's compatible with both TF-A
releases older than v2.14 as well as v2.14 itself.
