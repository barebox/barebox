// SPDX-License-Identifier: GPL-2.0

//! The `barebox` crate.
//!
//! This crate contains the barebox APIs that have been ported or wrapped for
//! usage by Rust code in the barebox and is shared by all of them.

#![no_std]
#![feature(allocator_api)]
// For `#[rustc_std_internal_symbol]` on the allocator shims.
#![feature(rustc_attrs)]
#![allow(internal_features)]

pub mod allocator;
pub mod print;

pub use ffi;
