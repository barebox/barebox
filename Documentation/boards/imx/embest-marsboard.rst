Embest MarS Board
=================

Board comes with:

* 1G DDR3 SDRAM
* 4G eMMC
* 2M SPI-NOR Flash

Layout::

  .-----------------------------------------------------.
  |O                                    OTG-->| V |    O|
  |                              SW1    USB   '---'  .--|
  |             .-----------.    v.---.           .->| <|
  |             |           |    1|o--| O         |  `--|
  |             | i.MX6Dual |    2|--o| N        Debug  |
  |             |    SoC    |     `---'          USB    |
  |             |           |                           |
  |             `-----------'                           |
  |                                                     |
  |                                                     |
  |                                                     |
  |                                                     |
  |O                                                   O|
  `-----------------------------------------------------'

Boot Configuration
==================

DIP Switch ``SW1`` on the board can be used to set ``BOOT_MODE1`` and
``BOOT_MODE0`` going to the i.MX6:

Set ``SW1 = 01`` for serial boot::

        SW1
        v.---.
        1|o--| O
        2|--o| N
         `---'

Set ``SW1 = 10`` for internal (SPI-NOR Flash) boot::

        SW1
        v.---.
        1|--o| O
        2|o--| N
         `---'

Set ``SW1 = 00`` for boot from eFuses::

        SW1
        v.---.
        1|o--| O
        2|o--| N
         `---'

Flashing barebox
----------------

  1. Connect to the board's Debug Mini-USB (115200 8N1)

  2. Set ``SW1 = 01`` for serial boot mode (see above)

  3. Turn board's power on

  4. Upload barebox image to the board via imx-usb-loader

.. code-block:: none

  host$ imx-usb-loader images/barebox-embest-imx6q-marsboard.img
..

  4. Flash barebox to SPI-NOR Flash via Android Fastboot

.. code-block:: none

  host$ fastboot flash bbu-spiflash images/barebox-embest-imx6q-marsboard.img

..

  5. Restore ``SW1 = 10`` for internal (SPI-NOR) boot (see above)
