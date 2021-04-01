.. _bareboxdt:

Barebox devicetree handling and bindings
========================================

The preferred way of adding board support to barebox is to have devices
on non-enumerable buses probed from device tree.
barebox provide both the Linux OpenFirmware ``of_*`` and the libfdt ``fdt_`` APIs
for device tree parsing. The former makes porting the device tree specific
bits from Linux device drivers very straight forward, while the latter can be
used for very early (PBL) handling of flattened device trees, should this be
necessary.

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

Device Tree probing largely happens via compatible properties with no special
meaning to the node names themselves. It's thus paramount that any device tree
nodes extended in the barebox device tree are referenced by a phandle, not by
path, to avoid run-time breakage like this::

  # Upstream dts/src/$ARCH/board.dts
  / {
  	leds {
            led-red { /* formerly named red when the barebox DTS was written */
            	/* ... */
            };
        };
  };

  # barebox arch/$ARCH/dts/board.dts
  #include <$ARCH/board.dts>
  / {
  	leds {
            red {
                barebox,default-trigger = "heartbeat";
            };
        };
  };

In the previous example, a device tree sync with upstream resulted in a regression
as the former override became a new node with a single property without effect.

Using phandles avoids this. When no phandle mapping the full path is defined
upstream, the ``&{/path}`` syntax should be used instead, e.g.::

  &{/leds/red} {
      barebox,default-trigger = "heartbeat";
  };

This would lead to a compile error should the ``/leds/red`` path be renamed or
removed. This also applies to uses of ``/delete-node/``.

Only exception to this rule are well-known node names that are specified by
the `specification`_ to be parsed by name. These are: ``chosen``, ``aliases``
and ``cpus``, but **not** ``memory``.

.. _specification: https://www.devicetree.org/specifications/

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
   bindings/firmware/*
   bindings/leds/*
   bindings/misc/*
   bindings/mtd/*
   bindings/rtc/*
