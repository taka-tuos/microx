#include <stdio.h>

#include "sheet.h"
#include "bootpack.h"
#include "nvbdflib.h"

extern BDF_FONT *bdf;

struct SHEET *bdfFB;
int bdfColor;

void boxfill(struct SHEET *sht, int x0, int y0, int x1, int y1, int c)
{
	for(int y = y0; y < y1; y++) {
		for(int x = x0; x < x1; x++) {
			sht->buf[sht->bxsize * y + x] = c;
		}
	}
}

void swaprgb(unsigned int *p, int sx, int sy)
{
	int x, y;
	
	for(y = 0; y < sy; y++) {
		for(x = 0; x < sx; x++) {
			int c = p[y * sx + x];
			int a = (c >> 24) & 0xff;
			int r = (c >> 16) & 0xff;
			int g = (c >>  8) & 0xff;
			int b = (c >>  0) & 0xff;
			
			p[y * sx + x] = (a << 24) | (b << 16) | (g << 8) | r;
		}
	}
}

void bdfDot(int x, int y, int c)
{
	if(c) bdfFB->buf[y * bdfFB->bxsize + x] = bdfColor;
}

void change_wtitle8(struct SHEET *sht, char act)
{
	int x, y, xsize = sht->bxsize;
	int c, tc_new, tbc_new, tc_old, tbc_old, *buf = sht->buf;
	if (act != 0) {
		tc_new  = 0xffffff;
		tbc_new = 0x000088;
		tc_old  = 0x888888;
		tbc_old = 0x666666;
	} else {
		tc_new  = 0x888888;
		tbc_new = 0x666666;
		tc_old  = 0xffffff;
		tbc_old = 0x000088;
	}
	for (y = 3; y <= 20; y++) {
		for (x = 3; x <= xsize - 4; x++) {
			if(!(y >= 5 && y < 5 + 14 && x >= xsize - 21 && x < xsize - 21 + 16)) {
				c = buf[y * xsize + x];
				if (c == tc_old) {
					c = tc_new;
				} else if (c == tbc_old) {
					c = tbc_new;
				}
				buf[y * xsize + x] = c;
			}
		}
	}
	sheet_refresh(sht, 3, 3, xsize, 21);
	return;
}

void make_window(struct SHEET *sht_win, char *title)
{
	int x, y;
	static char closebtn[14][16] = {
		"OOOOOOOOOOOOOOO@", "OQQQQQQQQQQQQQ$@", "OQQQQQQQQQQQQQ$@",
		"OQQQ@@QQQQ@@QQ$@", "OQQQQ@@QQ@@QQQ$@", "OQQQQQ@@@@QQQQ$@",
		"OQQQQQQ@@QQQQQ$@", "OQQQQQ@@@@QQQQ$@", "OQQQQ@@QQ@@QQQ$@",
		"OQQQ@@QQQQ@@QQ$@", "OQQQQQQQQQQQQQ$@", "OQQQQQQQQQQQQQ$@",
		"O$$$$$$$$$$$$$$@", "@@@@@@@@@@@@@@@@"};
	
	sht_win->flag2 = 0;
	sht_win->act = 0;
	
	bdfSetDrawingAreaSize(sht_win->bxsize, sht_win->bysize);
	
	boxfill(sht_win, 0, 0, sht_win->bxsize - 0, sht_win->bysize - 0, 0x000000);
	boxfill(sht_win, 0, 0, sht_win->bxsize - 1, sht_win->bysize - 1, 0xcccccc);
	boxfill(sht_win, 1, 1, sht_win->bxsize - 1, sht_win->bysize - 1, 0x888888);
	boxfill(sht_win, 1, 1, sht_win->bxsize - 2, sht_win->bysize - 2, 0xffffff);
	boxfill(sht_win, 2, 2, sht_win->bxsize - 2, sht_win->bysize - 2, 0xcccccc);
	boxfill(sht_win, 3, 3, sht_win->bxsize - 3, 21, 0x000088);
	
	for (y = 0; y < 14; y++) {
		for (x = 0; x < 16; x++) {
			int c = closebtn[y][x];
			if (c == '@') {
				c = 0x000000;
			} else if (c == '$') {
				c = 0x888888;
			} else if (c == 'Q') {
				c = 0xcccccc;
			} else {
				c = 0xffffff;
			}
			sht_win->buf[(5 + y) * sht_win->bxsize + (sht_win->bxsize - 21 + x)] = c;
		}
	}
	
	bdfFB = sht_win;
	bdfColor = 0xffffff;
	
	bdfPrintString(bdf,6,6,title);
	
	sheet_refresh(sht_win,0,0,sht_win->bxsize,sht_win->bysize);
}
