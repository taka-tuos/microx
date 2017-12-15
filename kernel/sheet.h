#ifndef __GAME_H__
#define __GAME_H__

#include <stdlib.h>

#define MAX_SHEETS		256
struct SHEET {
	unsigned int *buf;
	int bxsize, bysize, vx0, vy0, col_inv, height, flags;
	int flag2;
	int act;
	struct SHTCTL *ctl;
	int task;
	void *ext;
};
struct SHTCTL {
	unsigned int *vram, *map;
	int xsize, ysize, top;
	struct SHEET *sheets[MAX_SHEETS];
	struct SHEET sheets0[MAX_SHEETS];
};

struct SHTCTL *shtctl_init(unsigned int *vram, int xsize, int ysize);
struct SHEET *sheet_alloc(struct SHTCTL *ctl);
void sheet_setbuf(struct SHEET *sht, unsigned int *buf, int xsize, int ysize, int col_inv);
void sheet_updown(struct SHEET *sht, int height);
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1);
void sheet_slide(struct SHEET *sht, int vx0, int vy0);
void sheet_free(struct SHEET *sht);

#endif
