.. SPDX-License-Identifier: GPL-2.0

Quick Start
===========

barebox' Rust support is derived from Linux, so the kernel's Rust
documentation applies as well.

Requirements
------------

The minimum ``rustc`` and ``bindgen`` versions are listed in
``scripts/min-tool-version.sh``. The Rust standard library source and
``libclang`` are needed as well. With rustup and cargo::

	rustup component add rust-src
	cargo install --locked bindgen-cli

On Debian, install ``rustc``, ``rust-src``, ``bindgen`` and
``libclang-dev``.

``make rustavailable`` explains why a toolchain is rejected.
``CONFIG_RUST`` is only offered with a suitable one.

Building
--------

``CONFIG_RUST`` is currently available on 64-bit sandbox only::

	make ARCH=sandbox rust_defconfig
	make ARCH=sandbox

The ``rust.config`` fragment enables Rust in any other defconfig::

	make ARCH=<arch> <board>_defconfig
	make ARCH=<arch> rust.config

``version`` prints the Rust compiler used for the build and ``bfetch``
lists ``RUST`` among the features.

Editor support
--------------

``make rust-analyzer`` generates a ``rust-project.json`` for
rust-analyzer in the build directory.
