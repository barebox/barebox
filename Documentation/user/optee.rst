
.. _optee:

OP-TEE
======

barebox has support for loading and communicating with the Open Portable Trusted
Execution Environment (OP-TEE).

Loading OP-TEE
--------------

barebox can start OP-TEE either during lowlevel board initialization
in the :ref:`prebootloader <pbl>` or prior to starting the linux kernel.

.. _optee_early_loading:

During the PBL
^^^^^^^^^^^^^^

To start OP-TEE during the lowlevel initialization of your board in the ``PBL``,
enable the ``CONFIG_PBL_OPTEE`` configuration variable. Your board should then
call the function ``start_optee_early(void* tee, void* fdt)`` with a valid tee
and FDT. If you're running on an i.MX6 platform your board code should call
``imx6q_start_optee_early()`` or ``imx6ul_start_optee_early()`` instead since it
validates that the TZASC not bypassed and is configured as expected by OP-TEE.

Ensure that your OP-TEE is compiled with ``CFG_NS_ENTRY_ADDR`` unset, otherwise
OP-TEE will not correctly return to barebox after startup. Since OP-TEE in the
default configuration also modifies the device tree, don't pass the barebox
internal device tree, instead copy it into a different memory location and pass
it to OP-TEE afterwards. The modified device tree can then be passed to the
main barebox start function.

.. note:: Modification of the device tree usually makes it bigger.
  Some spare space must be left after the end of the device tree to
  accommodate this.

Before Linux start
^^^^^^^^^^^^^^^^^^

The support for late loading of OP-TEE had been deprecated and ultimately
removed as it greatly increased the attack surface and was only supported
on 32-bit ARM systems.
OP-TEE loading is now only supported :ref:`in the prebootloader <optee_early_loading>`.


Communication with OP-TEE
-------------------------

Controlled by the ``CONFIG_OPTEE`` option, barebox has support for
communicating with OP-TEE via secure monitor calls and dynamic shared memory.
This is possible independently of whether OP-TEE was loaded by barebox or not.

The primary use cases currently is SCMI-over-OP-TEE, which is required on
the STM32MP13.
