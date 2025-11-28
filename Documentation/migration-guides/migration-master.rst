<config.h> removal for PowerPC
------------------------------

PowerPC was the last remaining user of per-board ``<config.h>`` files.
The alternative for out-of-tree boards is now to patch
``arch/powerpc/include/asm/config.h`` to include the board's config.h,
like the in-tree boards are already doing.
