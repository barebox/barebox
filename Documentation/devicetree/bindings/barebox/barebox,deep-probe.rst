barebox deep probe properties
=============================

A ``barebox,deep-probe`` property in the top-level node indicates to barebox
that the barebox board and device drivers support recursively probing devices
on demand (deep probe).

For drivers to support deep probe, they must not rely on initcall ordering.
Resources needed by drivers should be referenced via device tree, e.g.,
instead of direct use of hardcoded GPIO numbers, GPIOs must either be:

* described in the device's device tree node and requested using API that
  that takes the device or the device tree node as argument

* probe of the GPIO controller be ensured via ``of_device_ensure_probed``,
  e.g., ``of_devices_ensure_probed_by_property("gpio-controller");``

The inverted flag is ``barebox,disable-deep-probe``, which means that the
board is not deep probe capable (or tested) yet.

The ``barebox,disable-deep-probe`` property takes precedence over
``barebox,deep-probe``, but not over ``BAREBOX_DEEP_PROBE_ENABLE``
in the board code.

If neither property exists, the default deep probe behavior depends on
the ``CONFIG_DEEP_PROBE_DEFAULT`` variable.

.. code-block:: none

   / { /* SoM Device Tree */
   	barebox,deep-probe;
   };

   / { /* Board Device Tree */
        /* FIXME: While the SoM supports deep probe, our board code is broken
	 * currently, so override until it's fixed
	 */
   	barebox,disable-deep-probe;
   };
