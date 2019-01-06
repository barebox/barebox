Microchip KSZ 9477 Evaluation board
===================================

This is an evaluation board for a switch that uses the at91sam9x5 CPU.
The board uses Device Tree and supports multi image.

Building barebox:

.. code-block:: sh

  make ARCH=arm microchip_ksz9477_evb_defconfig
