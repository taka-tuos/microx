#ifndef __MMU__
#define __MMU__

#define MEMMAN_FREES		4096	/* ‚±‚ê‚Å–ñ32KB */
#define MEMMAN_ADDR			0x003c0000
struct FREEINFO {	/* ‚ ‚«î•ñ */
	unsigned int addr, size;
};
struct MEMMAN {		/* ƒƒ‚ƒŠŠÇ— */
	int frees, maxfrees, lostsize, losts;
	struct FREEINFO free[MEMMAN_FREES];
};
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size);
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size);

#endif