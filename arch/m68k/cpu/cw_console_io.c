/*
 * Copyright (c) 2008 Carsten Schlote <c.schlote@konzeptpark.de>
 * See file CREDITS for list of people who contributed to this project.
 *
 * This file is part of barebox.
 *
 * barebox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * barebox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with barebox.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  Debug output stubs over BDM for Codewarrior
 */
#include <common.h>
#include <command.h>
#include <console.h>
#include <reloc.h>
#include <init.h>

#ifdef CONFIG_HAS_EARLY_INIT


#if 0 // FIXME - make a CW debug port serial driver for barebox

/*
 * The result of an I/O command can be any one of the following.
 */
typedef enum DSIOResult
{
	kDSIONoError	= 0x00,
	kDSIOError	= 0x01,
	kDSIOEOF	= 0x02
} DSIOResult;

/*
 *	MessageCommandID
 */
typedef enum MessageCommandID
{
	/*
	 * target->host support commands
	 */

	kDSWriteFile	= 0xD0,		/*		L2	L3		*/
	kDSReadFile	= 0xD1 		/*		L2	L3		*/

} MessageCommandID;


enum DSIOResult TransferData(
	MessageCommandID msg,
	unsigned char *buffer, int size,
	int * txsize
)
{
	enum DSIOResult iores = kDSIOError;
	unsigned long sized2=0;

	/* -- Call codewarrior stub -- */
	__asm__ __volatile__ (
"       move.l 	%[cmd],%%d0    \n"
"       move.l 	#0,%%d1         \n"
"       move.l 	%[size],%%d2    \n"
"       move.l 	%[buffer],%%d3  \n"
"	trap	#14            \n"
"	move.l	%%d1,%[txsize]  \n"
"	move.l  %%d0,%[res]     \n"
	: [res] "=r" (iores), [txsize] "=g" (sized2)
	: [cmd] "g" (msg), [size] "g" (size), [buffer] "g" (buffer)
	: "d2","d3" );

	if (txsize!=NULL) *txsize=sized2;
	return iores;
}

void *get_early_console_base(const char *name)
{
	return (void*)0xdeadbeef;
}

static unsigned char early_iobuffer[80];
static int early_iobuffer_cnt;

void early_console_putc(void *base, char c)
{
	early_iobuffer[early_iobuffer_cnt++] = c;
	if ( ( early_iobuffer_cnt >= sizeof(early_iobuffer)) ||
	     (c == '\n') )
	{
		TransferData(kDSWriteFile,early_iobuffer,early_iobuffer_cnt, NULL);
		early_iobuffer_cnt = 0;
	}
}

void early_console_init(void *base, int baudrate)
{
	early_iobuffer_cnt = 0;
}

//void early_console_start(const char *name, int baudrate)
//{
//}

#endif

#endif
