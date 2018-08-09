Driver model
============

barebox has a driver model. This matches the devices on a board with their
corresponding drivers. From a user's point of view this is mostly visible in the
:ref:`command_devinfo` and :ref:`command_drvinfo` commands. Without arguments
the :ref:`command_devinfo` command will show a hierarchical list of devices
found on the board. As this may be instantiated from the :ref:`devicetree`
there may be devices listed for which no driver is available. The
:ref:`command_drvinfo` command shows a list of drivers together with the
devices they handle.

:ref:`command_devinfo` also shows a list of device files a device has registered::

 `-- 70008000.esdhc
    `-- mmc1
       `-- 0x00000000-0x1d9bfffff (   7.4 GiB): /dev/mmc1
       `-- 0x00100000-0x040fffff (    64 MiB): /dev/mmc1.0
       `-- 0x04100000-0x240fffff (   512 MiB): /dev/mmc1.1
       `-- 0x24100000-0xe40fffff (     3 GiB): /dev/mmc1.2
       `-- 0xe4100000-0x1640fffff (     2 GiB): /dev/mmc1.3
       `-- 0x00080000-0x000fffff (   512 KiB): /dev/mmc1.barebox-environment

In this case the /dev/mmc1\* are handled by the mmc1 device. It must be noted
that it's desirable that devices (mmc1) have the same name as the device files (/dev/mmc1\*),
but this doesn't always have to be the case.

Device detection
----------------

Some devices like USB or MMC may take a longer time to probe. Probing USB
devices should not delay booting when USB may not even be used. This case is
handled with device detection. The user visible interface to device detection
is the :ref:`command_detect` command. ``detect -l`` shows a list of detectable
devices:

.. code-block:: console

  barebox:/ detect -l
  70004000.esdhc
  70008000.esdhc
  73f80000.usb
  73f80200.usb
  73f80400.usb
  83fe0000.pata
  ata0
  mmc0
  mmc1
  miibus0

A particular device can be detected with ``detect <devname>``:

.. code-block:: console

  barebox:/ detect 73f80200.usb
  Found SMSC USB331x ULPI transceiver (0x0424:0x0006).
  Bus 002 Device 004: ID 13d3:3273 802.11 n WLAN
  mdio_bus: miibus0: probed
  eth0: got preset MAC address: 00:1c:49:01:03:4b
  Bus 002 Device 005: ID 0b95:7720 ZoWii
  Bus 002 Device 002: ID 0424:2514
  Bus 002 Device 001: ID 0000:0000 EHCI Host Controller

For detecting all devices ``detect -a`` can be used.

.. _device_parameters:

Device parameters
-----------------

Devices can have parameters which can be used to configure devices or to provide
additional information for a device. The parameters can be listed with the
:ref:`command_devinfo` command. A ``devinfo <devicename>`` will print the parameters
of a device:

.. code-block:: console

  barebox:/ devinfo eth0
  Parameters:
    ipaddr: 192.168.23.197
    serverip: 192.168.23.1
    gateway: 192.168.23.1
    netmask: 255.255.0.0
    ethaddr: 00:1c:49:01:03:4b

The parameters can be used as shell variables:

.. code-block:: sh

  eth0.ipaddr=192.168.23.15
  echo "my current ip is: $eth0.ipaddr"

device variables may have a type, so assigning wrong values may fail:

.. code-block:: console

  barebox:/ eth0.ipaddr="This is not an IP"
  set parameter: Invalid argument
  barebox:/ echo $?
  1

**HINT:** barebox has tab completion for variables. Typing ``eth0.<TAB><TAB>``
will show the parameters for eth0.
