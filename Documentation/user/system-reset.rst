.. _system_reset:

System Restart
--------------

When running the reset command barebox restarts the SoC somehow. Restart can
be done in software, but a more reliable way is to use a hard reset line, which
really resets the whole machine.
The most common way to force such a hard reset is by using a watchdog. Its
trigger time will be setup as short as possible and after that the software just
waits for its reset. Very simple and most of the time it does what's expected.

But there are some drawbacks within this simple approach.

* most used watchdogs are built-in units in the SoCs. There is nothing wrong
  with that, but these units can mostly reset the CPU core and sometimes a little
  bit more of the SoC. This means this reset is not exactly the same than the
  real POR (e.g. power on reset). In this case you must still handle different
  hardware in a special way because it hasn't seen the reset the CPU has seen.
  Enabled DMA units for example can continue to run and transfer data while the
  CPU core runs through its reset code. This can trigger very strange failures.

* when interacting with flash memories (mostly NOR types and used to store the
  root filesystem) it cannot provide data (sometimes called 'array mode') the
  CPU wants to read after a reset while it is still in some programming mode.
  And if the software is currently changing some data inside the flash and
  an internal reset happens the CPU and the flash memory are doing different
  things and the system hangs until a real POR which also resets the flash
  memory into the 'array mode'.

* some SoC's boot behaviour gets parametrized by so called 'bootstrap pins'.
  These pins can have a different meaning at reset time and at run-time later
  on (multi purpose pins) but their correct values at reset time are very
  important to boot the SoC sucessfully. If external devices are connected to
  these multi purpose pins they can disturb the reset values, and so parametrizing
  the boot behaviour differently and hence crashing the SoC until the next real
  POR happens which also resets the external devices (and keep them away from the
  multi purpose pins).

* when power management comes into play another level of failure is
  possible. To save power the software can lower the clock(s), but to really
  save power, the power supply voltages must be lowered as well. Most PMICs
  (e.g. power management controllers) are dedicated external companion devices,
  loosely connected to their SoC. If the SoC's internal reset source now resets
  the CPU it may increases its clock(s) back to the frequencies after a POR, but
  the external PMIC still provides voltages related to lower frequencies. The
  system isn't consistent any more. If you are in luck, the SoC still works
  somehow, even if the voltages are out of their specifications for the
  currently used clock speeds. But don't rely on it.

To workaround these issues the reset signal triggered by a SoC internal source
must be 'visible' to the external devices to also reset them like a real POR does.
But many SoCs do not provide such a signal. So you can't use the internal reset
source if you face one of the above listed issues!

A different solution is to use the PMIC (if available) to trigger the reset.
Many PMICs provide their own watchdog units and if they trigger a reset they
also switch their voltages back to the real POR values. This will be a system
wide reset, like the POR is.

Drawback of the PMIC solution is, you can't use the SoC's internal mechanisms to
detect the :ref:`reset_reason` anymore. From the SoC point of view it is always
a POR when the PMIC handles the system reset. If you are in luck the PMIC
instead can provide this information if you depend on it.
