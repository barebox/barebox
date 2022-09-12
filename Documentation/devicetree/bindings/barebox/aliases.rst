barebox DT aliases
==================

barebox can use the properties in the ``/aliases`` node to arrive
at deterministic names for devices, e.g.:

.. code-block:: none

   / {
   	aliases {
   		mmc0 = &sdhci;
   		wdog0 = &iwdg;
   	};
   };

will assign the MMC device created from probing the node at ``&sdhci``
the name ``/dev/mmc0``. Similarly, the watchdog device created from
probing the node at ``&iwdg`` will be named ``wdog0``.

By default, barebox will assume the aliases in the DT to align with
the bootsource communicated by the firmware. If this is not the case,
a device tree override is possible via an
``/aliases/barebox,bootsource-${bootsource}${bootsource_instance}``
property:

.. code-block:: none

  &{/aliases} {
	mmc0 = &sdmmc0;
	mmc1 = &sdhci;
	barebox,bootsource-mmc0 = &sdhci;
	barebox,bootsource-mmc1 = &sdmmc0;
  };

This will ensure that when booting from MMC, ``/dev/mmc${bootsource_instance}``
will point at the correct boot device, despite bootrom and board DT alias
order being different.
