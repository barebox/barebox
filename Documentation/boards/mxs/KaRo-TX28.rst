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

Using the default configuration::

  make ARCH=arm tx28stk5_defconfig

Build the binary image::

  make ARCH=arm CROSS_COMPILE=armv5compiler

**NOTE:** replace the armv5compiler with your ARM v5 cross compiler.

**NOTE:** to use the result, you also need the following resources from Freescale:

  * the 'bootlets' archive
  * the 'elftosb2' encryption tool
  * in the case you want to start barebox from an attached SD card
    the 'sdimage' tool from Freescale's 'uuc' archive.

Memory layout when barebox is running
-------------------------------------

  * 0x40000000 start of SDRAM
  * 0x40000100 start of kernel's boot parameters

    * below malloc area: stack area
    * below barebox: malloc area

  * 0x47000000 start of barebox
