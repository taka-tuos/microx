#include "console.h"
#include "asmfuncs.h"

void cons_putchar_hw(int x, int y, unsigned char f, unsigned char b, int c)
{
	unsigned short	*vram_textmode;
	unsigned short	color;
	
	vram_textmode	= ( unsigned short *)VRAM_TEXTMODE;
	
	color = ( b << 4 ) | ( f & 0x0F );
	
	vram_textmode	+= x + y * MAX_X;
	
	*vram_textmode	= ( color << 8 ) | c;
	
	int postion = (x + 1) + y * MAX_X;
	
	io_out8(0x3d4, 0xf);
	io_out8(0x3d5, postion & 0xff);
	
	io_out8(0x3d4, 0xe);
	io_out8(0x3d5, postion >> 8);
}

void cons_putchar(struct CONSOLE *cons, int chr, char move)
{
	if (chr == 0x09) {	/* �^�u */
		for (;;) {
			cons_putchar_hw(cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, ' ');
			cons->cur_x++;
			if (cons->cur_x == 80) {
				cons_newline(cons);
			}
			if (((cons->cur_x - 1) & 3) == 0) {
				break;	/* 32�Ŋ���؂ꂽ��break */
			}
		}
	} else if (chr == 0x0a) {	/* ���s */
		cons_newline(cons);
	} else if (chr == 0x0d) {	/* ���A */
		/* �Ƃ肠�����Ȃɂ����Ȃ� */
	} else {	/* ���ʂ̕��� */
		cons_putchar_hw(cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, chr);
		if (move != 0) {
			/* move��0�̂Ƃ��̓J�[�\����i�߂Ȃ� */
			cons->cur_x++;
			if (cons->cur_x == 80) {
				cons_newline(cons);
			}
		}
	}
	return;
}

void cons_newline(struct CONSOLE *cons)
{
	int x, y;
	if (cons->cur_y < 25 - 1) {
		cons->cur_y++; /* ���̍s�� */
	} else {
		/* �X�N���[�� */
		for (y = 0; y < MAX_Y - 1; y++) {
			for (x = 0; x < MAX_X; x++) {
				((unsigned short *)VRAM_TEXTMODE)[x + y * MAX_X] = ((unsigned short *)VRAM_TEXTMODE)[x + (y + 1) * MAX_X];
			}
		}
		for (y = MAX_Y - 1; y < MAX_Y; y++) {
			for (x = 0; x < MAX_X; x++) {
				((unsigned short *)VRAM_TEXTMODE)[x + y * MAX_X] = COL8_000000;
			}
		}
	}
	cons->cur_x = 0;
	return;
}

void cons_putstr0(struct CONSOLE *cons, char *s)
{
	for (; *s != 0; s++) {
		cons_putchar(cons, *s, 1);
	}
	return;
}

void cons_putstr1(struct CONSOLE *cons, char *s, int l)
{
	int i;
	for (i = 0; i < l; i++) {
		cons_putchar(cons, s[i], 1);
	}
	return;
}