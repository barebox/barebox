iBASE MI991AF
=============

The iBASE MI991AF Mini-ITX motherboard has

  * 7th/6th Generation Intel® Xeon® E3 / Core™ i7 / i5 /i3 / Pentium® / Celeron® QC/DC processors, up to 4GHz
  * 2x DDR4 SO-DIMM, Max. 32GB, ECC compatible
  * Intel® Processor integrated graphics device, supports DVI-D, HDMI and DisplayPort
  * Dual Intel® Gigabit LAN
  * 2x USB 2.0, 6x USB 3.0, 4x COM, 4x SATAIII
  * 2x Mini PCI-E slots, 1x mSATA, 1x PCI-E(x16)
  * Watchdog timer, Digital I/O, iAMT (11.0), TPM (1.2), iSMART

Running barebox
---------------

Building the barebox image for this target is covered by the ``efi_defconfig``

BIOS should be configured as follow:

  * When you turn on the computer, the BIOS is immediately activated. Pressing
    the <Del> key immediately allows you to enter the BIOS Setup utility. If you are
    a little bit late pressing the <Del> key, POST (Power On Self Test) will
    continue with its test routines, thus preventing you from invoking the Setup.
    In this case restart the system by pressing the ”Reset” button or simultaneously
    pressing the <Ctrl>, <Alt> and <Delete> keys. You can also restart by turning the
    system Off and back On again.
  * Reset BIOS settings. With this step we wont to make sure BIOS has defined common state to avoid
    undocumented issues. Switch to "Save & Exit" tab, choice "Restore Defaults"
    and press Enter. Answer "Yes" and press Enter again. Then choice "Save Changes and Exit"
    and press Enter.
  * Enable UEFI support. Switch to "Boot" tab. Choice "Boot mode select" and set it to "UEFI".
    Switch in the "Save & Exit" tab to "Save Changes and Exit" and press Enter.

To make network work in barebox you will need to prepare efi binary network drivers and put them in to
"network-drivers" directory.

To continue please proceed with barebox :ref:`barebox_on_uefi` documentation.

Links
-----

  * https://www.ibase.com.tw/english/ProductDetail/EmbeddedComputing/MI991 
