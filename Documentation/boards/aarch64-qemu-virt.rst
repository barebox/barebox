Aarch64 Qemu virt
=================

Besides a number of physical ARM64 targets, barebox also supports the
``qemu-virt64`` board in the generic ``multi_v8_defconfig``::

 make multi_v8_defconfig
 make

Running barebox on QEMU aarch64 virt machine
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Usage::

	$ qemu-system-aarch64 -m 2048M \
		-cpu cortex-a57 -machine virt \
		-display none -serial stdio \
		-kernel ./images/barebox-dt-2nd.img
