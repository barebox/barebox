Freescale i.MX23 evaluation kit
===============================

This CPU card is based on an i.MX23 CPU. The card is shipped with:

  * 32 MiB synchronous dynamic RAM (mobile DDR type)
  * ENC28j60 based network (over SPI)

Memory layout when barebox is running:

  * 0x40000000 start of SDRAM
  * 0x40000100 start of kernel's boot parameters

    * below malloc area: stack area
    * below barebox: malloc area

  * 0x41000000 start of barebox

How to get the bootloader binary image
--------------------------------------

Using the default configuration:

.. code-block:: sh

  make ARCH=arm freescale-mx23-evk_defconfig

Build the bootloader binary image:

.. code-block:: sh

  make ARCH=arm CROSS_COMPILE=armv5compiler

**NOTE:** replace the armv5compiler with your ARM v5 cross compiler.
