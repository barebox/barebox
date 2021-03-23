Aarch64 Qemu virt
=================

Besides a number of physical ARM64 targets, barebox also supports a
``qemu-virt64`` board.

Running barebox on QEMU aarch64 virt machine
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Usage::

	$ qemu-system-aarch64 -m 2048M \
		-cpu cortex-a57 -machine virt \
		-display none -serial stdio \
		-kernel ./images/barebox-dt-2nd.img
