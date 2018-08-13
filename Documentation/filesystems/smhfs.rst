.. index:: smhfs (filesystem)

.. _filesystems_smhfs:

File I/O over ARM semihosting support
=====================================

Target Side Setup
-----------------

barebox can communicate with debug programms attached via SWD/JTAG by
means of ARM semihosting protocol.

Not all of the I/O primitives neccessary to implement a full
filesystem are exposed in ARM semihosting API and because of that some
aspects of filesystem funcionality are missing. Implementation does
not have support for listing directories. This means a
:ref:`command_ls` to a SMHFS-mounted path will show an empty
directory. Nevertheless, the files are there.

Example:

.. code-block:: console

  barebox:/ mount -t smhfs /dev/null /mnt/smhfs


Host Side Setup
---------------

FIXME: Currently OpenOCD does not work correctly if Barebox is built
with MMU enabled, so before using this featrue, please make sure that
MMU is disabled in your particular configuration

To make semihosting work host machine connected to the target via
JTAG/SWD must have semihosting capable debug software running. One
such tool would be OpenOCD. For ARM9 and ARM11 CPUs most recent
release of OpenOCD should suffice, however for ARMv7A based devices
patched version from here http://openocd.zylin.com/#/c/2908/ has to be
used.

The following steps are required to set up a operational semihosting
channel:

      1. In a terminal start OpenOCD and specify your particular board
         and debug adapter used.

      2. In a separate terminal connect to OpenOCD via telnet

	   telnet localhost 4444

      3. In resulting telnet session execute the following commands:

           halt
	   arm semihosting on
	   resume

After that is done all of the semihosting related functions should be
ready to use.
