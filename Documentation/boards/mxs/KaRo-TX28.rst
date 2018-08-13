KARO TX28 CPU module
====================

The CPU module
--------------

http://www.karo-electronics.de/

This CPU card is based on a Freescale i.MX28 CPU. The card is shipped with:

  * 128 MiB synchronous dynamic RAM (DDR2 type), 200 MHz support
  * 128 MiB NAND K9F1G08U0A (3.3V type)
  * PCA9554 GPIO expander
  * DS1339 RTC
  * LAN8710 PHY

Supported baseboards
--------------------

Supported baseboards are:

  * KARO's Starterkit 5

How to get barebox for 'KARO's Starterkit 5'
--------------------------------------------

Using the default configuration:

.. code-block:: sh

  make ARCH=arm tx28stk5_defconfig

Build the binary image:

.. code-block:: sh

  make ARCH=arm CROSS_COMPILE=armv5compiler

**NOTE:** replace the armv5compiler with your ARM v5 cross compiler.

This produces the following images:

 * barebox-karo-tx28-bootstream.img - Use with the bcb command
 * barebox-karo-tx28-sd.img - Use for SD cards
 * barebox-karo-tx28-2nd.img - Use for 2nd stage booting (with :ref:`command_bootm`)

