#ifndef READKEY_H
#define READKEY_H

#define CTL_CH(c)		((c) - 'a' + 1)

/* Ascii keys */
#define KEY_ENTER		'\n'
#define KEY_RETURN		'\r'

/* Misc. non-Ascii keys */
#define KEY_UP			CTL_CH('p')	/* cursor key Up	*/
#define KEY_DOWN		CTL_CH('n')	/* cursor key Down	*/
#define KEY_RIGHT		CTL_CH('f')	/* Cursor Key Right	*/
#define KEY_LEFT		CTL_CH('b')	/* cursor key Left	*/
#define KEY_HOME		CTL_CH('a')	/* Cursor Key Home	*/
#define KEY_ERASE_TO_EOL	CTL_CH('k')
#define KEY_REFRESH_TO_EOL	CTL_CH('e')
#define KEY_ERASE_LINE		CTL_CH('x')
#define KEY_INSERT		CTL_CH('o')
#define KEY_CLEAR_SCREEN	CTL_CH('l')
#define KEY_DEL7		127
#define KEY_END			133		/* Cursor Key End	*/
#define KEY_PAGEUP		135		/* Cursor Key Page Up	*/
#define KEY_PAGEDOWN		136		/* Cursor Key Page Down	*/
#define KEY_DEL			137		/* Cursor Key Del	*/

#define ANSI_CLEAR_SCREEN "\e[2J\e[;H"

#define printf_reverse(fmt,args...)	printf("\e[7m" fmt "\e[m",##args)
#define puts_reverse(fmt)		puts("\e[7m" fmt "\e[m")
#define gotoXY(row, col)		printf("\e[%d;%dH", col, row)
#define clear()				puts("\e[2J")

int read_key(void);

#endif /* READKEY_H */

