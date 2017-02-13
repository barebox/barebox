#ifndef READKEY_H
#define READKEY_H

#define CTL_CH(c)		((c) - 'a' + 1)

/* Ascii keys */
#define BB_KEY_ENTER		'\n'
#define BB_KEY_RETURN		'\r'

/* Misc. non-Ascii keys */
#define BB_KEY_UP		CTL_CH('p')	/* cursor key Up	*/
#define BB_KEY_DOWN		CTL_CH('n')	/* cursor key Down	*/
#define BB_KEY_RIGHT		CTL_CH('f')	/* Cursor Key Right	*/
#define BB_KEY_LEFT		CTL_CH('b')	/* cursor key Left	*/
#define BB_KEY_HOME		CTL_CH('a')	/* Cursor Key Home	*/
#define BB_KEY_ERASE_TO_EOL	CTL_CH('k')
#define BB_KEY_REFRESH_TO_EOL	CTL_CH('e')
#define BB_KEY_ERASE_LINE	CTL_CH('x')
#define BB_KEY_INSERT		CTL_CH('o')
#define BB_KEY_CLEAR_SCREEN	CTL_CH('l')
#define BB_KEY_DEL7		127
#define BB_KEY_END		133		/* Cursor Key End	*/
#define BB_KEY_PAGEUP		135		/* Cursor Key Page Up	*/
#define BB_KEY_PAGEDOWN		136		/* Cursor Key Page Down	*/
#define BB_KEY_DEL		137		/* Cursor Key Del	*/

#define ANSI_CLEAR_SCREEN "\e[2J\e[;H"

#define printf_reverse(fmt,args...)	printf("\e[7m" fmt "\e[0m",##args)
#define puts_reverse(fmt)		puts("\e[7m" fmt "\e[0m")
#define gotoXY(row, col)		printf("\e[%d;%dH", col, row)
#define clear()				puts("\e[2J")

int read_key(void);

#endif /* READKEY_H */

