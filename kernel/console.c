#include "console.h"
#include "asmfuncs.h"

void cons_initalize(struct CONSOLE *cons)
{
	cons->cur_x = 0;
	cons->cur_y = 0;
	cons->col_f = 7;
	cons->col_b = 0;
}

void cons_putchar_hw(int x, int y, int f, int b, int c)
{
	unsigned char *p = (unsigned char *)VRAM + (x + y * MAX_X) * 2;
	
	p[0] = c;
	p[1] = (f & 15) | ((b & 15) << 4);
}

void cons_applycursor(struct CONSOLE *cons)
{
	int x = cons->cur_x;
	int y = cons->cur_y;
	
	int postion = x + y * MAX_X;
	
	io_out8(0x3d4, 0xf);
	io_out8(0x3d5, postion & 0xff);
	
	io_out8(0x3d4, 0xe);
	io_out8(0x3d5, postion >> 8);
}

void cons_putchar(struct CONSOLE *cons, int chr, char move)
{
	if (chr == 0x09) {	/* タブ */
		for (;;) {
			cons_putchar_hw(cons->cur_x, cons->cur_y, cons->col_f, cons->col_b, ' ');
			cons->cur_x++;
			if (cons->cur_x == MAX_X) {
				cons_newline(cons);
			}
			if (((cons->cur_x - 1) & 3) == 0) {
				break;	/* 32で割り切れたらbreak */
			}
		}
	} else if (chr == 0x0a) {	/* 改行 */
		cons_newline(cons);
	} else if (chr == 0x0d) {	/* 復帰 */
		/* とりあえずなにもしない */
	} else {	/* 普通の文字 */
		cons_putchar_hw(cons->cur_x, cons->cur_y, cons->col_f, cons->col_b, chr);
		if (move != 0) {
			/* moveが0のときはカーソルを進めない */
			cons->cur_x++;
			if (cons->cur_x == MAX_X) {
				cons_newline(cons);
			}
		}
	}
	cons_applycursor(cons);
	return;
}

void cons_newline(struct CONSOLE *cons)
{
	int x, y;
	if (cons->cur_y < MAX_Y - 1) {
		cons->cur_y++; /* 次の行へ */
	} else {
		/* スクロール */
		memcpy(VRAM, VRAM+MAX_X*2, ((MAX_X-1)*MAX_Y)*2);
		for (y = MAX_Y - 1; y < MAX_Y; y++) {
			for (x = 0; x < MAX_X; x++) {
				memset(VRAM+(MAX_X*y)*2, 0, MAX_X*2);
			}
		}
	}
	cons->cur_x = 0;
	return;
}

void cons_putstr0(struct CONSOLE *cons, char *s)
{
	unsigned char *sz = (unsigned char *)s;
	for (; *sz != 0; sz++) {
		cons_putchar(cons, *sz, 1);
	}
	return;
}

void cons_putstr1(struct CONSOLE *cons, char *s, int l)
{
	unsigned char *sz = (unsigned char *)s;
	int i;
	for (i = 0; i < l; i++) {
		cons_putchar(cons, sz[i], 1);
	}
	return;
}
