/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Maurits van der Schee
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *
 * ============================================================================
 * Name        : 2048.c
 * Author      : Maurits van der Schee
 * Description : Console version of the game "2048" for GNU/Linux
 * ============================================================================
 */

#include <common.h>
#include <readkey.h>
#include <command.h>
#include <stdlib.h>

#define SIZE 4
static uint32_t score;

static void getColor(uint16_t value, char *color, size_t length)
{
	uint8_t original[] = {8,255,1,255,2,255,3,255,4,255,5,255,6,255,7,255,9,0,10,0,11,0,12,0,13,0,14,0,255,0,255,0};
	uint8_t *scheme = original;
	uint8_t *background = scheme+0;
	uint8_t *foreground = scheme+1;
	if (value > 0) while (value >>= 1) {
		if (background+2<scheme+sizeof(original)) {
			background+=2;
			foreground+=2;
		}
	}
	snprintf(color,length,"\033[38;5;%d;48;5;%dm",*foreground,*background);
}

static void drawBoard(uint16_t board[SIZE][SIZE])
{
	int8_t x,y;
	char color[40], reset[] = "\033[0m";
	printf("\033[H");

	printf("2048.c %17d pts\n\n",score);

	for (y=0;y<SIZE;y++) {
		for (x=0;x<SIZE;x++) {
			getColor(board[x][y],color,40);
			printf("%s",color);
			printf("       ");
			printf("%s",reset);
		}
		printf("\n");
		for (x=0;x<SIZE;x++) {
			getColor(board[x][y],color,40);
			printf("%s",color);
			if (board[x][y]!=0) {
				char s[8];
				int8_t t;
				snprintf(s,8,"%u",board[x][y]);
				t = 7-strlen(s);
				printf("%*s%s%*s",t-t/2,"",s,t/2,"");
			} else {
				printf("   ·   ");
			}
			printf("%s",reset);
		}
		printf("\n");
		for (x=0;x<SIZE;x++) {
			getColor(board[x][y],color,40);
			printf("%s",color);
			printf("       ");
			printf("%s",reset);
		}
		printf("\n");
	}
	printf("\n");
	printf("        ←,↑,→,↓ or q        \n");
	printf("\033[A");
}

static int8_t findTarget(uint16_t array[SIZE],int8_t x,int8_t stop)
{
	int8_t t;
	/* if the position is already on the first, don't evaluate */
	if (x==0) {
		return x;
	}
	for(t=x-1;t>=0;t--) {
		if (array[t]!=0) {
			if (array[t]!=array[x]) {
				/* merge is not possible, take next position */
				return t+1;
			}
			return t;
		} else {
			/* we should not slide further, return this one */
			if (t==stop) {
				return t;
			}
		}
	}
	/* we did not find a */
	return x;
}

static bool slideArray(uint16_t array[SIZE])
{
	bool success = false;
	int8_t x,t,stop=0;

	for (x=0;x<SIZE;x++) {
		if (array[x]!=0) {
			t = findTarget(array,x,stop);
			/* if target is not original position, then move or merge */
			if (t!=x) {
				/* if target is not zero, set stop to avoid double merge */
				if (array[t]!=0) {
					score+=array[t]+array[x];
					stop = t+1;
				}
				array[t]+=array[x];
				array[x]=0;
				success = true;
			}
		}
	}
	return success;
}

static void rotateBoard(uint16_t board[SIZE][SIZE])
{
	int8_t i,j,n=SIZE;
	uint16_t tmp;
	for (i=0; i<n/2; i++){
		for (j=i; j<n-i-1; j++){
			tmp = board[i][j];
			board[i][j] = board[j][n-i-1];
			board[j][n-i-1] = board[n-i-1][n-j-1];
			board[n-i-1][n-j-1] = board[n-j-1][i];
			board[n-j-1][i] = tmp;
		}
	}
}

static bool moveUp(uint16_t board[SIZE][SIZE])
{
	bool success = false;
	int8_t x;
	for (x=0;x<SIZE;x++) {
		success |= slideArray(board[x]);
	}
	return success;
}

static bool moveLeft(uint16_t board[SIZE][SIZE])
{
	bool success;
	rotateBoard(board);
	success = moveUp(board);
	rotateBoard(board);
	rotateBoard(board);
	rotateBoard(board);
	return success;
}

static bool moveDown(uint16_t board[SIZE][SIZE])
{
	bool success;
	rotateBoard(board);
	rotateBoard(board);
	success = moveUp(board);
	rotateBoard(board);
	rotateBoard(board);
	return success;
}

static bool moveRight(uint16_t board[SIZE][SIZE])
{
	bool success;
	rotateBoard(board);
	rotateBoard(board);
	rotateBoard(board);
	success = moveUp(board);
	rotateBoard(board);
	return success;
}

static bool findPairDown(uint16_t board[SIZE][SIZE])
{
	bool success = false;
	int8_t x,y;
	for (x=0;x<SIZE;x++) {
		for (y=0;y<SIZE-1;y++) {
			if (board[x][y]==board[x][y+1]) return true;
		}
	}
	return success;
}

static int16_t countEmpty(uint16_t board[SIZE][SIZE])
{
	int8_t x,y;
	int16_t count=0;
	for (x=0;x<SIZE;x++) {
		for (y=0;y<SIZE;y++) {
			if (board[x][y]==0) {
				count++;
			}
		}
	}
	return count;
}

static bool gameEnded(uint16_t board[SIZE][SIZE])
{
	bool ended = true;
	if (countEmpty(board)>0) return false;
	if (findPairDown(board)) return false;
	rotateBoard(board);
	if (findPairDown(board)) ended = false;
	rotateBoard(board);
	rotateBoard(board);
	rotateBoard(board);
	return ended;
}

static void addRandom(uint16_t board[SIZE][SIZE])
{
	int8_t x,y;
	int16_t r,len=0;
	uint16_t n,list[SIZE*SIZE][2];

	for (x=0;x<SIZE;x++) {
		for (y=0;y<SIZE;y++) {
			if (board[x][y]==0) {
				list[len][0]=x;
				list[len][1]=y;
				len++;
			}
		}
	}

	if (len>0) {
		r = rand()%len;
		x = list[r][0];
		y = list[r][1];
		n = ((rand()%10)/9+1)*2;
		board[x][y]=n;
	}
}

static int test(void)
{
	uint16_t array[SIZE];
	uint16_t data[] = {
		0,0,0,2,	2,0,0,0,
		0,0,2,2,	4,0,0,0,
		0,2,0,2,	4,0,0,0,
		2,0,0,2,	4,0,0,0,
		2,0,2,0,	4,0,0,0,
		2,2,2,0,	4,2,0,0,
		2,0,2,2,	4,2,0,0,
		2,2,0,2,	4,2,0,0,
		2,2,2,2,	4,4,0,0,
		4,4,2,2,	8,4,0,0,
		2,2,4,4,	4,8,0,0,
		8,0,2,2,	8,4,0,0,
		4,0,2,2,	4,4,0,0
	};
	uint16_t *in,*out;
	uint16_t t,tests;
	uint8_t i;
	bool success = true;

	tests = (sizeof(data)/sizeof(data[0]))/(2*SIZE);
	for (t=0;t<tests;t++) {
		in = data+t*2*SIZE;
		out = in + SIZE;
		for (i=0;i<SIZE;i++) {
			array[i] = in[i];
		}
		slideArray(array);
		for (i=0;i<SIZE;i++) {
			if (array[i] != out[i]) {
				success = false;
			}
		}
		if (success==false) {
			for (i=0;i<SIZE;i++) {
				printf("%d ",in[i]);
			}
			printf("=> ");
			for (i=0;i<SIZE;i++) {
				printf("%d ",array[i]);
			}
			printf("expected ");
			for (i=0;i<SIZE;i++) {
				printf("%d ",in[i]);
			}
			printf("=> ");
			for (i=0;i<SIZE;i++) {
				printf("%d ",out[i]);
			}
			printf("\n");
			break;
		}
	}
	if (success) {
		printf("All %u tests executed successfully\n",tests);
	}
	return !success;
}

static int do_2048(int argc, char *argv[])
{
	uint16_t board[SIZE][SIZE];
	char c;
	bool success;

	if (argc == 2 && strcmp(argv[1],"test")==0) {
		return test();
	}

	score = 0;

	printf("\033[?25l\033[2J\033[H");

	memset(board,0,sizeof(board));
	addRandom(board);
	addRandom(board);
	drawBoard(board);
	while (true) {
		c=read_key();
		switch(c) {
			case BB_KEY_LEFT: /* left arrow */
				success = moveLeft(board);  break;
			case BB_KEY_RIGHT: /* right arrow */
				success = moveRight(board); break;
			case BB_KEY_UP:	/* up arrow */
				success = moveUp(board);    break;
			case BB_KEY_DOWN: /* down arrow */
				success = moveDown(board);  break;
			default: success = false;
		}
		if (success) {
			drawBoard(board);
			udelay(150000);
			addRandom(board);
			drawBoard(board);
			if (gameEnded(board)) {
				printf("         GAME OVER          \n");
				break;
			}
		}
		if (c=='q') {
			printf("            QUIT            \n");
			break;
		}
	}

	printf("\033[?25h");

	return 0;
}

BAREBOX_CMD_HELP_START(2048)
BAREBOX_CMD_HELP_TEXT("Use your arrow keys to move the tiles. When two tiles with")
BAREBOX_CMD_HELP_TEXT("the same number touch, they merge into one!")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(2048)
	.cmd		= do_2048,
	BAREBOX_CMD_DESC("the 2048 game")
	BAREBOX_CMD_GROUP(CMD_GRP_MISC)
	BAREBOX_CMD_HELP(cmd_2048_help)
BAREBOX_CMD_END
