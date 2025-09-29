.. _develop_security-policies:

Security Policies (Developer Manual)
====================================

Overview
--------

This document describes how to define new SConfig symbols and integrate them
into Barebox security policies. SConfig uses the same backend as Kconfig, and
its configuration files live alongside Kconfig files (e.g. ``common/Sconfig``).

Key principles:

- Except for the name, symbols are always ``bool``.
- Policies are board-specific and described in ``.sconfig`` files at build-time.
- Every policy is complete and no implicit defaults are applied by mere building
- Policy ``.sconfig`` files are post-processed into ``.sconfig.c`` files and
  then compiled and linked into the final barebox binary.

Creating New Symbols
--------------------

1. **Add a new symbol** to the appropriate ``Sconfig`` file, such as ``common/Sconfig``:

   .. code-block:: kconfig

      config ENV_HANDLING
          bool "Allow persisting and loading the environment from storage"
          depends on $(kconfig-enabled ENV_HANDLING)

2. **Reference it in code** using:

   .. code-block:: c

      #include <security/config.h>

      if (!IS_ALLOWED(SCONFIG_ENV_HANDLING))
          return -EPERM;

3. **Update policies**:

   Every existing ``.sconfig`` policy must define a value for the new symbol
   as there are no implicit defaults to ensure every policy explicitly encodes
   all options in accordance with its security requirements.

   Example in ``myboard-lockdown.sconfig``:

   .. code-block:: none

      SCONFIG_ENV_HANDLING=n

   And in ``myboard-devel.sconfig``:

   .. code-block:: none

      SCONFIG_ENV_HANDLING=y

Linking Policy Files
--------------------

Policies can be added to the build using ``policy-y`` in the board’s
Makefile:

.. code-block:: make

   policy-y += myboard-lockdown.sconfig

As policies are enforced to be complete, they may require resynchronization
(e.g., with ``make olddefconfig``) if the config changes. A build failure
will alert the user to this fact.

``virt32_secure_defconfig`` is maintained as reference configuration for
trying out security policies and that it's buildable is ensured by CI.

Tips for Symbol Design
----------------------

- Avoid naming symbols after board names. Favor functionality.
- Prefer giving Sconfig symbols the same name as Kconfig symbols, when they
  address the same goal, but at runtime instead of build-time.
- When possible, reuse logic in core code by wrapping around
  ``IS_ALLOWED()`` checks.

Validation & Maintenance
------------------------

Always run ``make security_olddconfig`` for the security policy reference
configuration ``virt32_policy_defconfig``::

  export ARCH=arm
  export CROSS_COMPILE=...
  make virt32_policy_defconfig
  make security_olddefconfig

CI also checks this configuration and verifies that it's up-to-date.
