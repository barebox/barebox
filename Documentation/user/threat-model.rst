.. _threat_model:

The barebox threat model
========================

It is not necessarily self-explanatory what barebox does and does not protect
against. This makes it hard to tell security bugs from ordinary ones, and to
know who is responsible for what: barebox, the boot stages before it or the
integrator who configures it.

This document describes what barebox is responsible for, so that
vulnerabilities can be told apart from ordinary bugs. For how to configure
barebox securely, see :ref:`security`.

barebox's responsibilities
--------------------------

barebox is one stage in a boot chain. In a verified boot chain, it must only
start software it has verified, and only use configuration that the previous
stage verified together with barebox itself.

barebox trusts the previous boot stage and everything it verified as part of
the barebox image. This includes the :ref:`prebootloader <pbl>`, barebox
proper, the :ref:`internal devicetree <internal_devicetree>`, the built-in
environment, the compiled-in public keys and security policies, and firmware
like OP-TEE that the prebootloader installs at a higher privilege level. If
any of these is left unverified, barebox can't make up for it.

.. note::

     The previous stage may verify only the prebootloader, as the i.MX8M boot
     ROM does. The prebootloader then checks barebox proper and external
     firmware against hashes built into it before using them to extend the
     chain of trust.

barebox also assumes that the hardware works as documented: the mask ROM
verifies what it's documented to verify, fuses read back as programmed, the
secure world stays isolated from the normal world and peripherals respect
their documented register ranges. barebox doesn't currently defend against
attacks on the hardware itself, like fault injection or side channels.

Everything barebox reads at runtime is untrusted. This includes partition
tables and file systems, boot images and boot entries, devicetree overlays,
firmware and update images, environment or state on storage media and input
from console, network or USB. Of these, barebox can only authenticate FIT
images, HMAC-protected state, signed TLVs and JSON Web Tokens, and only if
it is configured to require it.

The integrator decides which of these inputs barebox uses at all and what it
accepts from them: at build time via Kconfig and at runtime via security
policies. barebox's defaults are made for convenient development, so a
system is only locked down if it's configured that way; :ref:`security`
describes how.

Security policies
^^^^^^^^^^^^^^^^^

A :ref:`security policy <use_security-policies>` is a set of ``SCONFIG_``
options compiled into barebox. Board code or ``CONFIG_SECURITY_POLICY_INIT``
selects one at runtime. The active policy decides, among others, whether
barebox may

* read input from the console (``SCONFIG_CONSOLE_INPUT``) and
  offer an interactive shell (``SCONFIG_SHELL_INTERACTIVE``),

* load an environment from storage media (``SCONFIG_ENVIRONMENT_LOAD``),

* mount file systems other than devfs and ramfs (``SCONFIG_FS_EXTERNAL``),

* boot images without a valid signature (``SCONFIG_BOOT_UNSIGNED_IMAGES``),

* be remote-controlled over RATP (``SCONFIG_RATP``), accept fastboot OEM
  commands (``SCONFIG_FASTBOOT_CMD_OEM``) or act as a USB gadget at all
  (``SCONFIG_USB_GADGET``).

A locked-down system should deny all of these by default and allow them only
after authentication, e.g. with a device-bound unlock token as described in
:ref:`Run-time configuration <runtime_configuration>`. A bug in a denied
feature then can't be reached. Before reporting a bug in one of these
features as a vulnerability, check whether it can still be reached with the
feature denied. A way to get around the policy and use a denied feature is a
vulnerability.

Protections
^^^^^^^^^^^

If the `Assumptions`_ below hold, barebox guarantees the following:

* **Verified hand-over**: barebox only starts or passes on what a trusted key
  has signed: the kernel, initramfs, devicetree, overlays and firmware in a
  FIT image whose configuration signature was verified, with
  :ref:`global.bootm.verify <magicvar_global_bootm_verify>` pinned to
  ``signature``.

* **Verified configuration**: the verified image, not untrusted input,
  decides what barebox boots, what it tells the kernel and whether it
  verifies images. The only exception is where the configuration explicitly
  leaves a choice open, like which of a FIT's signed configurations to boot.

* **Memory safety against untrusted input**: parsing untrusted input necessary
  for booting from storage media or the console doesn't corrupt memory, so an
  attacker can't use it to run their own code.

* **Policy enforcement**: a feature the active security policy denies can't
  be used.

A bug that breaks one of these guarantees is a vulnerability. A bug that can
only be exploited after another guarantee is already broken is just a
weakness. barebox also has hardening measures that keep some classes of bugs
from crossing a security boundary, but a failure of these extra protections
alone is not a vulnerability.

Assumptions
-----------

barebox relies on the integrator to ensure the following. barebox can't
check them, and if one doesn't hold, no barebox configuration can make up
for it.

**barebox is authenticated before it is executed**
   An earlier stage (the mask ROM or another bootloader, like ARM Trusted
   Firmware) verifies the barebox image, and the board is locked down so an
   unverified barebox won't run. Restricting who may update barebox is only
   defence in depth.

**The devicetree is authenticated together with barebox**
   Keys under ``/signature`` in the internal devicetree are trusted just like
   compiled-in keys and can sign any FIT configuration. So whoever controls
   the devicetree controls what barebox accepts. The devicetree must be built
   into the barebox image or come from a stage that verified it, e.g. ARM
   Trusted Firmware loading it from a verified FIP.

**Only the built-in environment configures barebox**
   Global variables decide what is booted, what the kernel is told and,
   unless signature checking is pinned, whether images are verified at all.
   The environment sets these variables, so only the environment built into
   the verified image may be used; see
   :ref:`Disabling the non-builtin environment <disabling_env>`.

**Only authorized users reach the shell**
   The shell is the administrative interface. It can be reached over a
   serial console, the :ref:`network console <network_console>`, the RATP
   protocol used by bbremote, or fastboot, whose ``oem exec`` command is as
   powerful as the shell. Commands that write a partition, set a variable or
   apply an overlay do so without further checks.
   See :ref:`Disabling the shell <disabling_shell>` for more information.

**Files barebox is told to load reside in trusted storage**
   Some data is only referenced by name, not included. For example, a
   devicetree overlay in a signed FIT may have an ``fpga-region`` node with a
   ``firmware-name`` property. barebox loads that file from
   ``global.firmware.path`` and programs it into the FPGA. The signature
   covers the name, not the file, so such paths must point at trusted
   storage, e.g. the built-in environment or a file system protected by other
   means, like dm-verity with a signed root hash.

**barebox is configured securely**
   Beyond the above, barebox must be configured securely at build time via
   Kconfig and at runtime via
   :ref:`security policies <use_security-policies>`, as described in
   :ref:`security`. A bug that only affects a configuration that chapter
   advises against isn't a vulnerability.

What classes of problems are not considered vulnerabilities
-----------------------------------------------------------

The following classes of problems are **not** considered barebox
vulnerabilities. They should still be reported where barebox could do
better, so they can be fixed where reasonably possible, but they are handled
like any regular bug:

* **Configuration**:

  * outdated versions: integrators must keep barebox up to date. A
    vulnerability must be shown to affect the latest release or one of the
    long term stable releases listed in ``SECURITY.md``.

  * build-level: builds that lack the verification they rely on, e.g.
    ``CONFIG_BOOTM_FITIMAGE`` without ``CONFIG_BOOTM_FITIMAGE_SIGNATURE``, or
    with a security policy denying something the build can't enforce. Also
    options documented as lowering security, in particular everything
    ``CONFIG_INSECURE`` enables, and code that only exists for development or
    debugging, like the sandbox architecture's host interfaces, fuzzing
    harnesses or development keys.

  * runtime-level: a security policy that was configured to allow what should
    be denied in a verified boot setup, e.g. ``SCONFIG_BOOT_UNSIGNED_IMAGES``
    or ``SCONFIG_SHELL_INTERACTIVE`` in a lockdown policy, or no policy selected
    at all on a build that then permits everything
    (``CONFIG_SECURITY_POLICY_DEFAULT_PERMISSIVE``). Also
    :ref:`global.bootm.verify <magicvar_global_bootm_verify>` set to anything
    but ``signature``, or not pinned to it.

  * a barebox image, devicetree or environment the previous boot stage
    didn't authenticate; see `Assumptions`_.

  * barebox proper running in the secure world, because OP-TEE isn't
    started from the prebootloader; see :ref:`loading_firmware`.

  * boot entries referencing a FIT:
    :ref:`bootloader spec <bootloader_spec>` entries and
    :ref:`extlinux.conf <extlinux_conf>` files come from a file system
    barebox can't verify. Pointing them at a signed FIT doesn't make the boot
    trustworthy; see
    :ref:`Avoiding use of file systems <avoiding_filesystems>`.

* **Excess of initial privileges**:

  Anything an attacker can do if they already have the access it requires:

  * an image with a signature barebox accepts. Whoever can create one
    already has the signing key, so a bug that can only be reached this way
    doesn't bypass verification.

  * anything done from the shell or via a global variable set from a loaded
    environment.

  * anything that follows from controlling the devicetree, which carries
    the keys.

  * anything done through a feature the active security policy allows, e.g.
    flashing a partition via fastboot on a system whose policy permits
    fastboot OEM commands.

  * a bug in the barebox environment parser:
    The environment can contain scripts, and there is no way to sign an
    environment that isn't built in. An attacker can just add an init
    script instead of attacking the parser.

  * a bug in an unsigned kernel image format:
    Kernel images should be placed in a signed container. An attacker who
    can modify a kernel image without breaking a signature can simply make
    it run their own code once barebox starts it. There is no need to attack
    the kernel header barebox parses.

  * a bug in a feature that the documentation calls
    unsuitable for verified boot and that can be disabled at build time or
    runtime, but wasn't.
    For an example, see
    :ref:`avoiding use of file systems <avoiding_filesystems>`.

* **Selecting among signed configurations**:

  A FIT signature covers the configuration it's in, but not the ``default``
  property that picks a configuration when barebox isn't told which one to
  boot. Booting a different signed configuration of the same FIT is
  therefore not a bypass; see :ref:`pinning_fit_config`.

* **Update images**:

  :ref:`barebox_update <update>` and fastboot's ``flash`` command don't
  authenticate anything. Their checks, e.g. that the image is for the right
  board, only exist to avoid bricking the board. The previous boot stage,
  not barebox, verifies what gets installed.

* **Network boot**:

  The network isn't meant to be used in verified boot setups, and booting
  over it is not a verified boot path. DHCP options, the hostname and files
  fetched via TFTP or NFS are as untrusted as any other input and the barebox
  network stack implementation is not hardened against adversaries.

* **Denial of service by way of a boot image**:

  barebox tries to reject malformed images, but an image that makes barebox
  hang, panic or refuse to boot isn't a vulnerability: whoever can supply
  such an image can just as well prevent booting by erasing it. Memory
  corruption is still in scope, as it can let an attacker run their own
  code.

* **Hardening failures**:

  * a missing bounds or argument check whose only effect is that a
    malformed image is rejected later than it could have been.

  * bypassing a defence in depth measure without showing how to exploit it
    further.

* **Random information leaks**:

  Small amounts of memory the attacker can't choose reaching the console,
  e.g. through unterminated strings, structure padding or printed memory
  addresses.

* **Physical access**:

  A verified boot chain is meant to hold up even when the attacker has the
  device in hand. So everything barebox reads over its external interfaces
  (storage media, USB and the serial console) is in scope, no matter how the
  attacker got access. Attacks that bypass or modify the hardware are out of
  scope: fault injection, debug ports like JTAG that the integrator should have
  fused off, or replacing the storage barebox itself is loaded from, which the
  previous boot stage must detect.

Report vulnerabilities to security@barebox.org as described in
``SECURITY.md``. Everything else is welcome on the mailing list; see
:ref:`feedback`.
