Debugging with OpenOCD
======================

Barebox can be configured to break on prebootloader and main barebox entry. This
breakpoint can not be resumed and will stop the board to allow the user to
attach a JTAG debugger with OpenOCD. Additionally, barebox provides helper
scripts to load the symbols from the ELF binaries.
The python scripts require `pyelftools`.
To load the scripts into your gdb session, run the following command in the
barebox directory:

.. code-block:: none

    (gdb) source scripts/gdb/helper.py

This makes two new commands available in gdb, `bb-load-symbols` and
`bb-skip-break`. `bb-load-symbols` can load either the main `barebox` file or
one of the .pbl files in the image directories. The board needs to be stopped in
either the prebootloader or main barebox breakpoint, and gdb needs to be
connected to OpenOCD. To continue booting the board, `bb-skip-break` jumps over
the breakpoint and continues the barebox execution.
