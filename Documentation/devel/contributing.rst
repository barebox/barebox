.. _contributing:

Contributing
============

barebox is developed in the open, and contributions are welcome.

This document describes how to prepare and submit patches, how to build and test
barebox across multiple configurations, and how to reproduce CI results locally.

Coding Style
------------

barebox follows the general principles of the
`Linux kernel coding style <https://www.kernel.org/doc/html/latest/process/coding-style.html>`_.
Consistent formatting, clear structure, and portability are key goals.

It's an explicit goal of barebox that a single build should be able
to cover *all* boards across *all* SoCs supported for a given architecture.

To that end, please avoid constructs that reduce portability:

* ``#ifdef CONFIG_`` blocks in generic code that make it impossible
  to support multiple boards with the same build

* Use of weak symbols to override core behavior

License and DCO
---------------

barebox is free software distributed under the terms of the
GNU General Public License version 2. See the top-level ``COPYING``
document for the full terms.

All contributions need to include a ``Signed-off-by: Name <my@mail>`` in their
commit message to certify agreement to the `Developer Certificate of Origin <https://developercertificate.org/>`_.

Patch Workflow
--------------

barebox uses a similar patch process as the Linux kernel, so most of the
`Linux guide for submitting patches <https://www.kernel.org/doc/html/latest/process/submitting-patches.html>`_
also applies to barebox, except for the need to select your recipient;
all barebox patches go to the same list.

Patch series can be sent and fetched from the list using `b4 <https://pypi.org/project/b4/>`_ ::

   b4 shazam -M https://lore.barebox.org/$messageid # replace with link

Fixes should apply on master and new features on the next branch.

If a series fails to apply, ``b4`` can determine/guess the base
and have ``FETCH_HEAD`` point at it::

   b4 shazam -H https://lore.kernel.org/$messageid # URL can be omitted

``b4`` can also send patches to the barebox mailing list and offers
a web relay for when git-send-email is not feasible (e.g. corporate proxies).

After configuring your mail settings (or setting up the web relay),
patches can be sent with::

  git config sendemail.to barebox@lists.infradead.org
  b4 send

See the `b4 documentation <https://b4.docs.kernel.org/en/latest/>`_ for details.

Continuous Integration
----------------------

All patches applied to barebox will automatically be built and tested using
GitHub Actions. The CI performs a range of checks, including:

* Building all configs across architectures using ``MAKEALL``.
* Running tests against emulated targets using Labgrid
* Building host tools against musl
* Shuffle make dependency build order to find missing dependencies

You can run the exact same tests yourself by forking the
`the project on Github <https://github.com/barebox/barebox>`.

Read along for instructions on reproducing locally.

Building Multiple Configurations
--------------------------------

The ``MAKEALL`` script is a wrapper around ``make`` to more easily build
multiple configurations::

  ./MAKEALL -a arm imx_v7_defconfig imx_v8_defconfig

When cross-compiling, you need to set one of::

  CROSS_COMPILE
  CROSS_COMPILE_<arch>
  CROSS_COMPILE_<defconfig>

You can also also build all configs for an architecture by omitting
configs or just build everything at once::

  ./MAKEALL

See the help output of ``MAKEALL`` for more information.

Container Environment
---------------------

The barebox-ci container provides an easy way to run ``MAKEALL`` against
all configurations supported by barebox, even if the host system
lacks the appropriate toolchains. It ships with everything CI needs
and predefines ``CROSS_COMPILE_<arch>`` environment variables for
all supported architectures::

  # Build all configurations for RISC-V, no runtime test
  scripts/container.sh ./MAKEALL -a riscv

This ensures consistent results across developer systems and the CI.

.. _labgrid:

Labgrid
-------

Labgrid is used to run the barebox test suite, both on real and emulated
hardware. A number of YAML files located in ``test/$ARCH`` describe some
of the virtualized targets that barebox is known to run on.

barebox makes use of recent labgrid features, so you may need to install
it directly from PyPI instead of your distro's package repositories::

  pipx install pytest
  pipx inject pytest labgrid

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

  # Run interactively with graphics output
  pytest --lg-env test/mips/qemu-malta_defconfig.yaml --interactive --graphics

For testing, the QEMU fw_cfg and virtfs support is particularly useful::

  # inject boot.sh file in working directory into barebox environment
  # at /env/boot/fit and set /env/nv/boot.default to fit
  pytest --lg-env test/arm/virt@multi_v8_defconfig.yaml \
     --env nv/boot.default=fit --env boot/fit=@boot.sh

  # make available the host's local working directory in barebox as
  # /mnt/9p/host
  pytest --lg-env test/arm/virt@multi_v8_defconfig.yaml \
     --fs host=.

For a complete listing of possible options run ``pytest --help``.

The barebox-ci container already ships with labgrid and pytest.

``MAKEALL`` can also be passed a Labgrid environment file to
build and execute pytest afterwards, which is useful in combination::

  # Run MAKEALL and possibly pytest in the container
  alias MAKEALL="scripts/container.sh ./MAKEALL"

  # Build and test a single configuration
  MAKEALL test/mips/qemu-maltael_defconfig.yaml

  # Build all MIPS platforms that can be tested
  MAKEALL test/mips/*.yaml


Reproducing Shuffle Failures
----------------------------

The CI performs randomized build ordering checks using ``make --shuffle``
to detect missing dependencies between build targets.

This may lead to occasional build failures, which is a good thing, because
it allows us to fix issues, before downstream uns into them.

If a failure occurs, the log will include a ``shuffle=`` value::

  build (arm, virt32_secure_defconfig)

  <snip>

    HOSTCC  scripts/dtc/dtc-lexer.lex.o
    SCONFPP include/generated/sconfig_names.h
  make[1]: *** [/__w/barebox/barebox/Makefile:1230: include/generated/sconfig_names.h] Error 2 shuffle=857479879
  make[1]: *** Deleting file 'include/generated/sconfig_names.h'
  make[1]: *** Waiting for unfinished jobs....
    HOSTLD  scripts/dtc/fdtget
    HOSTLD  scripts/dtc/dtc
  make: *** [Makefile:185: sub-make] Error 2 shuffle=857479879
  Compile: FAILED
  Compiled in    7s

  --------------------- SUMMARY ----------------------------
  defconfigs compiled: 1
  	defconfigs with errors:   1 ( virt32_secure_defconfig )
  Total time spent:    7s


You should be able to reproduce the same issue locally by running::

  ./scripts/container.sh -e GNUMAKEFLAGS=--shuffle=857479879 \
	./MAKEALL -l '' -v 0 -a arm virt32_secure_defconfig

Replace ``857479879`` above with the actual value reported by the CI.
This reproduces the same randomized build sequence locally to help
identify dependency issues.
