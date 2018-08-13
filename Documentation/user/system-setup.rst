Appendix: System Setup
======================

Serial Console Access
---------------------

As most current development machines don't have serial ports, the usual setup
is to use a USB-Serial-Converter. Some evaluation boards have such a converter
on board. After connecting, these usually show up on your host as
``/dev/ttyUSB#`` or ``/dev/ttyACM#`` (check ``dmesg`` to find out).

On Debian systems, the device node will be accessible to the ``dialout`` group,
so adding your user to that group (``adduser <user> dialout``) removes the need
for root privileges.

Using the "screen" program
^^^^^^^^^^^^^^^^^^^^^^^^^^

The terminal manager ``screen`` can also be used as a simple terminal emulator:

.. code-block:: sh

  screen /dev/ttyUSB0 115200

To exit from ``screen``, press ``<CTRL-A> <K> <y>``.

Using the "microcom" program
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

A good alternative terminal program is microcom. On Debian it can be installed
with ``apt-get install microcom``, on other distributions it can be installed
from source:

https://git.pengutronix.de/cgit/tools/microcom

Usage is simple:

.. code-block:: sh

  microcom -p /dev/ttyUSB0

Network Access
--------------

Having network connectivity between your host and your target will save you a
lot of time otherwise spent on writing SD cards or using JTAG. The main
protocols used with barebox are DHCP, TFTP and NFS.

Configuration of dnsmasq for DHCP and TFTP
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``dnsmasq`` program can be configured as a DHCP and TFTP server in addition
to its original DNS functionality:

.. code-block:: sh

  sudo ip addr add 192.168.23.1/24 dev <interface>
  sudo ip link set <interface> up
  sudo /usr/sbin/dnsmasq --interface=<interface> --no-daemon --log-queries \
    --enable-tftp --tftp-root=<absolute-path-to-your-images>/ \
    --dhcp-range=192.168.23.240,192.168.23.250

Configuration of a TFTP Server
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Configuration of a BOOTP / DHCP Server
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Configuring a NFS Server
^^^^^^^^^^^^^^^^^^^^^^^^
