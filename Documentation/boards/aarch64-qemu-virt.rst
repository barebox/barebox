Aarch64
=======

Aarch64 Qemu virt
-----------------

Running barebox on QEMU aarch64 virt machine
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Usage::
	$ qemu-system-aarch64 -m 2048M \
		-cpu cortex-a57 -machine virt \
		-display none -serial stdio \
		-kernel ../barebox/barebox
