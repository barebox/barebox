// SPDX-License-Identifier: GPL-2.0

//! Backend for the [`log`] crate, forwarding records to `pr_print()`.

use core::ffi::{c_int, CStr};
use core::fmt::Arguments;

use kernel::str::CStrExt;
use log::{Level, LevelFilter, Metadata, Record, SetLoggerError};
use macros::export;

use crate::ffi::c_char;
#[cfg(CONFIG_PRINTK)]
use crate::ffi::c_void;

/// Logger forwarding [`log`] records to barebox' `pr_print()`.
pub struct BareboxLogger;

static LOGGER: BareboxLogger = BareboxLogger;

/// Returns the barebox loglevel corresponding to a [`log`] level.
fn barebox_loglevel(level: Level) -> c_int {
    let level = match level {
        Level::Error => bindings::LOGLEVEL_ERR,
        Level::Warn => bindings::LOGLEVEL_WARNING,
        Level::Info => bindings::LOGLEVEL_INFO,
        Level::Debug => bindings::LOGLEVEL_DEBUG,
        Level::Trace => bindings::LOGLEVEL_VDEBUG,
    };

    level as c_int
}

/// Prints already formatted arguments at `level` via barebox' `pr_print()`.
#[cfg_attr(not(CONFIG_PRINTK), allow(unused_variables))]
fn call_pr_print(level: c_int, args: Arguments<'_>) {
    // `pr_print()` is a static inline stub without a console.
    #[cfg(CONFIG_PRINTK)]
    // SAFETY: `%pA` takes a pointer to `core::fmt::Arguments`, which outlives the call.
    unsafe {
        bindings::pr_print(
            level,
            // `c"%pA".as_ptr()` would be a `core::ffi::c_char`, not our unsigned one.
            #[expect(clippy::manual_c_str_literals)]
            b"%pA\0".as_ptr(),
            core::ptr::from_ref(&args).cast::<c_void>(),
        );
    }
}

impl BareboxLogger {
    /// Installs the logger and sets the maximum level reported to it.
    ///
    /// Fails if a logger has already been installed.
    pub fn init(level: LevelFilter) -> Result<(), SetLoggerError> {
        log::set_logger(&LOGGER).map(|()| log::set_max_level(level))
    }
}

impl log::Log for BareboxLogger {
    fn enabled(&self, _metadata: &Metadata<'_>) -> bool {
        // pr_print() filters against global.loglevel.
        true
    }

    fn log(&self, record: &Record<'_>) {
        call_pr_print(
            barebox_loglevel(record.level()),
            format_args!("{}: {}\n", record.target(), record.args()),
        );
    }

    fn flush(&self) {}
}

/// Logs `msg` at barebox loglevel `level`, for the `rustlog` command.
///
/// # Safety
///
/// `msg` must point to a NUL-terminated string that is valid for the duration of the call.
#[export]
unsafe extern "C" fn rust_log(level: c_int, msg: *const c_char) -> c_int {
    let level = match level {
        ..=3 => Level::Error,
        4..=5 => Level::Warn,
        6 => Level::Info,
        7 => Level::Debug,
        _ => Level::Trace,
    };
    // SAFETY: the caller passes a valid NUL-terminated string that outlives the call.
    let msg = unsafe { CStr::from_char_ptr(msg) };

    log::log!(target: "rustlog", level, "{}", msg.to_str().unwrap_or("<invalid utf-8>"));
    0
}

/// Installs [`BareboxLogger`] with the static max level from `CONFIG_COMPILE_LOGLEVEL`.
///
/// # Safety
///
/// Always safe to call. It is only `unsafe` for the `#[export]` signature check.
#[export]
unsafe extern "C" fn rust_setup_log() -> c_int {
    match BareboxLogger::init(log::STATIC_MAX_LEVEL) {
        Ok(()) => 0,
        Err(_) => -(bindings::EBUSY as c_int),
    }
}
