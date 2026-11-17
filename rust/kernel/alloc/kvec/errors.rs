// SPDX-License-Identifier: GPL-2.0
// SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/rust/kernel/alloc/kvec/errors.rs?id=7d0a66e4bb9081d75c82ec4957c50034cb0ea449

//! Errors for the [`Vec`] type.

use kernel::fmt;
use kernel::prelude::*;

/// Error type for [`Vec::push_within_capacity`].
pub struct PushError<T>(pub T);

impl<T> fmt::Debug for PushError<T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "Not enough capacity")
    }
}

impl<T> From<PushError<T>> for Error {
    fn from(_: PushError<T>) -> Error {
        // Returning ENOMEM isn't appropriate because the system is not out of memory. The vector
        // is just full and we are refusing to resize it.
        EINVAL
    }
}

/// Error type for [`Vec::remove`].
pub struct RemoveError;

impl fmt::Debug for RemoveError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "Index out of bounds")
    }
}

impl From<RemoveError> for Error {
    fn from(_: RemoveError) -> Error {
        EINVAL
    }
}

/// Error type for [`Vec::insert_within_capacity`].
pub enum InsertError<T> {
    /// The value could not be inserted because the index is out of bounds.
    IndexOutOfBounds(T),
    /// The value could not be inserted because the vector is out of capacity.
    OutOfCapacity(T),
}

impl<T> fmt::Debug for InsertError<T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            InsertError::IndexOutOfBounds(_) => write!(f, "Index out of bounds"),
            InsertError::OutOfCapacity(_) => write!(f, "Not enough capacity"),
        }
    }
}

impl<T> From<InsertError<T>> for Error {
    fn from(_: InsertError<T>) -> Error {
        EINVAL
    }
}
