Canon DIGIC
===========

Canon PowerShot A1100 IS
------------------------

Running barebox on QEMU
^^^^^^^^^^^^^^^^^^^^^^^

QEMU supports Canon A1100 camera emulation since version 2.0.

Usage::

  $ qemu-system-arm -M canon-a1100 \
      -nographic -monitor null -serial stdio \
      -bios barebox.canon-a1100.bin
