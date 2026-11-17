// SPDX-License-Identifier: GPL-2.0
// SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/rust/kernel/lib.rs?id=7d0a66e4bb9081d75c82ec4957c50034cb0ea449

//! The `kernel` crate.
//!
//! This crate contains the kernel APIs that have been ported or wrapped for
//! usage by Rust code in the kernel and is shared by all of them.
//!
//! In other words, all the rest of the Rust code in the kernel (e.g. kernel
//! modules written in Rust) depends on [`core`] and this crate.
//!
//! If you need a kernel C API that is not ported or wrapped yet here, then
//! do so first instead of bypassing this crate.

#![no_std]
//
// Please see https://github.com/Rust-for-Linux/linux/issues/2 for details on
// the unstable features in use.
//
// Expected to become stable.
#![feature(arbitrary_self_types)]
#![feature(derive_coerce_pointee)]
//
// To be determined.
#![feature(used_with_arg)]
//
// `feature(file_with_nul)` is expected to become stable. Before Rust 1.89.0, it did not exist, so
// enable it conditionally.
#![cfg_attr(CONFIG_RUSTC_HAS_FILE_WITH_NUL, feature(file_with_nul))]

// Ensure conditional compilation based on the kernel configuration works;
// otherwise we may silently break things like initcall handling.
#[cfg(not(CONFIG_RUST))]
compile_error!("Missing kernel configuration for conditional compilation");

// Allow proc-macros to refer to `::kernel` inside the `kernel` crate (this crate).
extern crate self as kernel;

pub use ffi;

pub mod alloc;
pub mod bitmap;
pub mod bits;
pub mod bug;
#[doc(hidden)]
pub mod build_assert;
pub mod error;
pub mod fmt;
pub mod init;
pub mod prelude;
pub mod print;
pub mod processor;
pub mod ptr;
pub mod sizes;
mod static_assert;
#[doc(hidden)]
pub mod std_vendor;
pub mod str;
pub mod time;
pub mod transmute;
pub mod types;

#[doc(hidden)]
pub use bindings;
pub use macros;

/// Prefix to appear before log messages printed from within the `kernel` crate.
const __LOG_PREFIX: &[u8] = b"rust_kernel\0";

#[cfg(not(testlib))]
#[panic_handler]
fn panic(info: &core::panic::PanicInfo<'_>) -> ! {
    pr_emerg!("{}\n", info);
    // SAFETY: FFI call.
    unsafe { bindings::BUG() };
}

/// Produces a pointer to an object from a pointer to one of its fields.
///
/// If you encounter a type mismatch due to the [`Opaque`] type, then use [`Opaque::cast_into`] or
/// [`Opaque::cast_from`] to resolve the mismatch.
///
/// [`Opaque`]: crate::types::Opaque
/// [`Opaque::cast_into`]: crate::types::Opaque::cast_into
/// [`Opaque::cast_from`]: crate::types::Opaque::cast_from
///
/// # Safety
///
/// The pointer passed to this macro, and the pointer returned by this macro, must both be in
/// bounds of the same allocation.
///
/// # Examples
///
/// ```
/// # use kernel::container_of;
/// struct Test {
///     a: u64,
///     b: u32,
/// }
///
/// let test = Test { a: 10, b: 20 };
/// let b_ptr: *const _ = &test.b;
/// // SAFETY: The pointer points at the `b` field of a `Test`, so the resulting pointer will be
/// // in-bounds of the same allocation as `b_ptr`.
/// let test_alias = unsafe { container_of!(b_ptr, Test, b) };
/// assert!(core::ptr::eq(&test, test_alias));
/// ```
#[macro_export]
macro_rules! container_of {
    ($field_ptr:expr, $Container:ty, $($fields:tt)*) => {{
        let offset: usize = ::core::mem::offset_of!($Container, $($fields)*);
        let field_ptr = $field_ptr;
        let container_ptr = field_ptr.byte_sub(offset).cast::<$Container>();
        $crate::assert_same_type(field_ptr, (&raw const (*container_ptr).$($fields)*).cast_mut());
        container_ptr
    }}
}

/// Helper for [`container_of!`].
#[doc(hidden)]
pub fn assert_same_type<T>(_: T, _: T) {}

/// Helper for `.rs.S` files.
#[doc(hidden)]
#[macro_export]
macro_rules! concat_literals {
    ($( $asm:literal )* ) => {
        ::core::concat!($($asm),*)
    };
}

/// Wrapper around `asm!` configured for use in the kernel.
///
/// Uses a semicolon to avoid parsing ambiguities, even though this does not match native `asm!`
/// syntax.
// For x86, `asm!` uses intel syntax by default, but we want to use at&t syntax in the kernel.
#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
#[macro_export]
macro_rules! asm {
    ($($asm:expr),* ; $($rest:tt)*) => {
        ::core::arch::asm!( $($asm)*, options(att_syntax), $($rest)* )
    };
}

/// Wrapper around `asm!` configured for use in the kernel.
///
/// Uses a semicolon to avoid parsing ambiguities, even though this does not match native `asm!`
/// syntax.
// For non-x86 arches we just pass through to `asm!`.
#[cfg(not(any(target_arch = "x86", target_arch = "x86_64")))]
#[macro_export]
macro_rules! asm {
    ($($asm:expr),* ; $($rest:tt)*) => {
        ::core::arch::asm!( $($asm)*, $($rest)* )
    };
}

/// Gets the C string file name of a [`Location`].
///
/// If `Location::file_as_c_str()` is not available, returns a string that warns about it.
///
/// [`Location`]: core::panic::Location
///
/// # Examples
///
/// ```
/// # use kernel::file_from_location;
///
/// #[track_caller]
/// fn foo() {
///     let caller = core::panic::Location::caller();
///
///     // Output:
///     // - A path like "rust/kernel/example.rs" if `file_as_c_str()` is available.
///     // - "<Location::file_as_c_str() not supported>" otherwise.
///     let caller_file = file_from_location(caller);
///
///     // Prints out the message with caller's file name.
///     pr_info!("foo() called in file {caller_file:?}\n");
///
///     # if cfg!(CONFIG_RUSTC_HAS_FILE_WITH_NUL) {
///     #     assert_eq!(Ok(caller.file()), caller_file.to_str());
///     # }
/// }
///
/// # foo();
/// ```
#[inline]
pub fn file_from_location<'a>(loc: &'a core::panic::Location<'a>) -> &'a core::ffi::CStr {
    #[cfg(CONFIG_RUSTC_HAS_FILE_AS_C_STR)]
    {
        loc.file_as_c_str()
    }

    #[cfg(all(CONFIG_RUSTC_HAS_FILE_WITH_NUL, not(CONFIG_RUSTC_HAS_FILE_AS_C_STR)))]
    {
        loc.file_with_nul()
    }

    #[cfg(not(CONFIG_RUSTC_HAS_FILE_WITH_NUL))]
    {
        let _ = loc;
        c"<Location::file_as_c_str() not supported>"
    }
}
