// SPDX-License-Identifier: GPL-2.0
// SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/rust/kernel/bug.rs?id=05f7e89ab9731565d8a62e3b5d1ec206485eeb0b

// Copyright (C) 2024, 2025 FUJITA Tomonori <fujita.tomonori@gmail.com>

//! Support for BUG and WARN functionality.
//!
//! C header: [`include/asm-generic/bug.h`](srctree/include/asm-generic/bug.h)
//!
//! barebox has no `bug_entry` table, so warnings call a C helper with the caller's location.

#[macro_export]
#[doc(hidden)]
#[cfg(CONFIG_BUG)]
macro_rules! warn_flags {
    ($flags:expr) => {
        // SAFETY: `c_str!()` yields a valid null-terminated string and `warn_at()` only prints it.
        unsafe {
            $crate::bindings::warn_at(
                $crate::c_str!(::core::file!()).as_char_ptr(),
                line!() as $crate::ffi::c_uint,
            );
        }
    };
}

#[macro_export]
#[doc(hidden)]
#[cfg(not(CONFIG_BUG))]
macro_rules! warn_flags {
    ($flags:expr) => {};
}

/// Report a warning if `cond` is true and return the condition's evaluation result.
#[macro_export]
macro_rules! warn_on {
    ($cond:expr) => {{
        let cond = $cond;
        if cond {
            $crate::warn_flags!(0);
        }
        cond
    }};
}
