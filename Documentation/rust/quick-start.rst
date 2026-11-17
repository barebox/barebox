.. SPDX-License-Identifier: GPL-2.0

Quick Start
===========

barebox' Rust support is derived from Linux, so the kernel's Rust
documentation applies as well.

Requirements
------------

The minimum ``rustc`` and ``bindgen`` versions are listed in
``scripts/min-tool-version.sh``. Like Linux, barebox follows the
versions shipped by Debian stable, currently Debian 13 (Trixie).
Newer toolchains are supported as well; new warnings they emit are
fixed like new C compiler warnings.

The Rust standard library source and ``libclang`` are needed as well.
With rustup and cargo::

	rustup component add rust-src
	cargo install --locked bindgen-cli

On Debian, install ``rustc``, ``rust-src``, ``bindgen`` and
``libclang-dev``.

``make rustavailable`` explains why a toolchain is rejected.
``CONFIG_RUST`` is only offered with a suitable one.

Building
--------

``CONFIG_RUST`` is available on 64-bit sandbox, ARMv7 and ARM64::

	make ARCH=sandbox rust_defconfig
	make ARCH=sandbox

The ``rust.config`` fragment enables Rust in any other defconfig::

	make ARCH=arm64 multi_v8_defconfig
	make ARCH=arm64 rust.config

``version`` prints the Rust compiler used for the build and ``bfetch``
lists ``RUST`` among the features.

Editor support
--------------

``make rust-analyzer`` generates a ``rust-project.json`` for
rust-analyzer in the build directory.

Writing Rust code
-----------------

Rust code in barebox is ``no_std``: ``core`` and ``alloc`` can be used,
``std`` can't. A global allocator on top of barebox' heap backs ``Box``,
``Vec``, ``String`` and the other ``alloc`` types. ``Rc``, ``Arc`` and
``CString`` are left out to save space.

External crates
---------------

The build doesn't use cargo, so crates from crates.io need to be
vendored into ``rust/vendor/`` and checked in unmodified.
Addition of new crates must be well-reasoned. To add one, list it
in the dummy package ``rust/Cargo.toml`` and run::

	cd rust && cargo vendor

Then add a build rule for the crate to ``rust/Makefile`` and an entry
to ``scripts/generate_rust_analyzer.py``, so rust-analyzer finds it.

This doesn't apply to crates that have been vendored into the
Linux kernel:
Those should be imported with the least amount of modification,
to simplify future syncs against the Linux Rust support.
