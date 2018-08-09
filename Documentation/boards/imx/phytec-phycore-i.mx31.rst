Phytec phyCORE-i.MX31 CPU module PCM-037
========================================

The CPU module
--------------

http://www.phytec.eu/europe/products/modules-overview/phycore/produktdetails/p/phycore-imx31-2.html

This CPU card is based on a Freescale i.MX31 CPU. The card in
configuration -0000REU is shipped with:

  * 128 MiB synchronous dynamic RAM (DDR type)
  * 64 MiB NAND flash
  * 32 MiB NOR flash
  * 512 kiB SRAM
  * 4kiB EEPROM
  * MMU, FPU
  * Serial, Ethernet, USB (OTG), I2C, SPI, MMC/SD/SDIO, PCMCIA/CF, RTC

Supported baseboards
--------------------

Supported baseboards are:
  * Silica / Phytec PCM-970 via phyMAP-i.MX31, PMA-001

How to get barebox for Phytec's phyCORE-i.MX31
----------------------------------------------

Using the default configuration:

.. code-block:: sh

  make ARCH=arm pcm037_defconfig

Build the binary image:

.. code-block:: sh

  make ARCH=arm CROSS_COMPILE=armv5compiler

.. note:: replace ``armv5compiler`` with your ARM v5 cross compiler prefix,
 e.g.: ``arm-1136jfs-linux-gnueabi-``

The resulting binary image to be flashed will be ``barebox.bin``, whereas
the file named just ``barebox`` is an ELF executable for ARM.
