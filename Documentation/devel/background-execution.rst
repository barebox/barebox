Background execution in barebox
===============================

barebox does not use interrupts to avoid the associated increase in complexity.
Nevertheless it is sometimes desired to execute code "in the background",
like for example polling for completion of transfers or to regularly blink a
heartbeat LED. For these scenarios barebox offers the techniques described below.

Pollers
-------

Pollers are a way in barebox to frequently execute code in the background.
They don't run within their own threads, but instead they are executed
whenever ``is_timeout()`` is called.
This has a few implications. First of all, pollers are not executed when
``is_timeout()`` is not called. For this and other reasons, loops polling for
hardware events should always use a timeout, which is best implemented with
``is_timeout()``. Another thing to remember is that pollers can be executed
anywhere in the middle of other device accesses whenever ``is_timeout()`` is
called. Care must be taken that a poller doesn't access the very same device
again itself. See "slices" below on how devices can safely be accessed from
pollers. Some operations are entirely forbidden from pollers. For example it is
forbidden to access the virtual filesystem, as this could trigger arbitrary
device accesses.  The macro ``assert_command_context()`` is added to those
places to make sure they are not called in the wrong context. The poller
interface is declared in ``include/poller.h``.  ``poller_register()`` is used
to register a poller. The ``struct poller_struct`` passed to
``poller_register()`` needs to have the ``poller_struct.func()`` member
populated with the function that shall be called. From the moment a poller is
registered ``poller_struct.func()`` will be called regularly as long as someone
calls ``is_timeout()``.  A special poller can be registered with
``poller_async_register()``. A poller registered this way won't be called right
away, instead running it can be triggered by calling ``poller_call_async()``.
This will execute the poller after the ``@delay_ns`` argument.
``poller_call_async()`` may also be called from with the poller, so with this
it's possible to run a poller regularly with configurable delays.

Pollers are limited in the things they can do. Poller code must always be
prepared for the case that the resources it accesses are currently busy and
handle this gracefully by trying again later. Most places in barebox either do
not test for resources being busy or return with an error if they are busy.
Calling into the filesystem potentially uses arbitrary other devices, so
doing this from pollers is forbidden. For the same reason it is forbidden
to execute barebox commands from pollers.

Workqueues
----------

Sometimes code wants to access files from poller context or even wants to
execute barebox commands from poller context. The fastboot implementation is an
example for such a case. The fastboot commands come in via USB or network and
the completion and packet receive handlers are executed in poller context. Here
workqueues come into play. They allow to queue work items from poller context.
These work items are executed later at the point where the shell fetches its
next command. At this point we implicitly know that it's allowed to execute
commands or to access the filesystem, because otherwise the shell could not do
it. This means that execution of the next shell command will be delayed until
all work items are being done. Likewise the work items will only be executed
when the current shell command has finished.

The workqueue interface is declared in ``include/work.h``.

``wq_register()`` is the first step to use the workqueue interface.
``wq_register()`` registers a workqueue to which work items can be attached.
Before registering a workqueue a ``struct work_queue`` has to be populated with
two function callbacks.  ``work_queue.fn()`` is the callback that actually does
the work once it is scheduled.  ``work_queue.cancel()`` is a callback which is
executed when the pending work shall be cancelled. This is primarily for
freeing the memory associated to a work item.  ``wq_queue_work()`` is for
actually queueing a work item on a work queue. This can be called from poller
code. Usually a work item is allocated by the poller and then freed either in
``work_queue.fn()`` or in ``work_queue.cancel()``.

bthreads
--------

barebox threads are co-operative green threads, which are scheduled for the
same context as workqueues: Before the shell executes the next command.
This means that bthreads can be used to implement workqueues, but not pollers.

The bthread interface is declared in ``include/bthread.h``.
``bthread_create()`` is used to allocate a bthread control block along with
its stack. ``bthread_wake()`` can be used to add it into the run queue.
From this moment on and until the thread terminates, the thread will be
switched to regularly as long as the shell processes commands.

bthreads are allowed to call ``is_timeout()``, which will eventually
arrange for other threads to execute. This allowed implementing a Linux-like
completion API on top, which can be useful for porting threaded kernel code.

Slices
------

Slices are a way to check if a device is currently busy and thus may not be
called into currently. Pollers wanting to access a device must call
``slice_busy()`` on the slice provided by the device before calling into it.
When the slice is acquired (which can only happen inside a poller) the poller
can't continue at this moment and must try again next time it is executed.
Drivers whose devices provide a slice must call ``slice_acquire()`` before
accessing the device and ``slice_release()`` afterwards. Slices can have
dependencies to other slices, for example a USB network controller uses the
corresponding USB host controller. A dependency can be expressed with
``slice_depends_on()``. With this the USB network controller can add a
dependency from the network device it provides itself to the USB host
controller it depends on.  With this ``slice_busy()`` on the network device
will return ``true`` when the USB host controller is busy.

The usual pattern for using slices is that the device driver for a device
calls ``slice_acquire()`` when entering the driver and ``slice_release()``
before leaving the driver. The driver also provides a function returning
the slice for a device, for example the ethernet support code provides
``struct slice *eth_device_slice(struct eth_device *edev)``. Poller code
which wants to use the ethernet device checks for the availability doing
``slice_busy(eth_device_slice(edev))`` before accessing the ethernet
device. When the slice is not busy the poller can continue with accessing
that device. Otherwise the poller must return and try again next time it
is called.

Limitations
-----------

What barebox does is best described as cooperative multitasking. As pollers
can't be interrupted, it will directly affect the user experience when they
take too long. When barebox reacts sluggishly to key presses, then probably
pollers take too long to execute. A first test if this is the case can
be done by executing ``poller -t`` on the command line. This command will print
how many times we can execute all registered pollers in one second. When this
number is too low then pollers are guilty responsible. Workqueues help to run
schedule/execute longer running code, but during the time while workqueues are
executed nothing else happens. This means that when fastboot flashes an image
in a workqueue then barebox won't react to any key presses on the command line.
The usage of the interfaces described in this document is not yet very
widespread in barebox. The interfaces are used in the places where we need
them, but there are other places which do not use them but should.

For example using a LED driven by a I2C GPIO expander used as hearbeat LED
used to not work properly before addition of slices.
Consider the I2C driver accesses an unrelated I2C device,
like an EEPROM. After having initiated the transfer the driver polls for the
transfer being completed using a ``is_timeout()`` loop. The call to
``is_timeout()`` then calls into the registered pollers from which one accesses
the same I2C bus whose driver is just polling for completion of another
transfer. Without slices, this left the I2C driver in an undefined state,
where it would likely not work anymore.
