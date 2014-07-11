TI Davinci
==========

virt2real
---------

virt2real is a miniature board for creation of WiFi
or Internet controllable smart devices.

The board has

  * TI DaVinchi DM365 running at 300 MHz;
  * 128 MiB DDR2 SDRAM;
  * 256 MiB NAND Flash Memory;
  * 2 x UART serial interfaces;
  * 1 x Ethernet interface (Micrel KS8851);
  * 1 x USB interface;
  * microSD card slot.

The board uses U-Boot as bootloader.


Running barebox
^^^^^^^^^^^^^^^

  1. Connect to the boards's UART0 (115200 8N1);
     Use J2.2 (GND), J2.4 (UART0_TXD), J2.6 (UART0_RXD) pins.

  2. Turn board's power on;

  3. Wait for ``Hit any key to stop autoboot`` prompt and press the space key.

  4. Upload ``barebox.bin`` via Ymodem

.. code-block:: none
    virt2real ># loady
..

  5. Run barebox

.. code-block:: none
    virt2real ># go 0x82000000
..


Links
^^^^^

  * http://virt2real.com/
  * http://wiki.virt2real.ru/
  * https://github.com/virt2real
