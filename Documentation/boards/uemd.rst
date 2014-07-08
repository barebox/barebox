UEMD
====

MB 77.07
--------

MB 77.07 board uses MBOOT as bootloader.
MBOOT is a U-Boot clone with reduced functionality.
See https://github.com/RC-MODULE/mboot for details.

Running barebox
^^^^^^^^^^^^^^^

  1. Connect to the boards's UART (38400 8N1);

  2. Turn board's power on;

  3. Wait ``Hit any key (in 2 sec) to skip autoload...`` prompt and press the space key;

  4. Compile ``zbarebox.bin`` image and upload it to the board via tftp

.. code-block:: none

  MBOOT # tftpboot zbarebox.bin
  greth: greth_halt
  TFTP Using GRETH_10/100 device
  TFTP params: server 192.168.0.1 our_ip 192.168.0.7
  TFTP params: filename 'zbarebox.bin' load_address 0x40100000
  TFTP Loading: ################
  TFTP done

..

  5. Run barebox

.. code-block:: none

  MBOOT # go 0x40100000

..
