.. _booting_openbsd:

Booting OpenBSD
===============

barebox can boot OpenBSD on EFI-capable ARM64 systems.  It acts as an EFI
loader, exposing EFI protocols (block devices, GOP framebuffer, RNG, …) to
subsequently loaded EFI binaries before handing off to the OS.

Basic boot
----------

When ``CONFIG_EFI_LOADER_BOOTMGR`` is enabled, barebox's default boot order
includes the ``efibootmgr`` target, which scans all storage devices for an
ESP partition and boots ``EFI/BOOT/BOOTAA64.EFI`` from it.  If the OpenBSD
installer placed its EFI bootloader on the ESP in the standard location, a
plain ``boot`` invocation should find and start it without any additional
configuration.

To boot from a specific disk explicitly:

.. code-block:: sh

  boot virtioblk0

or to persist:

.. code-block:: sh

  #!/bin/sh
  nv boot.default=virtioblk0

Framebuffer console
-------------------

OpenBSD's kernel discovers its framebuffer console by reading
``/chosen/stdout-path`` from the device tree.  If that property points at a
``simple-framebuffer`` node, the framebuffer is attached as the system console
before the kernel's driver subsystem starts.

barebox sets this up when the ``register_simplefb`` parameter on the
framebuffer device is set to ``stdout-path``:

.. code-block:: sh

  fb0.register_simplefb=stdout-path

The three valid values are:

``disabled``
  No simplefb DT node is created (default).

``enabled``
  A simplefb DT node is created but ``stdout-path`` is not set.

``stdout-path``
  A simplefb DT node is created and ``/chosen/stdout-path`` is pointed at
  it, directing the OpenBSD kernel to use the framebuffer as its boot console.

Systems without a serial console
---------------------------------

On laptops and desktops without an accessible serial port, ``stdout-path``
mode is required, not optional.  Without it the kernel has no console after
``ExitBootServices()`` and the screen stays black.

Enable the framebuffer before booting and persist both settings:

.. code-block:: sh

  nv dev.fb0.enable=1
  nv dev.fb0.register_simplefb=stdout-path
