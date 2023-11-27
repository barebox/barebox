/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __LINUX_NOTIFIER_H
#define __LINUX_NOTIFIER_H

#include <notifier.h>

#define BLOCKING_NOTIFIER_HEAD			NOTIFIER_HEAD

#define blocking_notifier_call_chain		notifier_call_chain

#define blocking_notifier_head			notifier_head

#define blocking_notifier_chain_register	notifier_chain_register
#define blocking_notifier_chain_unregister	notifier_chain_unregister

#endif
