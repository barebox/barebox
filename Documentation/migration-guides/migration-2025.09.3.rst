Release v2025.09.3
==================

FIT Images
----------

The fix for `CVE-2026-33243 <https://nvd.nist.gov/vuln/detail/CVE-2026-33243>`_
has the side effect that barebox after v2025.09.3 will not boot a signed
configuration that excludes some images from the signature.

Previously, it was possible to generate readily exploitable FIT images
by omitting them from ``sign-images`` in the ITS.

If a FIT fails to boot with **v2025.09.3**, when it succesfully booted
v2025.09.2 or earlier, it's likely that it was vulnerable even without
knowledge of CVE-2026-33243.

Recommendation is to not write FIT ITS manually, but to use higher level
tooling that generates the ITS and feeds it to ``mkimage(1)``.

For more details, refer to the `security advisory <https://github.com/barebox/barebox/security/advisories/GHSA-3fvj-q26p-j6h4>`_.
