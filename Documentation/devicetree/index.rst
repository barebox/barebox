Barebox devicetree handling and bindings
========================================

The preferred way of adding board support to barebox is to have devices
on non-enumerable buses probed from device tree.
barebox imports the Linux OpenFirmware ``of_*``-API functions for device tree
parsing, which makes porting the device tree specific bits from device drivers
very straight forward.

Additionally, barebox has support for programmatically fixing up device trees
it passes to the kernel, either directly via ``of_register_fixup`` or via device
tree overlays.

Upstream Device Trees
---------------------

barebox regularly synchronizes with the Linux kernel device tree definitions
via the `kernel.org Split device-tree repository`_.
They are located under the top-level ``dts/`` directory.

Patches against ``dts/`` and its subdirectories are not accepted upstream.

.. _kernel.org Split device-tree repository: https://git.kernel.org/pub/scm/linux/kernel/git/devicetree/devicetree-rebasing.git/

barebox Device Trees
--------------------

For supporting architectures, barebox device trees are located in
``arch/$ARCH/dts``. Usually the barebox ``board.dts`` imports the upstream
device tree under ``dts/src/$ARCH`` with ``#include "$ARCH/board.dts"`` and
then extends it with barebox-specifics like :ref:`barebox,state`,
environment or boot-time device configuration.

Device Tree Compiler
--------------------

barebox makes use of the ``dtc`` and ``fdtget`` and the underlying ``libfdt``
from the `Device-Tree Compiler`_ project.

.. _Device-Tree Compiler: https://git.kernel.org/pub/scm/utils/dtc/dtc.git

These utilities are built as part of the barebox build process. Additionally,
libfdt is compiled once more as part of the ``CONFIG_BOARD_ARM_GENERIC_DT``
if selected.

Steps to update ``scripts/dtc``:

* Place a ``git-checkout`` of the upstream ``dtc`` directory in the parent
  directory of your barebox ``git-checkout``.
* Run ``scripts/dtc/update-dtc-source.sh`` from the top-level barebox directory.
* Wait till ``dtc`` build, test, install and commit conclude.
* Compile-test with ``CONFIG_BOARD_ARM_GENERIC_DT=y``.
* If ``scripts/dtc/Makefile`` or barebox include file changes are necessary,
  apply them manually in a commit preceding the ``dtc`` update.

barebox-specific Bindings
-------------------------

Contents:

.. toctree::
   :glob:
   :maxdepth: 1

   bindings/barebox/*
   bindings/leds/*
   bindings/misc/*
   bindings/mtd/*
   bindings/rtc/*
