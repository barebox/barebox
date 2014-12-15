Networking
==========

barebox has IPv4 networking support. Several protocols such as
:ref:`command_dhcp`, :ref:`filesystems_nfs`, :ref:`command_tftp` are
supported.

Network configuration
---------------------

The first step for networking is configuring the network device. The network
device is usually ``eth0``. The current configuration can be viewed with the
:ref:`command_devinfo` command:

.. code-block:: sh

  barebox:/ devinfo eth0
  Parameters:
    ipaddr: 192.168.23.197
    serverip: 192.168.23.1
    gateway: 192.168.23.1
    netmask: 255.255.0.0
    ethaddr: 00:1c:49:01:03:4b

The configuration can be changed on the command line with:

.. code-block:: sh

  eth0.ipaddr=172.0.0.10

The :ref:`command_dhcp` command will change the settings based on the answer
from the DHCP server.

This low-level configuration of the network interface is often not necessary. Normally
the network settings should be edited in ``/env/network/eth0``, then the network interface
can be brought up using the :ref:`command_ifup` command.

Network filesystems
-------------------

barebox supports NFS and TFTP as filesystem implementations. See :ref:`filesystems_nfs`
and :ref:`filesystems_tftp` for more information. After the network device has been
brought up a network filesystem can be mounted with:

.. code-block:: sh

  mount -t tftp 192.168.2.1 /mnt

or

.. code-block:: sh

  mount -t nfs 192.168.2.1:/export none /mnt

**NOTE:** this can often be hidden behind the :ref:`command_automount` command to make
mounting transparent to the user.

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

This will send UDP packets to 192.168.23.2 on port 6666. On 192.168.23.2 the
scripts/netconsole script can be used to control barebox:

.. code-block:: sh

  scripts/netconsole <board IP> 6666

The netconsole can be used just like any other console.
