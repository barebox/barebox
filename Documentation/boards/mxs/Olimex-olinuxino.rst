Olimex LTD Olinuxino
====================

The CPU module
--------------

This CPU module is based on a Freescale i.MX23 CPU.

How to get the bootloader binary image
--------------------------------------

Using the default configuration:

.. code-block:: sh

  make ARCH=arm imx233-olinuxino_defconfig

Build the binary image:

.. code-block:: sh

  make ARCH=arm CROSS_COMPILE=armv5compiler

**NOTE:** replace the armv5compiler with your ARM v5 cross compiler.

This produces the following images:

 * barebox-olinuxino-imx23-bootstream.img - Use with the bcb command
 * barebox-olinuxino-imx23-sd.img - Use for SD cards
 * barebox-olinuxino-imx23-2nd.img - Use for 2nd stage booting (with :ref:`command_bootm`)
