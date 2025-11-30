// SPDX-License-Identifier: GPL-2.0

//! Printing facilities.
//!
//! C header: [`include/printf.h`](srctree/include/printf.h)

#[cfg(CONFIG_PRINTK)]
use crate::ffi::c_void;
pub use core::fmt::Arguments;

/// Prints already formatted arguments via barebox' `printf()`.
///
/// Public but hidden since it should only be used by [`printf!`].
///
/// [`printf!`]: crate::printf
#[doc(hidden)]
#[cfg_attr(not(CONFIG_PRINTK), allow(unused_variables))]
pub fn call_printf(args: Arguments<'_>) {
    // `printf()` is a static inline stub without a console.
    #[cfg(CONFIG_PRINTK)]
    // SAFETY: `%pA` takes a pointer to `core::fmt::Arguments`, which outlives the call.
    unsafe {
        bindings::printf(
            // `c"%pA".as_ptr()` would be a `core::ffi::c_char`, not our unsigned one.
            #[expect(clippy::manual_c_str_literals)]
            b"%pA\0".as_ptr(),
            core::ptr::from_ref(&args).cast::<c_void>(),
        );
    }
}

/// Prints to the console without log level, like C's `printf()`.
#[macro_export]
macro_rules! printf (
    ($($arg:tt)+) => (
        $crate::print::call_printf(core::format_args!($($arg)+))
    );
);
