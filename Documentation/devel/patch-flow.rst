.. _patch_flow:

Patch Flow
==========

This document describes the path a patch takes from the mailing list into a
barebox release. See :ref:`contributing` for how to prepare and submit the
patch in the first place.

Branches
--------

Two branches are published in the official barebox repositories:

``master``
  The stable mainline. Releases are branched from here. ``master`` is
  fast-forward only and never rewritten, so it is safe to base work on.

``next``
  The integration branch. It contains everything queued for the next
  release. ``next`` is **not** fast-forward: it is regularly rebuilt from
  scratch and force-pushed. Never base work on ``next`` that you intend to
  keep, and never merge ``next`` into a downstream branch. Internally all
  new features are collected in ``for-next/`` branches from which ``next``
  is merged

From patch to release
---------------------

#. A patch is picked up from the mailing list and applied to the
   ``for-next/`` topic branch matching its subsystem or topic.

#. ``next`` is rebuilt by merging all internal ``for-next/`` branches on top
   of ``master``, and is published for testing and CI.

#. After the release, the ``for-next/`` branches are merged into ``master``
   and deleted. ``next`` is then rebuilt on the new ``master``, and the cycle
   starts over.

The consequence for contributors is that a new feature takes one to two
months to reach a release, depending on where in the cycle it was applied,
while a fix can make the next release. The monthly release schedule and the
release numbering are described in the "Release Strategy" section of the
top-level ``README.rst``.

What to base your work on
-------------------------

New features should be based on ``master`` and targeted for ``next``. Merge
conflicts like Makefile/Kconfig conflicts or context changes will be handled
at the maintainers side. If and only if a patch depends on a feature currently
sitting in ``next`` please base your work on ``next`` and note explicitly when
sending the patch.

GitHub pull requests
--------------------

We also accept GitHub pull requests. Same rules as above apply. Make sure your
work is based on master and the pull request is targeted for ``next`` which
lets the GitHub Logic properly detect when a pull request is applied. Should
you have to base your work on ``next`` for the above reasons your changes will
be cherry picked and the pull request is manually closed.
