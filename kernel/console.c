#include "console.h"
#include "asmfuncs.h"

#include "multiboot.h"

#include "vga_font.h"

extern MULTIBOOT_INFO *mboot_info;

int MAX_Y, MAX_X;

const unsigned int table_rgb16[16 * 3] = {
	0x000000,
	0x0000aa,
	0x00aa00,
	0x00aaaa,
	0xaa0000,
	0xaa00aa,
	0xaaaa00,
	0xaaaaaa,
	0x555555,
	0x5555ff,
	0x55ff55,
	0x55ffff,
	0xff5555,
	0xff55ff,
	0xffff55,
	0xffffff,
};

void cons_initalize(struct CONSOLE *cons)
{
	cons->cur_x = 0;
	cons->cur_y = 0;
	cons->col_f = 7;
	cons->col_b = 0;
}

void cons_putchar_raw(int x, int y, int f, int b, int c)
{
	//unsigned char *p = (unsigned char *)VRAM + (x + y * MAX_X) * 2;
	
	//p[0] = c;
	//p[1] = (f & 15) | ((b & 15) << 4);
	//fb[y*MAX_X+x].ch = c;
	//fb[y*MAX_X+x].attr = (f & 15) | ((b & 15) << 4);
	
	unsigned int *fbg = (unsigned int *)mboot_info->framebuffer_addr[0];
	
	int fc = table_rgb16[f];
	int bc = table_rgb16[b];
	for(int i = 0; i < 16; i++) {
		unsigned char d = vga_font[c*16+i];
		for(int j = 0; j < 8; j++) {
			fbg[(y*16+i)*(mboot_info->framebuffer_width)+(x*8+j)] = (d & (0x80 >> j)) ? fc : bc;
		}
	}
}

void cons_putchar(struct CONSOLE *cons, int chr, char move)
{
	if (chr == 0x09) {	/* タブ */
		for (;;) {
			cons_putchar_raw(cons->cur_x, cons->cur_y, cons->col_f, cons->col_b, ' ');
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
		cons_putchar_raw(cons->cur_x, cons->cur_y, cons->col_f, cons->col_b, chr);
		if (move != 0) {
			/* moveが0のときはカーソルを進めない */
			cons->cur_x++;
			if (cons->cur_x == MAX_X) {
				cons_newline(cons);
			}
		}
	}
	return;
}

void cons_newline(struct CONSOLE *cons)
{
	int x, y;
	unsigned int *fbg = (unsigned int *)mboot_info->framebuffer_addr[0];
	if (cons->cur_y < MAX_Y - 1) {
		cons->cur_y++; /* 次の行へ */
	} else {
		/* スクロール */
		//memcpy(VRAM, VRAM+MAX_X*2, ((MAX_X-1)*MAX_Y)*2);
		//memcpy(fb, fb+MAX_X*2, ((MAX_X-1)*MAX_Y)*sizeof(cons_char));
		memcpy(fbg, fbg+MAX_X*8*16, ((MAX_X-1)*MAX_Y)*8*16*4);
		for (y = MAX_Y - 1; y < MAX_Y; y++) {
			for (x = 0; x < MAX_X; x++) {
				//memset(VRAM+(MAX_X*y)*2, 0, MAX_X*2);
				//memset(fb+(MAX_X*y)*2, 0, MAX_X*sizeof(cons_char));
				memset(fbg+(MAX_X*y)*8*16, 0, MAX_X*8*16*4);
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
