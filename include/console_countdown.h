#ifndef __CONSOLE_COUNTDOWN_H
#define __CONSOLE_COUNTDOWN_H

#define CONSOLE_COUNTDOWN_SILENT (1 << 0)
#define CONSOLE_COUNTDOWN_ANYKEY (1 << 1)
#define CONSOLE_COUNTDOWN_RETURN (1 << 3)
#define CONSOLE_COUNTDOWN_CTRLC (1 << 4)
#define CONSOLE_COUNTDOWN_EXTERN (1 << 5)

int console_countdown(int timeout_s, unsigned flags, char *out_key);
void console_countdown_abort(void);

#endif /* __CONSOLE_COUNTDOWN_H */
