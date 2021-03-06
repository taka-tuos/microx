#ifndef __CONSOLE__
#define __CONSOLE__

extern int MAX_Y;
extern int MAX_X;
#define	MAX_XY				(MAX_X*MAX_Y)

struct CONSOLE {
	int cur_x, cur_y;
	int col_f, col_b;
};

void cons_initalize(struct CONSOLE *cons);
void cons_putchar(struct CONSOLE *cons, int chr, char move);
void cons_newline(struct CONSOLE *cons);
void cons_putstr0(struct CONSOLE *cons, char *s);
void cons_putstr1(struct CONSOLE *cons, char *s, int l);

#endif
