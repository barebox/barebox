Networking
==========

barebox has IPv4 networking support. Several protocols such as :ref:`DHCP
<command_dhcp>`, NFS and TFTP are supported.

Network configuration
---------------------

The first step for networking is configuring the network device. The network
device is usually ``eth0``. The current configuration can be viewed with the
:ref:`devinfo <command_devinfo>` command:

.. code-block:: sh

  barebox:/ devinfo eth0
  Parameters:
    ethaddr: 00:1c:49:01:03:4b
    gateway: 192.168.23.1
    ipaddr: 192.168.23.197
    netmask: 255.255.0.0
    serverip: 192.168.23.1

The configuration can be changed on the command line with:

.. code-block:: sh

  eth0.ipaddr=172.0.0.10

The :ref:`dhcp command <command_dhcp>` will change the settings based on the answer
from the DHCP server.

This low-level configuration of the network interface is often not necessary. Normally
the network settings should be edited in ``/env/network/eth0``, then the network interface
can be brought up using the :ref:`ifup command <command_ifup>`.

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
