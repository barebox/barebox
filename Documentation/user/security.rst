.. _security:

Security Considerations
=======================

As a bootloader, barebox is often used as part of a cryptographically verified
boot chain. Such a boot chain is only as secure as its weakest link and
special care needs to be taken while configuring and deploying barebox.

Verified Boot
-------------

In a cryptographically verified boot chain (henceforth termed "Verified Boot"),
each boot stage must be verified by the previous boot stage before execution.

At the start of the verification chain lies some hardware root of trust, most
often a public key (or its hash) that's programmed into one-time-programmable
(OTP) eFuses while the board is still in the factory.

The SoC's mask ROM (sometimes called BootROM) will consult the eFuses and
use them to verify the first stage bootloader image. From there on, it's
expected that every boot stage will only boot the next one after verification.

Verification can take many forms:

- The mask ROM provides an API to verify later images against the same key
  used to verify the first boot stage.

- If both first stage and second stage result from the same build, the first
  stage can embed the hash of the second stage.

- The boot stage has an embedded public key which is used to verify the
  signature of the later boot stage.

Ensuring barebox is verified
----------------------------

The way that barebox is verified is highly SoC-specific as it's usually done
by the SoC mask ROM and in some cases by a different first stage bootloader
like ARM Trusted Firmware.

For some SoCs, like i.MX :ref:`High Assurance Boot <hab>`, the barebox
build system has built-in support for invoking the necessary external tools
to sign the boot images.  In the general case however, the signing happens
outside the barebox build system and the integrator needs to ensure that
the images are signed with the correct keys.

In any case, each individual board must be locked down, i.e., configured to
only boot correctly signed images.

The latter is often done by writing a different set of eFuses, see for
example the barebox :ref:`hab command <command_hab>` which does the necessary
fusing for both HABv4 and AHAB.

.. warning:: barebox commands like :ref:`hab command <command_hab>` only
   touch the subset of fuses relevant to most users. It's up to the integrators
   to fuse away unneeded functionality like USB recovery or JTAG as needed.

Any verified boot setup that doesn't ensure that barebox was correctly signed
before execution is thus fundamentally flawed. A corollary to this is that
it's not enough to restrict the ways that barebox can be updated: An attacker
can often overwrite barebox without its knowledge, via physical access or
after having booted into the OS. barebox's signature being validated by the
previous boot stage is thus paramount.

Specifically, the checks :ref:`barebox update <update>` performs on an image,
e.g. that it targets the right board, exist to reduce the risk of bricking
the board and can be skipped with ``-f``. They are not a security measure.

Ensuring the barebox devicetree is verified
-------------------------------------------

Whoever controls the devicetree barebox uses for itself can inject RSA keys
to be used for FIT verification and control what drivers are probed.

The devicetree must therefore be either:

- verified by the previous boot stage before passing it to barebox

- part of a barebox image and thus verified by the previous boot stage

``barebox-dt-2nd.img`` is bootable like a Linux kernel and takes its
devicetree from the previous stage. It's thus only suitable for verified
boot if that stage verified the devicetree as well, e.g. because both are
part of the same signed FIP image.

Loading firmware
----------------

In systems utilizing the ARM TrustZone, barebox is often tasked with loading
the secure OS (usually OP-TEE). After OP-TEE is loaded, the rest of the
software runs in a less-privileged non-secure or "normal" world.

The installation of OP-TEE (and any higher privileged firmware like ARM Trusted
Firmware) should happen as early as possible, i.e., within the barebox
:ref:`prebootloader <pbl>`. Delaying installation of OP-TEE means that most of
barebox will run with elevated permission, which greatly increases the attack
surface.

Pinning the FIT configuration
-----------------------------

A FIT signature covers the configuration node it is placed in, the images
that configuration refers to and their hashes. It does not cover the
``default`` property of the top-level ``/configurations`` node, which is
what barebox falls back to when no configuration matches the board compatible.

An attacker can therefore select which of the signed configurations of a FIT is
booted without altering any image. If that matters, ship only one configuration
per FIT, or name the configuration explicitly, e.g.
``bootm /dev/mmc0.kernel@conf-production``.

Ensuring the kernel is verified
-------------------------------

barebox can embed one or more RSA or ECDSA public keys that it will use to
verify signed FIT images. In a verified boot system, barebox should not
be allowed to boot any images that have not been signed by the correct key.
This can be enforced by setting ``CONFIG_BOOTM_FORCE_SIGNED_IMAGES=y``
and disabling any ways that could be used to override this.

How thoroughly an image is checked is controlled by
:ref:`global.bootm.verify <magicvar_global_bootm_verify>`. Only ``signature``
is suitable for verified boot. ``hash`` checks the image hashes, but not the
configuration signature, so it detects corruption, not tampering. The default
``available`` verifies whatever the image carries and accepts an image
carrying nothing.

As a global variable, the setting may be changeable at runtime unless barebox
is built with ``CONFIG_BOOTM_FORCE_SIGNED_IMAGES=y`` or the
:ref:`security policy <use_security-policies>` denies
``SCONFIG_BOOT_UNSIGNED_IMAGES``. Either pins it to ``signature``. This
will also result in :ref:`command_bootm` refusing to boot any non-FIT images.

For development convenience ``CONFIG_CRYPTO_BUILTIN_DEVELOPMENT_KEYS``
can be enabled after enabling ``CONFIG_INSECURE`` to compile well known
development keys into the barebox binary.
The private keys for these keys can be found
`[here] <https://github.com/pengutronix/ptx-code-signing-dev>`__.

Disabling the shell
^^^^^^^^^^^^^^^^^^^

While useful for development, the barebox shell can be used in creative
ways to circumvent boot restrictions. It's thus advisable to disable
the shell completely (``CONFIG_SHELL_NONE=y``) or make it non-interactive
(``CONFIG_CONSOLE_DISABLE_INPUT=y``). This may be coupled with muxing UART RX
pin as GPIO for maximum effectiveness.

In addition, there are alternative methods of accessing the shell like
netconsole, or fastboot. These should preferably be disabled or at least
not activated by default.

barebox places no restrictions on what the shell does: a command that writes
a partition, sets a variable or applies a devicetree overlay does exactly
that. Whoever reaches the shell is as trusted as the boot chain, so any
remaining way of reaching it is part of the boot chain. A console kept for
diagnostics should be output-only.

Disabling the non-builtin environment
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Anything done interactively by the shell can also be done automatically by
means of init scripts in the environment. Even without shell support, the
non-volatile variables in the environment could be used to reconfigure
barebox in an insecure manner or to influence the command line and device
tree passed to the kernel on boot.

A secure barebox should thus only consult the environment that it has built
in and not parse an externally located environment.

This can be enforced by disabling ``CONFIG_ENV_HANDLING``.
This does not preclude the use of :ref:`Bootchooser` as the
:ref:`barebox-state framework <state_framework>` can be used independently.

If environment handling is needed for other purposes, denying
``SCONFIG_ENVIRONMENT_LOAD`` in the
:ref:`security policy <use_security-policies>` keeps barebox from loading an
environment from media.

What matters is not the environment itself, but the global variables it sets.
They select the boot source, end up on the kernel command line and, unless
signature checking is pinned, decide whether images are verified at all. Any
way to arbitrarily set global variables, be it a writable environment,
a script on media or a shell, defeats verified boot regardless of how well
the images are signed.

Avoiding use of file systems
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

File systems are among the most complex parser code in barebox and a common
source of bugs.
Unlike Linux with its dm-verity support, barebox currently has no way to
verify a file system before mounting it.

The consequence is that in a verified boot setup, barebox should **never**
be allowed to mount file systems.
Especially, :ref:`bootloader spec files <bootloader_spec>` should not be used
in verified boot setups and signed FIT images **must** be located outside
a file system and directly in a raw partition.

Prevent the kernel from booting the rootfs in verity boots
----------------------------------------------------------

In systems, where barebox loads an initramfs that sets up a dm-verity rootfs and
passes the location of the root file system on the kernel command-line, make
sure not to use ``root=``!
``root=`` is also interpreted by the kernel and can lead to the kernel mounting
the rootfs without dm-verity, if the initramfs failed to load, e.g. due to a
different compression algorithm.

The fail-safe alternative is to use a parameter name understood only by the
initramfs (e.g. ``verity_root=``) in all bootloader scripts. If the
``root=$dev`` is fixed up by barebox dynamically, the
:ref:`global.bootm.root_param <magicvar_global_bootm_root_param>` variable can
be used to customize the name of the parameter passed to Linux.

Configuring barebox
-------------------

To aid identification of security impact of config options, barebox provides
two top-level security-related options:

- ``CONFIG_INSECURE``: This enables convenient, but insecure, defaults.
  Any secure system should disable this option.

- ``CONFIG_HAS_INSECURE_DEFAULTS``: This is selected by options that have
  an outsized potential of compromising security. It's recommended that
  all configuration options that select ``CONFIG_HAS_INSECURE_DEFAULTS``
  are disabled.
  If not possible, special care needs to be taken in vetting the insecure
  defaults in question.

.. note:: The barebox configuration must be vetted individually according
 to threat model. Annotating options with HAS_INSECURE_DEFAULTS is
 a work-in-progress and is bound to be incomplete, because there is
 no security one-size-fits-all.

Compile-time configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^

Any code that's eliminated at compile-time is code that can't be exploited by
an attacker. It's thus strongly advisable to keep a separate secure
configuration that disables all features that are used for development and
are not absolutely necessary for booting in the field.

Run-time configuration
^^^^^^^^^^^^^^^^^^^^^^

It's sometimes desirable to retain some ability to debug locked down systems.
While attractive, it's not recommended to retain an insecure bootloader for
the purposes of debugging due to the risk of this bootloader being leaked.

Instead, it's recommended that debugging images are signed specifically to
target only a specific board.

This is sometimes supported out-of-the-box by the SoC like with the HABv4
field return feature.

In the generic case, barebox supports verification of JSON Web Tokens against
a compiled-in RSA public key. Board code should read the JSON Web Token
(e.g., from a raw partition on a USB mass storage device), verify the
serial number claim within against the board's actual serial number and only
then unlock any debugging functionality.

Security Policy
---------------

For general information on supported versions and how to report security
vulnerabilities, refer to the top-level
`SECURITY.md <https://github.com/barebox/barebox/security/policy>`_ document.
