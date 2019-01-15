Atmel AT91SAM9261-EK Evaluation Kit
===================================

For AT91SAM9261-EK there are three defconfigs.

The two defconfigs listed below are almost identical.
The one named _first_stage_ can be used for FLASH as it allows the first part to be loaded to SRAM.

.. code-block:: sh

  make ARCH=arm at91sam9261ek_defconfig
  make ARCH=arm at91sam9261ek_first_stage_defconfig

The following defconfig can be used to build a bootstrap variant of barebox

.. code-block:: sh

  make ARCH=arm at91sam9261ek_bootstrap_defconfig
