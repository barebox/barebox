TP-LINK MR3020
==============

The TP-LINK MR3020 wireless router has

  * Atheros ar9331 SoC;
  * 32 MiB SDRAM;
  * 4 MiB NOR type SPI Flash Memory;
  * RS232 serial interface (LV-TTL levels on board!);
  * 1 USB interface;
  * 1 Ethernet interfaces;
  * 802.11b/g/n (WiFi) interface;
  * LEDs & buttons.

The router uses U-Boot 1.1.4 as firmware.

Running barebox
---------------

Barebox can be started from U-Boot using tftp.
But you have to encode barebox image in a very special way.

First obtain ``lzma`` and ``mktplinkfw`` utilities.

The ``lzma`` utility can be obtained in Debian/Ubuntu
distro by installing lzma package.

The ``mktplinkfw`` utility can be obtained from openwrt, e.g.::

  $ OWRTPREF=https://raw.githubusercontent.com/mirrors/openwrt/master
  $ curl -OL $OWRTPREF/tools/firmware-utils/src/mktplinkfw.c \
          -OL $OWRTPREF/tools/firmware-utils/src/md5.c \
          -OL $OWRTPREF/tools/firmware-utils/src/md5.h
  $ cc -o mktplinkfw mktplinkfw.c md5.c

To convert your barebox.bin to U-Boot-loadable image (``6F01A8C0.img``)
use this command sequence::

  $ lzma -c -k barebox.bin > barebox.lzma
  $ ./FW/mktplinkfw -c -H 0x07200103 -W 1 -N TL-WR720N-v3 \
      -s -F 4Mlzma -k barebox.lzma -o 6F01A8C0.img

You must setup tftp-server on host 192.168.0.1.
Put your ``6F01A8C0.img`` to tftp-server directory
(usual ``/tftpboot`` or ``/srv/tftp``).

Connect your board to your tftp-server network via Ethernet.

Next, setup network on MR3020 and run ``6F01A8C0.img``, e.g.::

  hornet> set ipaddr 192.168.0.2
  hornet> set serverip 192.168.0.1
  hornet> tftpboot 0x81000000 6F01A8C0.img
  hornet> bootm 0x81000000


Links
-----

  * http://www.tp-link.com/en/products/details/?model=TL-MR3020
  * http://wiki.openwrt.org/toh/tp-link/tl-mr3020
  * https://wikidevi.com/wiki/TP-LINK_TL-MR3020

See also

  * http://www.eeboard.com/wp-content/uploads/downloads/2013/08/AR9331.pdf
  * http://squonk42.github.io/TL-WR703N/
