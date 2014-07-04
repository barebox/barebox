.. _multi_image:

Multi Image Support
===================

Traditionally a single configuration only works for a single board. Sometimes
even variants of a single board like different amount of memory require a new
config. This has the effect that the number of defconfig files increases dramatically.
All the configs have to be kept in sync manually. Multi Image Support solves this
problem.

With Multi Image Support binaries for multiple boards can be generated from a single
config. A single board can only support compilation with or without Multi Image Support.
Multi Image Support exposes several user visible changes:

* In ``make menuconfig`` it becomes possible to select multiple boards instead of
  only one.
* The ``barebox-flash-image`` link is no longer generated since there is no single
  binary to use anymore.
* images are generated under images/. The build process shows all images generated
  at the end of the build.

Technical background
--------------------

With Multi Image Support enabled, the main barebox binary (barebox.bin) will be
shared across different boards. For board specific code this means that it has
to test whether it actually runs on the board it is designed for. Typically board
specific code has this:

.. code-block:: c

	static int efikamx_init(void)
	{
		if (!of_machine_is_compatible("genesi,imx51-sb"))
			return 0;

		... board specific code ...
	}
	device_initcall(efikamx_init);

Multi Image Support always uses :ref:`PBL <pbl>` to generate compressed images.
A board specific PBL image is prepended to the common barebox binary. The PBL
image contains the devicetree which is passed to the common barebox binary to
let the common binary determine the board type.

The board specific PBL images are generated from a single set of object files
using the linker. The basic trick here is that the PBL objects have multiple
entry points, specified with the ENTRY_POINT macro. For each PBL binary
generated a different entry point is selected using the ``-e`` option to ld.
The linker will throw away all unused entry points and only keep the functions
used by a particular entry point.

The Multi Image PBL files can be disassembled with ``make <entry-function-name.pbl.S``
