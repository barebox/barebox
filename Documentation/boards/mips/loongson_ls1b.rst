Loongson LS1B
=============

The LS1B is a development board made by Loongson Technology Corp. Ltd.

The board has

  * Loongson LS1B SoC 250 MHz;
  * 64 MiB SDRAM;
  * 512 KiB SPI boot ROM;
  * 128M x 8 Bit NAND Flash Memory;
  * 2 x RS232 serial interfaces (DB9 connectors);
  * 2 x Ethernet interfaces;
  * 4 x USB interfaces;
  * microSD card slot;
  * LCD display (480x272);
  * audio controller;
  * beeper;
  * buttons;
  * EJTAG 10-pin connector.

The board uses PMON2000 as bootloader.

Running barebox
---------------

  1. Connect to the boards's UART2 (115200 8N1);

  2. Turn board's power on;

  3. Wait ``Press <Enter> to execute loading image`` prompt and press the space key.

  4. Build barebox and upload ``zbarebox.bin`` via Ymodem to the board:

.. code-block:: none

    PMON> ymodem base=0xa0200000

..

  5. Run barebox

.. code-block:: none

    PMON> g -e 0xa0200000

..

Links
-----

  * http://en.wikipedia.org/wiki/Loongson
  * http://www.linux-mips.org/wiki/Loongson
  * https://github.com/loongson-gz
  * http://www.linux-mips.org/wiki/PMON_2000
  * http://www.opsycon.se/PMON2000/Main
