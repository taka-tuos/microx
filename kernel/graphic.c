#include <stdio.h>
#include <stdint.h>

#include "sheet.h"
#include "bootpack.h"
#include "nvbdflib.h"
#include "unicode.h"

extern BDF_FONT *bdf;
extern BDF_FONT *bdfj;

struct SHEET *bdfFB;
int bdfColor;

int tca1[256];
int tca2[256];

void graphic_init()
{
	for(int i = 0; i < 256; i++) {
		int tir = (0x0b00 + 0x9b * i) >> 8;
		int tig = (0x2500 + 0xa5 * i) >> 8;
		int tib = (0x6b00 + 0x85 * i) >> 8;
		
		tca1[i] = (tir << 16) | (tig << 8) | tib;
	}
	
	for(int i = 0; i < 256; i++) {
		int tir = (0x8000 + 0x36 * i) >> 8;
		int tig = (0x8000 + 0x36 * i) >> 8;
		int tib = (0x8000 + 0x36 * i) >> 8;
		
		tca2[i] = (tir << 16) | (tig << 8) | tib;
	}
}

int charUFT8toUTF16(uint16_t *pUTF16, char *pUTF8)
{ 
	uint8_t bytes[3]; 
	uint16_t unicode16; 
	
	bytes[0] = *pUTF8++; 
	if( bytes[0] < 0x80 ) { 
			*pUTF16 = bytes[0]; 
			return(1); 
	} 
	bytes[1] = *pUTF8++; 
	if( bytes[0] >= 0xC0 && bytes[0] < 0xE0 )  { 
			unicode16 = 0x1f&bytes[0]; 
			*pUTF16 = (unicode16<<6)+(0x3f&bytes[1]); 
			return(2); 
	} 
	
	bytes[2] = *pUTF8++; 
	if( bytes[0] >= 0xE0 && bytes[0] < 0xF0 ) { 
			unicode16 = 0x0f&bytes[0]; 
			unicode16 = (unicode16<<6)+(0x3f&bytes[1]); 
			*pUTF16 = (unicode16<<6)+(0x3f&bytes[2]); 
			return(3); 
	} else 
	return(0); 
} 

int Utf8ToUtf16(uint16_t* pUTF16, char *pUTF8)
{
		int len = 0;
		int n;
		uint16_t wstr;

		while (*pUTF8) {
				n = charUFT8toUTF16(pUTF16, pUTF8);
				if (n == 0) 
						return -1;
				
				pUTF8 += n;
				len++;
				pUTF16++;
		}
		return len; 
}

void textout(struct SHEET *sht, int x, int y, int c, char *s)
{
	io_cli();
	
	bdfSetDrawingAreaSize(sht->bxsize, sht->bysize);
	
	bdfFB = sht;
	bdfColor = c;
	
	int cx = x;
	int cy = y;
	
	uint16_t utf16[2048];
	int len = Utf8ToUtf16(utf16,s);
	
	for(int i = 0; i < len; i++) {
		int ch = unicode[utf16[i]];
		//if(ch > 0xff) continue;
		BDF_FONT *b = ch > 0xff ? bdfj : bdf;
		bdfPrintCharacter(b,cx,cy,ch);
		cx = bdfGetDrawingCurrentX();
		cy = bdfGetDrawingCurrentY();
	}
	
	io_sti();
}

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
	int c, *tbc_new, *tbc_old, *buf = sht->buf;
	if (act != 0) {
		tbc_new = tca1;
		tbc_old = tca2;
	} else {
		tbc_new = tca2;
		tbc_old = tca1;
	}
	for (y = 3; y <= 20; y++) {
		for (x = 3; x <= xsize - 4; x++) {
			if(!(y >= 5 && y < 5 + 14 && x >= xsize - 21 && x < xsize - 21 + 16)) {
				c = buf[y * xsize + x];
				for(int i = 0; i < 256; i++) if(c == tbc_old[i]) c = tbc_new[i];
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
	
	boxfill(sht_win, 0, 0, sht_win->bxsize - 0, sht_win->bysize - 0, 0x000000);
	boxfill(sht_win, 0, 0, sht_win->bxsize - 1, sht_win->bysize - 1, 0xcccccc);
	boxfill(sht_win, 1, 1, sht_win->bxsize - 1, sht_win->bysize - 1, 0x888888);
	boxfill(sht_win, 1, 1, sht_win->bxsize - 2, sht_win->bysize - 2, 0xffffff);
	boxfill(sht_win, 2, 2, sht_win->bxsize - 2, sht_win->bysize - 2, 0xcccccc);
	//boxfill(sht_win, 3, 3, sht_win->bxsize - 3, 21, 0x000088);
	
	for (x = 0; x < sht_win->bxsize-6; x++) {
		int nx = (int)((float)x / (float)(sht_win->bxsize-6) * 256.0f);
		for (y = 3; y < 21; y++) {
			sht_win->buf[y * sht_win->bxsize + (3 + x)] = tca1[nx];
		}
	}
	
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
	
	/*io_cli();
	
	bdfSetDrawingAreaSize(sht_win->bxsize, sht_win->bysize);
	
	bdfFB = sht_win;
	bdfColor = 0xffffff;
	
	bdfPrintString(bdf,6,6,title);
	
	io_sti();*/
	textout(sht_win,6,6,0xffffff,title);
	
	sheet_refresh(sht_win,0,0,sht_win->bxsize,sht_win->bysize);
}
