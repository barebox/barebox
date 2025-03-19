.. _networking:

Networking
==========

barebox has IPv4 networking support. Several protocols such as :ref:`DHCP
<command_dhcp>`, NFS and TFTP are supported.

Network configuration
---------------------

Lowlevel network device configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Network devices are configured with a set of device specific variables:

+-------------------+--------------+----------------------------------------------------+
| name              | type         |                                                    |
+===================+==============+====================================================+
| <devname>.mode    | enum         | "dhcp": DHCP is used to get IP address and netmask |
|                   |              | "static": Static IP setup described by variables   |
|                   |              | below                                              |
|                   |              | "disabled": Interface unused                       |
+-------------------+--------------+----------------------------------------------------+
| <devname>.ipaddr  | ipv4 address | The IP address when using static configuration     |
+-------------------+--------------+----------------------------------------------------+
| <devname>.netmask | ipv4 address | The netmask when using static configuration        |
+-------------------+--------------+----------------------------------------------------+
| <devname>.gateway | ipv4 address | Alias for global.net.gateway. For                  |
|                   |              | compatibility, do not use.                         |
+-------------------+--------------+----------------------------------------------------+
| <devname>.serverip| ipv4 address | Alias for global.net.server. For                   |
|                   |              | compatibility, do not use.                         |
+-------------------+--------------+----------------------------------------------------+
| <devname>.ethaddr | MAC address  | The MAC address of this device                     |
+-------------------+--------------+----------------------------------------------------+

Additionally there are some more variables that are not specific to a
device:

+------------------------------+--------------+------------------------------------------------+
| name                         | type         |                                                |
+==============================+==============+================================================+
| global.net.gateway           | ipv4 host    | The network gateway used when a host is not in |
|                              |              | any directly visible subnet. May be set        |
|                              |              | automatically by DHCP.                         |
+------------------------------+--------------+------------------------------------------------+
| global.net.server            | hostname or  | The default server used by the defaultenv boot |
|                              | ipv4 address | scripts for NFS and TFTP; see                  |
|                              |              | :ref:`booting_linux_net`.                      |
|                              |              | If unspecified, may be set by DHCP.            |
+------------------------------+--------------+------------------------------------------------+
| global.net.nameserver        | ipv4 address | The DNS server used for resolving host names.  |
|                              |              | May be set by DHCP.                            |
+------------------------------+--------------+------------------------------------------------+
| global.net.ifup_force_detect | boolean      | Set to true if your network device is not      |
|                              |              | detected automatically during start (i.e. for  |
|                              |              | USB network adapters).                         |
+------------------------------+--------------+------------------------------------------------+

The first step for networking is configuring the network device. The network
device is usually ``eth0``. The current configuration can be viewed with the
:ref:`devinfo <command_devinfo>` command:

.. code-block:: sh

  barebox:/ devinfo eth0
  Parameters:
    ethaddr: 00:1c:49:01:03:4b
    ipaddr: 192.168.23.197
    netmask: 255.255.0.0
    [...]

The configuration can be changed on the command line with:

.. code-block:: sh

  eth0.ipaddr=172.0.0.10

The :ref:`dhcp command <command_dhcp>` will change the settings based on the answer
from the DHCP server.

To make the network device settings persistent across reboots there is a nonvolatile
variable (nvvar) for each of the variables above. The network device specific variables
are:

.. code-block:: sh

  nv.dev.<devname>.mode
  nv.dev.<devname>.ipaddr
  nv.dev.<devname>.netmask
  nv.dev.<devname>.ethaddr

The others are:

.. code-block:: sh

  nv.net.gateway
  nv.net.server
  nv.net.nameserver

A typical simple network setting is to use DHCP. Provided the network interface is eth0
then this would configure the network device for DHCP:

.. code-block:: sh

  nv dev.eth0.mode=dhcp

(In fact DHCP is the default, so the above is not necessary)

A static setup would look like:

.. code-block:: sh

  nv dev.eth0.mode=static
  nv dev.eth0.ipaddr=192.168.0.17
  nv dev.eth0.netmask=255.255.0.0
  nv net.server=192.168.0.1

The settings can be activated with the :ref:`ifup command <command_ifup>`:

.. code-block:: sh

  ifup eth0

or:

.. code-block:: sh

  ifup -a

'ifup -a' will activate all ethernet interfaces, also the ones on USB.

Network filesystems
-------------------

barebox supports NFS and TFTP both with commands (:ref:`nfs <command_nfs>` and
:ref:`tftp <command_tftp>`) and as filesystem implementations; see
:ref:`filesystems_nfs` and :ref:`filesystems_tftp` for more information. After
the network device has been brought up, a network filesystem can be mounted
with:

.. code-block:: sh

  mount -t tftp 192.168.2.1 /mnt

or

.. code-block:: sh

  mount -t nfs 192.168.2.1:/export none /mnt

.. _network_filesystems_automounts:

Automounts
^^^^^^^^^^

For user convenience, the default ``automount`` init script runs
the :ref:`automount command <command_automount>` to create automounts for
both TFTP and NFS. On first access, an Ethernet interface will be brought
up and file operations will be forwarded to a host specified by global
variables:

- ``/mnt/tftp``: will use ``$global.net.server`` as TFTP server

- ``/mnt/nfs``: will use ``$global.net.server`` as NFS server
  and ``/home/${global.user}/nfsroot/${global.hostname}`` as nfsroot.
  By default, a RPC lookup will be conducted to determine mount and
  NFS ports, but these can be overridden together using a user-specified
  by means of ``global.nfs.port``. The latter is equivalent to specifying
  ``-o port=$global.nfs.port,mountport=$global.nfs.port`` as argument
  to the :ref:`mount command <command_mount>`.

Network console
---------------

barebox has a UDP-based network console. If enabled in the config, you will see
something like this during startup::

  registered netconsole as netconsole

By default the network console is disabled during runtime to prevent security
risks. It can be enabled using:

.. code-block:: sh

  netconsole.ip=192.168.23.2
  netconsole.active=ioe

This will send UDP packets to a PC with IP address 192.168.23.2 and port 6666.

The ``netconsole.active`` parameter consists of the fields "input" (i),
"output" (o) and "error" (e); if the fields are set, the respective channel is
activated on the network console.

On the PC side, the ``scripts/netconsole`` script can be used to remote control
barebox:

.. code-block:: sh

  scripts/netconsole <board IP> 6666

The netconsole can be used just like any other console. Note, however, that the
simple console protocol is UDP based, so there is no guarantee about packet
loss.

DSA (Distributed Switch Architecture) Support in Barebox
--------------------------------------------------------

Barebox includes support for DSA (Distributed Switch Architecture), allowing
for basic configuration and management of network switches within the
bootloader.

DSA enables network devices to use a switch framework where each port of the
switch is treated as a separate network interface, while still allowing packet
forwarding between ports when enabled.

DSA Configuration
^^^^^^^^^^^^^^^^^

DSA switches are managed through device parameters, similar to network
interfaces. Each switch is identified as a separate device, typically named
``switch0`` or ``switch@00`` depending on the bus address.

Global Forwarding Control
"""""""""""""""""""""""""

A parameter, ``forwarding``, allows configuring whether a switch operates
in **isolated mode** (default) or **forwarding mode**:

- **Isolated Mode** (default): Each port is treated as an independent interface,
  with no forwarding between ports.

- **Forwarding Mode**: The switch allows forwarding of packets between ports,
  acting as a traditional non-managed switch.

To check the current DSA switch settings, use:

.. code-block:: sh

  barebox:/ devinfo switch0
  Parameters:
    forwarding: 0 (type: bool)

To enable forwarding mode:

.. code-block:: sh

  dev.switch0.forwarding=1

To disable forwarding and revert to isolated mode:

.. code-block:: sh

  dev.switch0.forwarding=0

Persisting Configuration
""""""""""""""""""""""""

To ensure that the forwarding mode setting persists across reboots,
it can be stored as a nonvolatile (nv) variable:

.. code-block:: sh

  nv dev.switch0.forwarding=1

This will configure the switch to always boot in forwarding mode.

Integration with Network Interfaces
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Each port of the DSA switch is exposed as an independent network interface in
Barebox. However, when forwarding is enabled, packets can be forwarded between
these interfaces without requiring intervention from the CPU.

To configure network settings for individual ports, use standard network
configuration variables (e.g., ``eth0.ipaddr``, ``eth0.mode``, etc.). These
settings are applied per port and are independent of the global switch
forwarding configuration.
