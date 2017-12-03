#include "console.h"
#include "asmfuncs.h"

#include "vga_font.h"

void cons_putchar_hw(int x, int y, int c)
{
	for(int i = 0; i < 16; i++)
	{
		unsigned char *p = (unsigned char *)0xa0000+((y*16+i)*(640/8)+x);
		io_out16(0x3ce, 0x0000);
		*p = 0xff;
		io_out16(0x3ce, 0x0700);
		int dmy = *p;
		*p = vga_font[c * 16 + i];
	}
}

void cons_applycursor(struct CONSOLE *cons, int del)
{
	int x = cons->cur_x;
	int y = cons->cur_y;
	
	
	if(del) {
		io_out16(0x3ce, 0 << 8);
		for(int i = 14; i < 16; i++) *((unsigned char *)0xa0000+((y*16+i)*(640/8)+x)) = 0xff;
	} else {
		io_out16(0x3ce, 15 << 8);
		for(int i = 14; i < 16; i++) *((unsigned char *)0xa0000+((y*16+i)*(640/8)+x)) = 0x7e;
	}
}

void cons_putchar(struct CONSOLE *cons, int chr, char move)
{
	cons_applycursor(cons,1);
	if (chr == 0x09) {	/* タブ */
		for (;;) {
			cons_putchar_hw(cons->cur_x, cons->cur_y, ' ');
			cons->cur_x++;
			if (cons->cur_x == 80) {
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
		cons_putchar_hw(cons->cur_x, cons->cur_y, chr);
		if (move != 0) {
			/* moveが0のときはカーソルを進めない */
			cons->cur_x++;
			if (cons->cur_x == 80) {
				cons_newline(cons);
			}
		}
	}
	cons_applycursor(cons,0);
	return;
}

void cons_newline(struct CONSOLE *cons)
{
	int x, y;
	if (cons->cur_y < 30 - 1) {
		cons->cur_y++; /* 次の行へ */
	} else {
		/* スクロール */
		io_out16(0x3ce, 0x0700);
		memcpy(0xa0000, 0xa0000+(640/8)*16, (640/8)*16*29);
		for (y = MAX_Y - 1; y < MAX_Y; y++) {
			for (x = 0; x < MAX_X; x++) {
				memset(0xa0000+(640/8)*16*y, 0, (640/8)*16);
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
