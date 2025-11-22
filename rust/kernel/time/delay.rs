// SPDX-License-Identifier: GPL-2.0
// SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/rust/kernel/time/delay.rs?id=7d0a66e4bb9081d75c82ec4957c50034cb0ea449

//! Delay and sleep primitives.
//!
//! This module contains the kernel APIs related to delay and sleep that
//! have been ported or wrapped for usage by Rust code in the kernel.
//!
//! C header: [`include/clock.h`](srctree/include/clock.h).

use super::Delta;
use crate::prelude::*;

/// Sleeps for a given duration at least.
///
/// Equivalent to the C side [`udelay()`], delay function.
///
/// `delta` must be within `[0, i32::MAX]` microseconds;
/// otherwise, it is erroneous behavior. That is, it is considered a bug
/// to call this function with an out-of-range value, in which case the function
/// will sleep for at least the maximum value in the range and may warn
/// in the future.
pub fn delay(delta: Delta) {
    // The maximum value is set to `i32::MAX` microseconds to prevent integer
    // overflow inside delay, which could lead to unintentional infinite sleep.
    const MAX_DELTA: Delta = Delta::from_micros(i32::MAX as i64);

    let delta = if (Delta::ZERO..=MAX_DELTA).contains(&delta) {
        delta
    } else {
        // TODO: Add WARN_ONCE() when it's supported.
        MAX_DELTA
    };

    // SAFETY: It is always safe to call `delay()` with any duration.
    unsafe {
        // Convert the duration to microseconds and round up to preserve
        // the guarantee; `delay()` sleeps for at least the provided duration,
        // but that it may sleep for longer under some circumstances.
        bindings::udelay(delta.as_micros_ceil() as c_ulong)
    }
}
