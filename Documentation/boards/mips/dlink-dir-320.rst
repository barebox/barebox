D-Link DIR-320
==============

The D-Link DIR-320 wireless router has

  * BCM5354 SoC;
  * 32 MiB SDRAM;
  * 4 MiB NOR type Flash Memory;
  * RS232 serial interface (LV-TTL levels on board!);
  * 1xUSB interface;
  * 4 + 1 ethernet interfaces;
  * 802.11b/g (WiFi) interface;
  * JTAG interface;
  * 5 LEDs;
  * 2 buttons.

The router uses CFE as firmware.

Running barebox
---------------

Barebox can be started from CFE using tftp.
You must setup tftp-server on host 192.168.0.1.
Put your barebox.bin to tftp-server directory
(usual /tftpboot or /srv/tftp).
Connect your DIR-320 to your tftp-server network via
one of four <LAN> sockets.

Next, setup network on DIR-320 and run barebox.bin, e.g.:

.. code-block:: console

  CFE> ifconfig eth0 -addr=192.168.0.99
  CFE> boot -tftp -addr=a0800000 -raw 192.168.0.1:barebox.bin


Links
-----

  * http://www.dlink.com.au/products/?pid=768
  * http://wiki.openwrt.org/toh/d-link/dir-320

CFE links:

  * http://www.broadcom.com/support/communications_processors/downloads.php#cfe
  * http://www.linux-mips.org/wiki/CFE
