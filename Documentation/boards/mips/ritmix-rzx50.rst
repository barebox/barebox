Ritmix RZX-50
=============

Ritmix RZX-50 is a portable game console for the Russian market.

The portable game console has

  * Ingenic JZ4755 SoC;
  * 64 MiB SDRAM;
  * 4 GiB microSDHC card / 4 GiB NAND type Flash Memory (internal boot device);
  * RS232 serial interface (LV-TTL levels on the board!);
  * LCD display (480x272);
  * Video out interface;
  * 1xUSB interface;
  * buttons.

The game console uses U-Boot 1.1.6 as bootloader.

Running barebox
---------------

  1. Connect to the game console's UART
     (see. http://a320.emulate.su/2012/01/19/uart-na-ritmix-rzx-50/);

  2. Unblock U-Boot console
     (see. http://a320.emulate.su/2012/01/25/rzx-50-dostup-k-konsoli-u-boot/);
     Please note that U-Boot's Zmodem support does not work;

  3. Boot Ritmix linux and login;

  4. Build barebox and upload ``barebox.bin`` via Zmodem to the board:

.. code-block:: none

    # cd /tmp
    # rz

..

  5. Write barebox to onboard flash:

.. code-block:: none

    # dd if=barebox.bin of=/dev/mmcblk0 seek=1048576 bs=1 count=262144

..

  6. Reboot RZX-50, next in U-Boot console start barebox:

.. code-block:: none

  CETUS # msc read 0xa0800000 0x100000 0x40000; g a0800000

..


Links
-----

  * http://www.ritmixrussia.ru/products/252/entertainment/game/rzx-50
  * ftp://ftp.ingenic.cn/2soc/4755/JZ4755_ds.pdf
  * ftp://ftp.ingenic.cn/3sw/01linux/01loader/u-boot/u-boot-1.1.6-jz-20110719-r1728-add-jz4770.patch.bz2
