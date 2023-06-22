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

barebox supports r8152, ASIX-compatible devices and the SMSC95xx. After
detection, the device shows up as an extra network device (e.g. eth1) and
can be used like a regular network device.

To use a USB network device together with the :ref:`command_ifup` command, add the
following to ``/env/network/eth0-discover``:

.. code-block:: sh

  #!/bin/sh

  usb

Alternatively, a ``detect -a`` (all) can be forced in ``ifup`` by setting
``global.net.ifup_force_detect=1``.

USB mass storage
^^^^^^^^^^^^^^^^

barebox supports USB mass storage devices. After probing them with the :ref:`command_usb`
command, they show up as ``/dev/diskx`` and can be used like any other device.

USB device support
------------------

barebox supports several different USB gadget drivers:

- Device Firmware Upgrade (DFU)
- Android Fastboot
- USB mass storage
- serial gadget

The recommended way to use USB gadget is with the :ref:`command_usbgadget` command.
While there are individual commands for :ref:`command_dfu` and :ref:`command_usbserial`,
the :ref:`command_usbgadget` commands supports registering composite gadgets, which
exports multiple functions at once. This happens in the "background" without impacting
use of the shell.

Partition description
^^^^^^^^^^^^^^^^^^^^^

The USB gadget commands for Android Fastboot, DFU and the mass storage gadget
take a partition description which describes which barebox partitions are exported via USB.
The partition description is referred to as ``<desc>`` in the command help texts. It has
the general form ``partition(name)flags,partition(name)flags,...``.

The **partition** field is the partition as accessible in barebox. This can be a
path in ``/dev/``, but could also be a regular file.

The **name** field is the name under which the partition shall be exported. This
is the name under which the partition can be found with the host tool.

Several **flags** are supported, each denoted by a single character:

* ``s`` Safe mode. The file is downloaded completely before it is written (DFU specific)
* ``r`` Readback. The partition is allowed to be read back (DFU specific)
* ``c`` The file shall be created if it doesn't exist. Needed when a regular file is exported.
* ``u`` The partition is a MTD device and shall be flashed with a UBI image.
* ``o`` The partition is optional, i.e. if it is not available at initialization time, it is skipped
        instead of aborting the initialization. This is currently only supported for fastboot.

Example:

.. code-block:: sh

  /dev/nand0.barebox.bb(barebox)sr,/kernel(kernel)rc

Board code authors are encouraged to provide a default environment containing
partitions with descriptive names. For boards where this is not specified,
there exist a number of **partition** specifiers for automatically generating entries:

* ``block`` exports all registered block devices (e.g. eMMC and SD)
* ``auto``  currently equivalent to ``block``. May be extended to other flashable
            devices, like EEPROMs, MTD or UBI volumes in future

Example usage of exporting registered block devices, barebox update
handlers and a single file that is created on flashing:

.. code-block:: sh

     detect -a # optional. Detects everything, so auto can register it
     usbgadget -A auto,/tmp/fitimage(fitimage)c -b

DFU
^^^

USB Device Firmware Upgrade (DFU) is an official USB device class specification of the USB
Implementers Forum. It provides a vendor-independent way to update the firmware of embedded
devices. The current specification is version 1.1 and can be downloaded here:
http://www.usb.org/developers/devclass_docs/DFU_1.1.pdf

On the barebox side, the update is handled with the :ref:`command_usbgadget` or the
:ref:`command_dfu` command.

On the host side, the tool `dfu-util <http://dfu-util.gnumonks.org/>`_ can be used
to update the partitions. It is available for most distributions and typically
supports the following options:

.. code-block:: none

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
Composite Gadget, see :ref:`command_usbgadget` for a usage description.

The Fastboot gadget supports the following commands:

- fastboot flash
- fastboot getvar
- fastboot boot
- fastboot reboot

``fastboot flash`` additionally supports image types UBI and Barebox. For UBI
Images and a MTD device as target, ubiformat is called. For a Barebox image
with an available barebox update handler for the fastboot exported device, the
barebox_update is called.

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

**Example booting kernel/devicetree/initrd with fastboot**

In Barebox start the fastboot gadget:

.. code-block:: sh

  usbgadget -A /kernel(kernel)c,/initrd(initrd)c,/devicetree(devicetree)c

On the host you can use this script to start a kernel with kernel, devicetree
and initrd:

.. code-block:: sh

  #!/usr/bin/env bash

  set -e
  set -v

  if [ "$#" -lt 3 ]
  then
          echo "USAGE: $0 <KERNEL> <DT> <INITRD> [<ARGS>]"
          exit 0
  fi

  kernel=$1
  dt=$2
  initrd=$3

  shift 3

  fastboot -i 7531 flash kernel $kernel
  fastboot -i 7531 flash devicetree $dt
  fastboot -i 7531 flash initrd $initrd


  fastboot -i 7531 oem exec 'global linux.bootargs.fa'$ct'=rdinit=/sbin/init'
  if [ $# -gt 0 ]
  then
          ct=1
          for i in $*
          do
                  fastboot -i 7531 oem exec 'global linux.bootargs.fa'$ct'='"\"$i\""
                  ct=$(($ct + 1))
          done
  fi
  timeout -k 5 3 fastboot -i 7531 oem exec -- bootm -o /devicetree -r /initrd /kernel

USB Mass storage gadget
^^^^^^^^^^^^^^^^^^^^^^^

Example exporting barebox block devices to a USB host::

.. code-block:: sh

  usbgadget -S /dev/mmc0(emmc),/dev/mmc1(sd)


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

USB OTG support
---------------

barebox does not have true USB OTG support. However, barebox supports some USB cores in
both host and device mode. If these are specified for otg in the device tree
(dr_mode = "otg";) barebox registers a OTG device which can be used to decide which
mode shall be used. The device has a ``mode`` parameter which by default has the
value ``otg``. setting this to ``host`` or ``peripheral`` puts the device in the corresponding
mode. Once a specific mode has been selected it can't be changed later anymore.

.. code-block:: sh

  barebox:/ devinfo otg0
  Parameters:
    mode: otg ("otg", "host", "peripheral")
  barebox:/ otg0.mode=host
  musb-hdrc: ConfigData=0xde (UTMI-8, dyn FIFOs, bulk combine, bulk split, HB-ISO Rx, HB-ISO Tx, SoftConn)
  musb-hdrc: MHDRC RTL version 2.0
  musb-hdrc: setup fifo_mode 4
  musb-hdrc: 28/31 max ep, 16384/16384 memory
  barebox:/

USB Type-C support
------------------

barebox can usually stay oblivious to the type of connector used. Sometimes though,
board code and user scripts may want to base their decisions on how a USB-C connector
is connected. Type C drivers can thus register with the Type C driver core to
export a number of device parameters:

- ``$typec0.usb_role`` = { ``none``, ``device``, ``host`` }
- ``$typec0.pwr_role`` = { ``sink``, ``source`` }
- ``$typec0.accessory`` = { ``none``, ``audio``, ``debug`` }

Currently, only the TUSB320 is supported, but it's straight-forward to port more
drivers from Linux.

USB Gadget autostart Support
----------------------------

Barebox can be configured to start usbgadget automatically by using global variables,
instead of creating boot script. This can be useful if autostart policy should be
chosen at boot time from other driver or script.
To get usbgadget autostart support barebox has to be compiled with
CONFIG_USB_GADGET_AUTOSTART enabled.

USB Gadget autostart Options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``global.usbgadget.autostart``
  Boolean flag. If set to 1, usbgadget will be started automatically on boot and
  enable USB OTG mode. (Default 0).
``global.usbgadget.acm``
  Boolean flag. If set to 1, CDC ACM function will be created.
  See :ref:`command_usbgadget` -a. (Default 0).
``global.system.partitions``
  Common function description for all of DFU, fastboot and USB mass storage
  gadgets. Both Fastboot and DFU partitions also have dedicated override
  variables for backwards-compatibility:

``global.usbgadget.dfu_function``
  Function description for DFU. See :ref:`command_usbgadget` -D [desc].
``global.fastboot.partitions``
  Function description for fastboot. See :ref:`command_usbgadget` -A [desc].
``global.fastboot.bbu``
  Export barebox update handlers. See :ref:`command_usbgadget` -b. (Default 0).
