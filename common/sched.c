/* SPDX License Identifier: GPL-2.0 */

#include <bthread.h>
#include <poller.h>
#include <work.h>
#include <slice.h>
#include <sched.h>

void resched(void)
{
	bool run_workqueues = !slice_acquired(&command_slice);

	if (poller_active())
		return;

	command_slice_acquire();

	if (run_workqueues) {
		wq_do_all_works();
		bthread_reschedule();
	}

	poller_call();

	command_slice_release();
}
