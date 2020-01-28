
.. _optee:

OP-TEE
======

Barebox is able to start the Open Portable Trusted Execution Environment
(OP-TEE) either before starting the linux kernel or during lowlevel board
initialization in the Pre Bootloader ``PBL``.

Before Linux start
------------------
Enable the `CONFIG_BOOTM_OPTEE` configuration variable and configure the
`CONFIG_OPTEE_SIZE` variable. This will reserve a memory area at the end
of memory for OP-TEE to run, usually Barebox would relocate itself there. To
load OP-TEE before the kernel is started, configure the global ``bootm.tee``
variable to point to a valid OPTEE v1 binary.

During the PBL
--------------
To start OP-TEE during the lowlevel initialization of your board in the ``PBL``,
enable the ``CONFIG_PBL_OPTEE`` configuration variable. your board should then
call the function ``start_optee_early(void* tee, void* fdt)`` with a valid tee
and FDT. Ensure that your OP-TEE is compiled with ``CFG_NS_ENTRY_ADDR`` unset,
otherwise OP-TEE will not correctly return to barebox after startup.
Since OP-TEE in the default configuration also modifies the device tree, don't
pass the barebox internal device tree, instead copy it into a different memory
location and pass it to OP-TEE afterwards.
The modified device tree can then be passed to the main barebox start function.
