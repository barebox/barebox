Emulated targets
================

Some targets barebox runs on are virtualized by emulators like QEMU, which
allows basic testing of barebox functionality without the actual hardware.

Generic DT image
----------------

Supported for ARM and RISC-V. It generates a barebox image that can
be booted with the Linux kernel booting convention, which makes
it suitable to be passed as argument to the QEMU ``-kernel`` option
(as well as booted just like Linux from barebox or other bootloaders).

The device tree can be passed externally via QEMU's ``-dtb`` option, but
also the QEMU internal device tree can be used.

The latter can be very useful with :ref:`virtio_sect`, because QEMU will
fix up the virtio mmio regions into the device tree and barebox will
discover the devices automatically, analogously to what it does with
VirtIO over PCI.

Running in QEMU
---------------

Emulated targets can be started interactively with ``pytest --interactive``::

  # Run x86 VM runnig the EFI payload from efi_defconfig
  pytest --lg-env test/x86/efi_defconfig.yaml --interactive

  # Identical to above, provided the CONFIG_NAME=efi_defconfig
  pytest --interactive

The test suite can be run by omitting the ``--interactive``.
For more information, see the :ref:`labgrid` section in the
:ref:`contributing` guide.
