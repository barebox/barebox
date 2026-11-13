devboot is now an option of the boot command
---------------------------------------------

devboot is implemented in C as the ``-D`` option of the ``boot`` command,
which needs ``CONFIG_BOOT_DEVBOOT``. The ``devboot`` command remains as a
shorthand for ``boot -D``, so ``devboot mmc`` keeps working as before.

The ``devboot`` boot target has been removed. Configurations booting with
devboot by default through it, like::

  nv boot.default=devboot@system0

now fail with ``Nothing bootable found on 'devboot@system0'``. Set the
boot target directly and enable devboot separately instead::

  nv boot.default=system0
  nv boot.devboot=1

Kernel command line fragments set by the devboot script no longer need the
``global.linux.bootargs.devboot.`` namespace: all changes the script makes
to the boot related globalvars are reverted once the ``boot`` command
returns.
