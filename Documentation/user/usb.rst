USB support
===========

USB host support
----------------

barebox has support for USB host and USB device mode. USB devices
take a long time to probe, so they are not probed automatically. Probing
has to be triggered using the :ref:`command_usb` or :ref:`command_detect` command.
USB devices in barebox are not hotpluggable. It is expected that USB
devices are not disconnected while barebox is running.

USB Networking
^^^^^^^^^^^^^^

barebox supports ASIX compatible devices and the SMSC95xx. After
detection the device shows up as eth0 and can be used like a regular network
device.

To use a USB network device together with the :ref:`command_ifup` command, add the
following to /env/network/eth0-discover:

.. code-block:: sh

  #!/bin/sh

  usb

USB mass storage
^^^^^^^^^^^^^^^^

barebox supports USB mass storage devices. After probing them with the :ref:`command_usb`
they show up as ``/dev/diskx`` and can be used like any other device.

USB device support
------------------

DFU
^^^

USB Device Firmware Upgrade (DFU) is an official USB device class specification of the USB
Implementers Forum. It provides a vendor independent way to update the firmware of embedded
devices. The current specification is version 1.1 and can be downloaded here:
http://www.usb.org/developers/devclass_docs/DFU_1.1.pdf

On barebox side the update is handled with the :ref:`command_dfu` command. It is passes a list
of partitions to provide to the host. The partition list has the form ``<file>(<name>)<flags>``.
``file`` is the path to the device or regular file which shall be updated. ``name`` is the
name under which the partition shall be provided to the host. For the possible ``flags`` see
:ref:`command_dfu`. A typical ``dfu`` command could look like this:

.. code-block:: sh

  dfu "/dev/nand0.barebox.bb(barebox)sr,/dev/nand0.kernel.bb(kernel)r,/dev/nand0.root.bb(root)r"

On the host the tool `dfu-util <http://dfu-util.gnumonks.org/>`_ can be used to update the
partitions. It is available for the most distributions. To update the Kernel for the above
example the following can be used:

.. code-block:: sh

  dfu-util -D arch/arm/boot/zImage -a kernel

The dfu-util command automatically finds dfu capable devices. If there are multiple devices
found it has to be specified with the ``-d``/``-p`` options.
