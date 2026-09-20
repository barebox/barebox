.. _bareboxdt:

Barebox devicetree handling and bindings
========================================

The preferred way of adding board support to barebox is to have devices
on non-enumerable buses probed from device tree.
barebox provides both the Linux OpenFirmware ``of_*`` and the libfdt ``fdt_`` APIs
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
nodes extended in the barebox device tree are referenced by label (e.g.
``<&phandle>``), not by path, to avoid run-time breakage like this::

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

The preferred way around this is to use labels directly::

  # Upstream dts/src/$ARCH/board.dts
  / {
  	leds {
            status_led: red { };
        };
  };

  # barebox arch/$ARCH/dts/board.dts
  #include <$ARCH/board.dts>

  &status_led {
      barebox,default-trigger = "heartbeat";
  };

If there's no label defined upstream for the node, but for a parent,
a new label can be constructed from that label and a relative path::

  # Upstream dts/src/$ARCH/board.dts
  / {
  	led_controller: leds {
            red { };
        };
  };

  # barebox arch/$ARCH/dts/board.dts
  #include <$ARCH/board.dts>

  &{led_controller/red} {
      barebox,default-trigger = "heartbeat";
  };

As last resort, the full path shall be used::

  &{/leds/red} {
      barebox,default-trigger = "heartbeat";
  };

Any of these three approaches would lead to a compile error should the
``/leds/red`` path be renamed or removed. This also applies to uses
of ``/delete-node/``.

Only exception to this rule are well-known node names that are specified by
the `specification`_ to be parsed by name. These are: ``chosen``, ``aliases``
and ``cpus``, but **not** ``memory``.

.. _specification: https://www.devicetree.org/specifications/

Built-in Device Tree Overlays
-----------------------------

Extending the upstream device tree in ``arch/$ARCH/dts`` only works for the
device tree barebox has built in, not for one passed in by the boot firmware
or appended to a generic image. Additions that aren't tied to a particular
device tree can be built as an overlay instead: have the board select
``CONFIG_OF_OVERLAY_BUILTIN`` and list the overlay in ``overlay-y``, which
works in any kbuild Makefile::

  overlay-$(CONFIG_MACH_MYBOARD) += myboard.dtbo

The overlay needs the compatible of the board or SoC it belongs to::

  / {
  	compatible = "myvendor,myboard";
  	barebox,assert-available = "/soc/mmc@5b010000";
  };

barebox applies it before probing any device if one of those compatibles is
in the live tree's root compatible. Fragments that find no target are
skipped: the tree at hand just describes a board without that node. What the
overlay can't do without goes into ``barebox,assert-available``, which drops
the overlay as a whole when that node is missing or disabled, see
:ref:`devicetree-barebox-assert-available`. Targets have to be paths:
``&label`` is a phandle, which a foreign device tree can't resolve.

Overlay Base Device Trees
-------------------------

Rather than hardcoding those paths, an overlay can name the device tree it
is written against, normally the SoC ``.dtsi`` compiled on its own::

  overlay-$(CONFIG_ARCH_MYSOC) += mysoc.dtbo
  DTBO_BASE_mysoc := mysoc-symbols

barebox compiles ``arch/$ARCH/dts/mysoc-symbols.dts`` with ``dtc -@`` and
writes the path behind each of its labels to ``mysoc-symbols-paths.h``::

  #include "mysoc-symbols-paths.h"

  BASE_NODE(sdmmc1) { barebox,restart-warm-bootrom; };
  &{/} { aliases { mmc0 = BASE_PATH(sdmmc1); }; };

``BASE_NODE(label)`` is the node labelled ``label``, ``BASE_SUBNODE(label,
name)`` an unlabelled child of it; ``BASE_PATH()``/``BASE_SUBPATH()`` are
the same as a path string.

The finished overlay is applied to the base at build time, so a fragment
without a target, an alias pointing at nothing or an unresolvable label
fails the build instead of the boot. Whether the nodes are *enabled* is up
to the board, so the overlay can still be skipped at runtime.

Device Tree Compiler
--------------------

barebox makes use of the ``dtc`` and ``fdtget`` and the underlying ``libfdt``
from the `Device-Tree Compiler`_ project.

.. _Device-Tree Compiler: https://git.kernel.org/pub/scm/utils/dtc/dtc.git

These utilities are built as part of the barebox build process. Additionally,
libfdt is compiled once more as part of the ``CONFIG_BOARD_GENERIC_DT``
if selected.

Steps to update ``scripts/dtc``:

* Place a ``git-checkout`` of the upstream ``dtc`` directory in the parent
  directory of your barebox ``git-checkout``.
* Run ``scripts/dtc/update-dtc-source.sh`` from the top-level barebox directory.
* Wait till ``dtc`` build, test, install and commit conclude.
* Compile-test with ``CONFIG_BOARD_GENERIC_DT=y``.
* If ``scripts/dtc/Makefile`` or barebox include file changes are necessary,
  apply them manually in a commit preceding the ``dtc`` update.

barebox-specific Bindings
-------------------------

Contents:

.. toctree::
   :glob:
   :maxdepth: 1

   bindings/barebox/*
   bindings/clocks/*
   bindings/firmware/*
   bindings/leds/*
   bindings/misc/*
   bindings/mtd/*
   bindings/power/*
   bindings/regulator/*
   bindings/rtc/*
   bindings/watchdog/*

Automatic Boot Argument Fixups to the Devicetree
------------------------------------------------

barebox automatically fixes up some boot and system information in the device tree.

In the device tree root, barebox fixes up

 * serial-number (if available)
 * machine compatible (if overridden)

In the ``chosen``-node, barebox fixes up

 * barebox-version
 * reset-source
 * reset-source-instance (if available)
 * reset-source-device (node-path, only if available)
 * bootsource
 * boot-hartid (only on RISC-V)

These values can be read from the booted Linux system in ``/proc/device-tree/``
or ``/sys/firmware/devicetree/base``.

.. _of_diff:

To see a dry run of what barebox would fixup, the ``of_diff`` command can be
used::

  # Diff before and after applying fixups on barebox DT
  of_diff - +

  # Diff kernel device tree before and after fixups
  of_diff /mnt/mmc2.0/boot/imx6q-tx6q.dtb +
