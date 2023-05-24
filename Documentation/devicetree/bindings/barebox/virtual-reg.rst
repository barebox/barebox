virtual-reg property
====================

The ``virtual-reg`` property provides a hint on the 32-bit virtual
address mapping the first physical base address in the ``reg`` property.
This is meant to allow the OS to use the boot firmware's virtual memory
mapping to access device resources early in the kernel boot process.

When barebox is compiled with ``CONFIG_MMU`` support and the
implementation supports remapping, devices with ``virtual_reg`` will have
all their resources remapped at the physical/virtual address offset calculated
by subtracting ``virtual-reg`` from the first address in ``reg``.

This is normally used to map I/O memory away from the zero page, so it
can be used again to trap null pointer dereferences, while allowing
full access to the device memory::

.. code-block:: none

   &{/soc} {
   	#address-cells = <1>;
   	#size-cells = <1>;

        flash@0 {
             reg = <0 0x10000>;
             virtual-reg = <0x1000>;
	     /* => memory region remapped from [0x1000, 0x11000] to [0x0000, 0x10000] */
        };
  };
