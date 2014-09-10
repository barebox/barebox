MIPS Creator CI20
=================

The MIPS Creator CI20 board is a high performance, fully featured
Linux and Android development platform.

Major hardware features include:

  * Ingenic JZ4780 SoC (dual 1.2GHz MIPS32 processor);
  * 1 GiB DDR3 memory;
  * 8 GiB NAND Flash;
  * 2 x UART;
  * Davicom dm9000 10/100 Ethernet controller;
  * 2 x USB (host and OTG);
  * HDMI output;
  * 14-pin ETAG connector;
  * GPIO, SPI, I2C, ADC, expansion headers.


The board uses U-Boot 2013.10 as bootloader.


Running barebox
---------------

  1. Boot the board with UART0 serial console. Stop the auto boot during U-boot.

  2. Upload ``zbarebox.bin`` via Ymodem to the board and then run:

.. code-block:: none

    ci20# loady 0xa8000000
    ...
    ci20# go 0xa8000000
..


Links
-----

  * http://store.imgtec.com/product/mips-creator-ci20
  * http://elinux.org/MIPS_Creator_CI20
