#include "multiboot.h"
#include "ata.h"
#include "pci.h"
#include "memory.h"
#include "console.h"
#include "bootpack.h"
#include "ff.h"

#include <libmicrox.h>

#define DUMP(v) printk(#v " = %08x\n", v);

struct CONSOLE *cons;

FATFS *ata;

void fork_exit(void)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct TASK *task = task_now();
	struct FIFO32 *fifo = (struct FIFO32 *) *((int *) 0x0fec);
	io_cli();
	fifo32_put(fifo, task - taskctl->tasks0 + 1024);	/* 1024〜2023 */
	io_sti();
	for (;;) {
		task_sleep(task);
	}
}

void fork_task(char *path)
{
	int i, segsiz, datsiz, esp, dathrb, appsiz;
	char *p, *q;
	struct TASK *task = task_now();
	UINT dmy;
	FIL *fd;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	
	fd = (FIL *) memman_alloc_4k(memman, sizeof(FIL));
	
	int r = f_open(fd,path,FA_READ);
	
	if(r == FR_OK) {
		appsiz = f_size(fd);
		p = (char *) memman_alloc_4k(memman, appsiz);
		f_read(fd, p, appsiz, &dmy);
		
		if (appsiz >= 36 && strncmp(p + 4, "Hari", 4) == 0 && *p == 0x00) {
			segsiz = *((int *) (p + 0x0000));
			esp    = *((int *) (p + 0x000c));
			datsiz = *((int *) (p + 0x0010));
			dathrb = *((int *) (p + 0x0014));
			q = (char *) memman_alloc_4k(memman, segsiz);
			task->ds_base = (int) q;
			
			//DUMP(segsiz);
			//DUMP(esp);
			//DUMP(datsiz);
			//DUMP(dathrb);
			
			set_segmdesc(task->ldt + 0, appsiz + 1024 - 1, (int) p, AR_CODE32_ER + 0x60);
			set_segmdesc(task->ldt + 1, segsiz + 1024 - 1, (int) q, AR_DATA32_RW + 0x60);
			for (i = 0; i < datsiz; i++) {
				q[esp + i] = p[dathrb + i];
			}
			
			start_app(0x1b, 0 * 8 + 4, esp, 1 * 8 + 4, &(task->tss.esp0));
			memman_free_4k(memman, (int) q, segsiz);
		} else {
			printk("%s is not executable file\n", path);
		}
		memman_free_4k(memman, (int) p, appsiz);
	} else {
		printk("%s not found\n", path);
	}
	
	fork_exit();
}

struct TASK *fork(char *path)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct TASK *forktask = task_alloc();
	int *fork_fifo = (int *) memman_alloc_4k(memman, 128 * 4);
	forktask->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 8;
	forktask->tss.eip = (int) &fork_task;
	forktask->tss.es = 1 * 8;
	forktask->tss.cs = 2 * 8;
	forktask->tss.ss = 1 * 8;
	forktask->tss.ds = 1 * 8;
	forktask->tss.fs = 1 * 8;
	forktask->tss.gs = 1 * 8;
	*((char **) (forktask->tss.esp + 4)) = path;
	task_run(forktask, 1, 2);
	fifo32_init(&(forktask->fifo), 128, fork_fifo, forktask);
}

const unsigned char table_rgb[16 * 3] = {
	0x00, 0x00, 0x00,
	0x00, 0x00, 0xaa,
	0x00, 0xaa, 0x00,
	0x00, 0xaa, 0xaa,
	0xaa, 0x00, 0x00,
	0xaa, 0x00, 0xaa,
	0xaa, 0xaa, 0x00,
	0xaa, 0xaa, 0xaa,
	0x55, 0x55, 0x55,
	0x55, 0x55, 0xff,
	0x55, 0xff, 0x55,
	0x55, 0xff, 0xff,
	0xff, 0x55, 0x55,
	0xff, 0x55, 0xff,
	0xff, 0xff, 0x55,
	0xff, 0xff, 0xff,
};

void kill_fork(struct TASK *task)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	task_sleep(task);
	memman_free_4k(memman, task->cons_stack, 64 * 1024);
	memman_free_4k(memman, (int) task->fifo.buf, 128 * 4);
	io_cli();
	task->flags = 0;
	if (taskctl->task_fpu == task) {
		taskctl->task_fpu = 0;
	}
	io_sti();
	task->flags = 0;
	return;
}


void set_palette(int start, int end, unsigned char *rgb)
{
	int i, eflags;
	io_cli();
	io_out8(0x03c8, start);
	for (i = start; i <= end; i++) {
		io_out8(0x03c9, rgb[0] / 4);
		io_out8(0x03c9, rgb[1] / 4);
		io_out8(0x03c9, rgb[2] / 4);
		rgb += 3;
	}
	io_sti();
	return;
}

void _kernel_entry(UINT32 magic, MULTIBOOT_INFO *info)
{
	struct FIFO32 fifo, keycmd;
	int fifobuf[128], keycmd_buf[32];
	struct TASK *task_a, *task;
	
	FATFS fs;
	
	init_gdtidt(info);
	init_pic();
	io_sti(); /* IDT/PICの初期化が終わったのでCPUの割り込み禁止を解除 */
	fifo32_init(&fifo, 128, fifobuf, 0);
	*((int *) 0x0fec) = (int) &fifo;
	init_pit();
	init_keyboard(&fifo, 256);
	io_out8(PIC0_IMR, 0xf8); /* PITとPIC1とキーボードを許可(11111000) */
	io_out8(PIC1_IMR, 0xff); /* 割り込み禁止(11111111) */
	fifo32_init(&keycmd, 32, keycmd_buf, 0);
	
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	int memtotal = info->mem_upper;
	memman_init(memman);
	memman_free(memman, 0x00400000, memtotal - 0x00400000);
	
	task_a = task_init(memman);
	fifo.task = task_a;
	task_run(task_a, 1, 2);
	
	cons = (struct CONSOLE *) memman_alloc_4k(memman, sizeof(struct CONSOLE));
	
	set_palette(0,16,table_rgb);
	
	io_out16(0x03c4,0x0100);
	
	io_out8(0x03c2,0xe3);
	io_out8(0x03c3,0x01);
	
	int i;
	int x3c4_data[] = { 0x01, 0x0f, 0x00, 0x06 };
	int x3d4_dataA[] = { 0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e };
	int x3d4_dataB[] = { 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	int x3d4_dataC[] = { 0xea, 0x8c, 0xdf, 0x28, 0x00, 0xe7, 0x04, 0xe3, 0xff };
	int x3ce_data[] = { 0x00, 0x0f, 0x00, 0x00, 0x00, 0x03, 0x05, 0x00, 0xff };
	int x3c0_dataA[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
	int x3c0_dataB[] = { 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
	int x3c0_dataC[] = { 0x01, 0x00, 0x0f, 0x00, 0x00 };
	
	for(i = 0x01; i <= 0x04; i++) io_out16(0x03c4,i | (x3c4_data[i-1] << 8));
	
	io_out16(0x03c4,0x0300);
	
	io_out16(0x03d4,0x2011);
	
	for(i = 0x00; i <= 0x07; i++) io_out16(0x03d4,i | (x3d4_dataA[i] << 8));
	for(i = 0x08; i <= 0x0f; i++) io_out16(0x03d4,i | (x3d4_dataB[i-0x08] << 8));
	for(i = 0x10; i <= 0x18; i++) io_out16(0x03d4,i | (x3d4_dataC[i-0x10] << 8));
	
	for(i = 0x00; i <= 0x08; i++) io_out16(0x03ce,i | (x3ce_data[i] << 8));
	
	for(i = 0x00; i <= 0x07; i++) {
		int dmy = io_in8(0x3da);
		io_out8(0x3c0, i|0x20);
		io_out8(0x3c0, x3c0_dataA[i]);
	}
	
	for(i = 0x08; i <= 0x0f; i++) {
		int dmy = io_in8(0x3da);
		io_out8(0x3c0, i|0x20);
		io_out8(0x3c0, x3c0_dataB[i-0x08]);
	}
	
	for(i = 0x10; i <= 0x14; i++) {
		int dmy = io_in8(0x3da);
		io_out8(0x3c0, i|0x20);
		io_out8(0x3c0, x3c0_dataC[i-0x10]);
	}
	
	io_out8(0x3c6, 0xff);
	io_out16(0x3ce, 0x0305);
	
	memset(0xa0000,0,640*480/8);
	
	find_pci_device();
	init_ata_disk_driver();
	
	ata = &fs;
	
	f_mount(&fs,"",1);
	
	fork("init.eim");
	
	while(1)
	{
		if(fifo32_status(&fifo) != 0) {
			i = fifo32_get(&fifo);
			if (1024 <= i && i <= 2023) {
				kill_fork(taskctl->tasks0 + (i - 1024));
			}
		} else {
			task_sleep(task_a);
		}
	}
}

int *microx_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax)
{
	struct TASK *task = task_now();
	struct TASK *task_cons;
	int ds_base = task->ds_base;
	int *reg = &eax + 1;
	
	int *p = (int *)(ds_base + eax);
	
	if(p[0] == mx32api_putchar) {
		printk("%c",p[1] & 0xff);
	}
	
	return 0;
}
