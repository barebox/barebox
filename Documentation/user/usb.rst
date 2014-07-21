USB support
===========

USB host support
----------------

barebox has support for both USB host and USB device mode. USB devices
take a long time to probe, so they are not probed automatically. Probing
has to be triggered using the :ref:`command_usb` or :ref:`command_detect` command.
USB devices in barebox are not hot-pluggable. It is expected that USB
devices are not disconnected while barebox is running.

USB Networking
^^^^^^^^^^^^^^

barebox supports ASIX-compatible devices and the SMSC95xx. After
detection, the device shows up as eth0 and can be used like a regular network
device.

To use a USB network device together with the :ref:`command_ifup` command, add the
following to ``/env/network/eth0-discover``:

.. code-block:: sh

  #!/bin/sh

  usb

USB mass storage
^^^^^^^^^^^^^^^^

barebox supports USB mass storage devices. After probing them with the :ref:`command_usb`
command, they show up as ``/dev/diskx`` and can be used like any other device.

USB device support
------------------

DFU
^^^

USB Device Firmware Upgrade (DFU) is an official USB device class specification of the USB
Implementers Forum. It provides a vendor-independent way to update the firmware of embedded
devices. The current specification is version 1.1 and can be downloaded here:
http://www.usb.org/developers/devclass_docs/DFU_1.1.pdf

On the barebox side, the update is handled with the :ref:`command_dfu` command.
It is passed a list of partitions to provide to the host. The partition list
has the form ``<file>(<name>)<flags>``.  ``file`` is the path to the device or
regular file which shall be updated. ``name`` is the name under which the partition
shall be provided to the host. For the possible ``flags`` see
:ref:`command_dfu`. A typical ``dfu`` command could look like this:

.. code-block:: sh

  dfu "/dev/nand0.barebox.bb(barebox)sr,/dev/nand0.kernel.bb(kernel)r,/dev/nand0.root.bb(root)r"

On the host side, the tool `dfu-util <http://dfu-util.gnumonks.org/>`_ can be used
to update the partitions. It is available for most distributions and typically
supports the following options::

  dfu-util -h
  Usage: dfu-util [options] ...
    -h --help                     Print this help message
    -V --version                  Print the version number
    -v --verbose                  Print verbose debug statements
    -l --list                     List the currently attached DFU capable USB devices
    -e --detach                   Detach the currently attached DFU capable USB devices
    -d --device vendor:product    Specify Vendor/Product ID of DFU device
    -p --path bus-port. ... .port Specify path to DFU device
    -c --cfg config_nr            Specify the Configuration of DFU device
    -i --intf intf_nr             Specify the DFU Interface number
    -a --alt alt                  Specify the Altsetting of the DFU Interface
                                  by name or by number
    -t --transfer-size            Specify the number of bytes per USB Transfer
    -U --upload file              Read firmware from device into <file>
    -D --download file            Write firmware from <file> into device
    -R --reset                    Issue USB Reset signalling once we're finished
    -s --dfuse-address address    ST DfuSe mode, specify target address for
                                  raw file download or upload. Not applicable for
                                  DfuSe file (.dfu) downloads

To update the kernel for the above example, you would use something like
the following:

.. code-block:: sh

  dfu-util -D arch/arm/boot/zImage -a kernel

The ``dfu-util`` command automatically finds DFU-capable devices. If there are
multiple devices found, you need to identify one with the ``-d``/``-p`` options.

USB serial console
^^^^^^^^^^^^^^^^^^

barebox can provide a serial console over USB. This can be initialized with the
:ref:`command_usbserial` command. Once the host is plugged in it should show a
new serial device, on Linux for example ``/dev/ttyACM0``.

Android Fastboot support
^^^^^^^^^^^^^^^^^^^^^^^^

barebox has support for the android fastboot protocol. There is no dedicated command
for initializing the fastboot protocol, instead it is integrated into the Multifunction
Composite Gadget, see :`ref:command_usbgadget` for a usage description.

The Fastboot gadget supports the following commands:

- fastboot flash
- fastboot getvar
- fastboot boot
- fastboot reboot

**NOTE** ``fastboot erase`` is not yet implemented. This means flashing MTD partitions
does not yet work.

The barebox Fastboot gadget supports the following non standard extensions:

- ``fastboot getvar all``
  Shows a list of all variables
- ``fastboot oem getenv <varname>``
  Shows a barebox environment variable
- ``fastboot oem setenv <varname>=<value>``
  Sets a barebox environment variable
- ``fastboot oem exec <cmd>``
  executes a shell command. Note the output can't be seen on the host, but the fastboot
  command returns successfully when the barebox command was successful and it fails when
  the barebox command fails.

USB Composite Multifunction Gadget
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

With the Composite Multifunction Gadget it is possible to create a USB device with
multiple functions. A useful combination is creating a Fastboot gadget and a USB serial
console. This combination can be created with:

.. code-block:: sh

  usbgadget -A /dev/mmc2.0(root),/dev/mmc2.1(data) -a

The ``-A`` option will create a Fastboot function providing ``/dev/mmc2.0`` as root
partition and ``/dev/mmc2.1`` as data partition. The ``-a`` option will create a
USB CDC ACM compliant serial device.

Unlike the :ref:`command_dfu` command the ``usbgadget`` command returns immediately
after creating the gadget. The gadget can be removed with ``usbgadget -d``.
