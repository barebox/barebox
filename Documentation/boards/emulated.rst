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

labgrid
-------

Labgrid is used to run the barebox test suite, both on real and emulated
hardware. A number of YAML files located in ``test/$ARCH`` describe some
of the virtualized targets that barebox is known to run on.

Example usage::

  # Run x86 VM runnig the EFI payload from efi_defconfig
  pytest --lg-env test/x86/efi_defconfig.yaml --interactive

  # Run the test suite against the same
  pytest --lg-env test/x86/efi_defconfig.yaml

The above assumes that barebox has already been built for the
configuration and that labgrid is available. If barebox has been
built out-of-tree, the build directory must be pointed at by
``LG_BUILDDIR``, ``KBUILD_OUTPUT`` or a ``build`` symlink.

Additional QEMU command-line options can be added by specifying
them after the ``--qemu`` option::

  # appends -device ? to the command line. Add --dry-run to see the final result
  pytest --lg-env test/riscv/rv64i_defconfig.yaml --interactive --qemu -device '?'

Some of the QEMU options that are used more often also have explicit
support in the test runner, so paravirtualized devices can be added
more easily::

  # Run tests and pass a block device (here /dev/virtioblk0)
  pytest --lg-env test/arm/virt@multi_v8_defconfig.yaml --blk=rootfs.ext4

For a complete listing of possible options run ``pytest --help``.

MAKEALL
-------

The ``MAKEALL`` script is a wrapper around ``make`` to more easily build
multiple configurations. It also accepts YAML Labgrid environment files
as arguments, which will cause it to build and then run the tests::

  ./MAKEALL test/mips/qemu-maltael_defconfig.yaml

This expects ``CROSS_COMPILE`` (or ``CROSS_COMPILE_mips``) to have been
set beforehand to point at an appropriate toolchain prefix.

The barebox-ci container provides an easy way to run ``MAKEALL`` against
all configurations supported by barebox, even if the host system
lacks the appropriate toolchains::

  # Run MAKEALL and possibly pytest in the container
  alias MAKEALL="scripts/container.sh ./MAKEALL"

  # Build a single configuration
  MAKEALL test/mips/qemu-maltael_defconfig.yaml

  # Build all configurations for an architecture, no test
  MAKEALL -a riscv

  # Build all mips platforms that can be tested
  MAKEALL test/mips/*.yaml
