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
as the "internal devicetree." The barebox devicetree commands work on this
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
