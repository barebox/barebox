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

test/emulate.pl
---------------

The ``emulate.pl`` script shipped with barebox can be used to easily
start VMs. It reads a number of YAML files in ``test/$ARCH``, which
describe some virtualized targets that barebox is known to run on.

Controlled by command line options, these targets are built with
tuxmake if available and loaded into the emulator for either interactive
use or for automated testing with Labgrid ``QEMUDriver``.

.. _tuxmake: https://pypi.org/project/tuxmake/
.. _Labgrid: https://labgrid.org

Install dependencies for interactive use::

  cpan YAML::XS # or use e.g. libyaml-libyaml-perl on Debian
  pip3 install tuxmake # optional

Example usage::

  # Switch to barebox source directory
  cd barebox

  # emulate x86 VM runnig the EFI payload from efi_defconfig
  ARCH=x86 ./test/emulate.pl efi_defconfig

  # build all MIPS targets known to emulate.pl and exit
  ARCH=mips ./test/emulate.pl --no-emulate

The script can also be used with a precompiled barebox tree::

  # Switch to build directory
  export KBUILD_OUTPUT=build

  # run a barebox image built outside tuxmake on an ARM virt machine
  ARCH=arm ./test/emulate.pl virt@multi_v7_defconfig --no-tuxmake

  # run tests instead of starting emulator interactively
  ARCH=arm ./test/emulate.pl virt@multi_v7_defconfig --no-tuxmake --test

``emulate.pl`` also has some knowledge on paravirtualized devices::

  # Run target and pass a block device (here /dev/virtioblk0)
  ARCH=riscv ./test/emulate.pl --blk=rootfs.ext4 rv64i_defconfig

Needed command line options can be passed directly to the
emulator/``pytest`` as well by placing them behind ``--``::

  # appends -device ? to the command line. Add -n to see the final result
  ARCH=riscv ./test/emulate.pl rv64i_defconfig -- -device ?

For a complete listing of options run ``./test/emulate.pl -h``.
