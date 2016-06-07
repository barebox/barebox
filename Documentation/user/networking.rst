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
| global.net.server            | ipv4 host    | The default server address. If unspecified, may|
|                              |              | be set by DHCP                                 |
+------------------------------+--------------+------------------------------------------------+
| global.net.nameserver        | ipv4 address | The DNS server used for resolving host names.  |
|                              |              | May be set by DHCP                             |
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
variable (nvvar) for each of the varariables above. The network device specific variables
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

**NOTE:** The execution of the mount command can often be hidden behind the
:ref:`automount command <command_automount>`, to make mounting transparent to
the user.

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
