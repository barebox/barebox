.. _use_security-policies:

Security Policies (User Manual)
===============================

Overview
--------

Barebox supports structured security configuration through **security policies**,
a runtime configuration mechanism that allows switching between multiple
predefined security policies (e.g. ``lockdown``, ``devel``),
depending on operational requirements.

This replaces ad-hoc board code with a clean, reproducible, and
auditable configuration-based model.

Concepts
--------

- **SConfig**: A configuration system using the same backend as
  Kconfig, designed for **runtime security policies**.
- **Policies**: Named configurations like ``myboard-lockdown.sconfig``,
  ``myboard-open.sconfig`` specific to each board.

Usage
-----

1. **Configure a policy** using menuconfig (or another frontend):

   .. code-block:: shell

      make KPOLICY=arch/arm/boards/myboard/myboard-lockdown.sconfig security_oldconfig
      make security_menuconfig # Iterates over all policies

2. **Configuration files** (e.g. ``myboard-lockdown.sconfig``) are in Kconfig
   format with ``SCONFIG_``-prefixed entries.

3. **Build integration**:

   The sconfig files for the board are placed into the ``security/``
   directory in the source tree and their relative file names
   (i.e., with the ``security/`` prefix) are added to
   ``CONFIG_SECURITY_POLICY_PATH``.

   Alternatively, policies can also be be referenced in a board's
   Makefile:

   .. code-block:: make

      lwl-y       += lowlevel.o
      obj-y       += board.o
      policy-y    += myboard-lockdown.sconfig myboard-devel.sconfig

   This latter method can be useful when building multiple boards in
   the same build, but with different security policies.

4. **Registration**:

   Policies added with ``CONFIG_SECURITY_POLICY_PATH`` are automatically
   registered for all enabled boards.

   Policies added with policy-y need to be explicitly added by symbol
   to the set of registered policies in board code:

   .. code-block:: c

     #include <security/policy.h>

     security_policy_add(myboard_lockdown)
     security_policy_add(myboard_devel)

5. **Runtime selection**:

   In board code, switch to a policy by name:

   .. code-block:: c

     #include <security/policy.h>

     security_policy_select("lockdown");

Limitations
-----------

Always start with the most restrictive policy and switch to more permissive policies later
when needed. Forbidding previously allowed options might have undesired side effects which
include:

- Forbidding mounting of file systems does not affect already mounted file systems
- Forbidding shell does not affect the already running instance

Trying it out
-------------

``virt32_secure_defconfig`` is the current reference platform for security
policy development and evaluation. ``images/barebox-dt-2nd.img`` that results
from building it can be passed as argument to ``qemu-system-arm -M virt -kernel``.

The easiest way to do this is probably installing labgrid and running
``pytest --interactive`` after having built the config.

Differences from Kconfig
------------------------

+-------------------------+------------------------------+-----------------------------+
| Feature                 | Kconfig                      | SConfig                     |
+=========================+==============================+=============================+
| Purpose                 | Build-time configuration     | Runtime security policy     |
+-------------------------+------------------------------+-----------------------------+
| File name               | .config                      | ${policy}.sconfig           |
+-------------------------+------------------------------+-----------------------------+
| policies per build      | One                          | Multiple                    |
+-------------------------+------------------------------+-----------------------------+
| Symbol types            | bool, int, string, ... etc.  | bool only                   |
+-------------------------+------------------------------+-----------------------------+
| Symbol dependencies     | Kconfig symbols              | Both Kconfig and Sconfig    |
|                         |                              | symbols                     |
+-------------------------+------------------------------+-----------------------------+

Best Practices
--------------

- Maintain all ``.sconfig`` files under version control,
  either as part of the barebox patch stack or in your BSP

- Document reasoning when changing every single security option
  (even when doing ``security_olddefconfig``).

- Avoid logic duplication in board code — rely on SConfig conditionals.

- Name policies meaningfully: e.g. ``lockdown``, ``tamper``, ``return``.
