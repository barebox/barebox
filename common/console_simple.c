#include <config.h>
#include <common.h>
#include <fs.h>
#include <errno.h>
#include <debug_ll.h>
#include <console.h>

LIST_HEAD(console_list);
EXPORT_SYMBOL(console_list);
static struct console_device *console;

int console_puts(unsigned int ch, const char *str)
{
	const char *s = str;
	int i = 0;

	while (*s) {
		console_putc(ch, *s);
		s++;
		i++;
	}

	return i;
}
EXPORT_SYMBOL(console_puts);

void console_putc(unsigned int ch, char c)
{
	if (!console) {
		if (c == '\n')
			putc_ll('\r');
		putc_ll(c);
		return;
	}

	if (c == '\n')
		console->putc(console, '\r');

	console->putc(console, c);
}
EXPORT_SYMBOL(console_putc);

int tstc(void)
{
	if (!console)
		return 0;

	return console->tstc(console);
}
EXPORT_SYMBOL(tstc);

int getchar(void)
{
	if (!console)
		return -EINVAL;
	return console->getc(console);
}
EXPORT_SYMBOL(getchar);

void console_flush(void)
{
	if (console && console->flush)
		console->flush(console);
}
EXPORT_SYMBOL(console_flush);

#ifndef ARCH_HAS_CTRLC
/* test if ctrl-c was pressed */
int ctrlc (void)
{
	if (tstc() && getchar() == 3)
		return 1;
	return 0;
}
EXPORT_SYMBOL(ctrlc);
#endif /* ARCH_HAS_CTRC */

int console_register(struct console_device *newcdev)
{
	if (console)
		return -EBUSY;

	console = newcdev;
	console_list.prev = console_list.next = &newcdev->list;
	newcdev->list.prev = newcdev->list.next = &console_list;

	if (newcdev->setbrg) {
		newcdev->baudrate = CONFIG_BAUDRATE;
		newcdev->setbrg(newcdev, newcdev->baudrate);
	}

	newcdev->f_active = CONSOLE_STDIN | CONSOLE_STDOUT | CONSOLE_STDERR;

	barebox_banner();

	return 0;
}

int console_unregister(struct console_device *cdev)
{
	return -EBUSY;
}
