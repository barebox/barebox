chumbyone Chumby Industrie's Falconwing
=======================================

This device is also known as "chumby one" (http://www.chumby.com/)

This CPU card is based on a Freescale i.MX23 CPU. The card is shipped with:

  * 64 MiB synchronous dynamic RAM (DDR type)

Memory layout when barebox is running:

  * 0x40000000 start of SDRAM
  * 0x40000100 start of kernel's boot parameters

    * below malloc area: stack area
    * below barebox: malloc area

  * 0x42000000 start of barebox

How to get the bootloader binary image
--------------------------------------

Using the default configuration:

.. code-block:: sh

  make ARCH=arm chumbyone_defconfig

Build the bootloader binary image:

.. code-block:: sh

  make ARCH=arm CROSS_COMPILE=armv5compiler

**NOTE:** replace the armv5compiler with your ARM v5 cross compiler.

How to prepare an MCI card to boot the "chumby one" with barebox
----------------------------------------------------------------

  * Create four primary partitions on the MCI card

    * the first one for the bootlets (about 256 kiB)
    * the second one for the persistant environment (size is up to you, at least 256k)
    * the third one for the kernel (2 MiB ... 4 MiB in size)
    * the fourth one for the root filesystem which can fill the rest of the available space

  * Mark the first partition with the partition ID "53" and copy the
    bootlets into this partition (currently not part of barebox!).

  * Copy the default barebox environment into the second partition
    (no filesystem required).

  * Copy the kernel into the third partition (no filesystem required).

  * Create the root filesystem in the fourth partition. You may copy an
    image into this partition or you can do it in the classic way:
    mkfs on it, mount it and copy all required data and programs into
    it.
