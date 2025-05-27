.. _devicetree:

Devicetree support
==================

Flattened Device Tree (FDT) is a data structure for describing the hardware on
a system. On an increasing number of boards, both barebox and the Linux kernel can
probe their devices directly from devicetrees. barebox needs the devicetree compiled
into the binary. The kernel usually does not have a devicetree compiled in; instead,
the kernel expects to be passed a devicetree from the bootloader.

From a bootloader's point of view, using devicetrees has the advantage that the
same devicetree can be used by both the bootloader and the kernel; this
drastically reduces porting effort since the devicetree has to be written only
once (and with luck somebody has already written a devicetree for the kernel).
Having barebox consult a devicetree is highly recommended for new projects.

.. _internal_devicetree:

The internal devicetree
-----------------------

The devicetree consulted by barebox plays a special role. It is referred to
as the "internal devicetree" or "live tree". The barebox devicetree commands work on this
devicetree. The devicetree source (DTS) files are kept in sync with the kernel DTS
files. As the FDT files are meant to be backward compatible, it should always be possible
to start a kernel with the barebox internal devicetree. However, since the barebox
devicetree may not be complete or contain bugs it is always possible to start the
kernel with a devicetree different from the one used by barebox.
If a device has been probed from the devicetree then using the :ref:`command_devinfo`
command on it will show the corresponding devicetree node:

.. code-block:: sh

  barebox@Phytec pcm970:/ devinfo 10002000.wdog
  Resources:
    num: 0
    name: /soc/aipi@10000000/wdog@10002000
    start: 0x10002000
    size: 0x00001000
  Driver: imx-watchdog
  Bus: platform
  Device node: /soc/aipi@10000000/wdog@10002000
  wdog@10002000 {
          compatible = "fsl,imx27-wdt", "fsl,imx21-wdt";
          reg = <0x10002000 0x1000>;
          interrupts = <0x1b>;
          clocks = <0x1 0x4a>;
  };

Devicetree commands
-------------------

barebox has commands to show and manipulate devicetrees. These commands always
work on the internal devicetree. It is possible to add/remove nodes using the
:ref:`command_of_node` command and to add/change/remove properties using the
:ref:`command_of_property` command. To dump devicetrees on the console use the
:ref:`command_of_dump` command.

.. code-block:: sh

  # dump the whole devicetree
  of_dump

  # dump node of_dump /soc/nand@d8000000/
  of_dump /soc/nand@d8000000/

  # create a new node
  of_node -c /chosen/mynode

  # add a property to it
  of_property -s /chosen/mynode/ myproperty myvalue

It is important to know that these commands normally work on the internal
devicetree. If you want to modify the devicetree the kernel is started with
see the -f options to of_property and of_node. This option will register the
operation for later execution on the Kernel devicetree.

Device tree overlays
--------------------

barebox has support for device tree overlays. barebox knows two different trees,
the live tree and the device tree the kernel is started with. Both can be applied
overlays to.

.. note:: Compiling a device tree discards label information by default. To be able
 to use phandles into the base device tree from inside an overlay, pass to dtc the
 ``-@`` option when compiling the base device tree.
 This will populate ``/__symbols__`` in the base device tree.

 Having ``__fixups__`` in the overlay, but no ``__symbols__`` in the base device
 tree is not allowed: ``ERROR: of_resolver: __symbols__ missing from base devicetree``.

Device tree overlays on the live tree
.....................................

While the live tree can be patched by board code, barebox does not
detect any changes to the live tree. To let the overlays have any effect, board
code must make sure the live tree is patched before the devices are instantiated
from it.

The ``CONFIG_OF_OVERLAY_LIVE`` option will need to be enabled to generate
``__symbols__`` into the barebox device tree.

Device tree overlays on the kernel device tree
..............................................

Overlays can be applied to the kernel device tree before it is handed over to
the kernel. The behaviour is controlled by different variables:

``global.of.overlay.dir``
  Overlays are read from this directory. barebox will try to apply all overlays
  found here if not limited by one of the other variables below. When the path
  given here is an absolute path it is used as is. A relative path is relative
  to ``/`` or relative to the rootfs when using bootloader spec.
``global.of.overlay.compatible``
  This is a space separated list of compatibles. Only overlays matching one of
  these compatibles will be applied. When this list is empty then all overlays
  will be applied. Overlays that don't have a compatible are considered being
  always compatible.
``global.of.overlay.filepattern``
  This is a space separated list of file patterns. An overlay is only applied
  when its filename matches one of the patterns. The patterns can contain
  ``*`` and ``?`` as wildcards. The default is ``*`` which means all files are
  applied.
``global.of.overlay.filter``
  This is a space separated list of filters to apply. There are two generic filters:
  ``filepattern`` matches ``global.of.overlay.filepattern`` above, ``compatible`` matches
  ``global.of.overlay.compatible`` above. The default is ``filepattern compatible``
  which means the two generic filters are active. This list may be replaced or
  supplemented by board specific filters.

The kernel device trees need to be built with symbols (``dtc -@`` option) enabled.
For upstream device trees, this is currently done on a case-by-case basis in the
Makefiles::

  DTC_FLAGS_bcm2711-rpi-4-b := -@
