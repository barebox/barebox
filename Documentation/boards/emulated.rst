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

Emulated targets can be started interactively with ``scripts/qemu_interactive.py``::

  # Run x86 VM runnig the EFI payload from efi_defconfig
  scripts/qemu_interactive.py test/x86/efi_defconfig.yaml

  # Identical to above, provided the CONFIG_NAME=efi_defconfig
  scripts/qemu_interactive.py

The test suite can be run with ``pytest`` instead.
For more information, see the :ref:`labgrid` section in the
:ref:`contributing` guide.

Netconsole over QEMU user networking
------------------------------------

barebox' UDP-based :ref:`network console <network_console>` can also
be used in combination with QEMU. With user-mode networking (SLIRP),
guest-to-host UDP works via NAT out of the box,
but unsolicited host-to-guest UDP requires an explicit port forward::

  scripts/qemu_interactive.py test/arm/multi_v8_defconfig.yaml    \
    --env nv/dev.netconsole.ip=10.0.2.2                          \
    --env nv/dev.netconsole.port=6666                            \
    --env init/netconsole="ifup -a1; netconsole.active=ioe"      \
    --port-forward=6666

This will point netconsole at the SLIRP gateway (``10.0.2.2`` is the host
as seen from the guest) and bring up the interface::

  netconsole: netconsole initialized with 10.0.2.2:6666

The ``i`` flag in ``netconsole.active`` is required for input; without it
only output reaches the host. On the host, you can then interact with
the netconsole via::

  scripts/netconsole -s 127.0.0.1 127.0.0.2 6666
