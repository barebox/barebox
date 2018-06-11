Garz+Fricke Vincell LT
======================

This CPU card is based on a Freescale i.MX53 CPU. The card is shipped with:

  * 512MiB NAND flash
  * 512MiB synchronous dynamic RAM
  * microSD slot

See http://www.garz-fricke.com/en/products/embedded-systems/single-board-computer/ia-0086r/ for more information.


Bootstrapping barebox
---------------------

The Vincell is shipped with the RedBoot bootloader. To replace RedBoot with
barebox, you first need to connect a serial console to the device.

UART1 is located on the Molex header X39. When the Vincell is lying on the
display, then Pin 1 on the header is in the lower right:

  * Pin 1 - GND
  * Pin 2 - TXD1
  * Pin 3 - RXD1

If everything is connected right, RedBoot shows up on the console.

factory_bootstrap
-----------------

Turn on the board and abort the boot process with ``Ctrl-C``.
Configure RedBoot so that it finds the barebox image on your TFTP server.
Execute ``fc`` and change the network setup according to your environment.
Be sure to copy the `Boot script` line to the prompt, otherwise it will be
set empty. The network setup can be validated with ``ping -h <serverip>``.
If the network setup is working properly, barebox can be loaded into memory::

  load -v -r -b 0x80100000 barebox-guf-vincell-lt.img
  exec

Once in barebox, the bootloader can now be persisted to NAND::

  barebox_update -t nand /mnt/tftp/barebox-guf-vincell-lt.img``
