// SPDX-License-Identifier: Apache-2.0 OR MIT
// SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/rust/pin-init/examples/error.rs?id=8cd9520d35a6c38db6567e97dd93b1f11f185dc6

#![cfg_attr(feature = "alloc", feature(allocator_api))]

use core::convert::Infallible;

#[cfg(feature = "alloc")]
use std::alloc::AllocError;

#[derive(Debug)]
pub struct Error;

impl From<Infallible> for Error {
    fn from(e: Infallible) -> Self {
        match e {}
    }
}

#[cfg(feature = "alloc")]
impl From<AllocError> for Error {
    fn from(_: AllocError) -> Self {
        Self
    }
}

#[allow(dead_code)]
fn main() {
    let _ = Error;
}
