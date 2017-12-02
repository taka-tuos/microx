#ifndef __CONSOLE__
#define __CONSOLE__

#define	VRAM_TEXTMODE		0x000B8000
#define	DEF_COLOR_BLACK		0x00
#define	DEF_COLOR_WHITE		0x07
#define	MAX_Y				30
#define	MAX_X				80
#define	MAX_XY				(80*30)

struct CONSOLE {
	int cur_x, cur_y;
};

void cons_putchar(struct CONSOLE *cons, int chr, char move);
void cons_newline(struct CONSOLE *cons);
void cons_putstr0(struct CONSOLE *cons, char *s);
void cons_putstr1(struct CONSOLE *cons, char *s, int l);

#define COL8_000000		0
#define COL8_FFFFFF		7

#endif
