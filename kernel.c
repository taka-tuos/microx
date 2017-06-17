#include "multiboot.h"
#include "ata.h"
#include "pci.h"
#include "memory.h"
#include "console.h"
#include "bootpack.h"
#include "ff.h"

struct CONSOLE *cons;

FATFS *ata;

void _kernel_entry(UINT32 magic, MULTIBOOT_INFO *info)
{
	struct FIFO32 fifo, keycmd;
	int fifobuf[128], keycmd_buf[32];
	struct TASK *task_a, *task;
	
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
	memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
	memman_free(memman, 0x00400000, memtotal - 0x00400000);
	
	/*task_a = task_init(memman);
	fifo.task = task_a;
	task_run(task_a, 1, 2);*/
	
	cons = (struct CONSOLE *) memman_alloc_4k(memman, sizeof(struct CONSOLE));
	
	char s[60];
	
	sprintf(s, "CS:%08x SS:%08x\n", read_cs(), read_ss());
	cons_putstr0(cons, s);
	
	char b[512];
	
	find_pci_device();
	//show_all_pci_device();
	init_ata_disk_driver();
	
	FATFS fs;
	ata = &fs;
	
	printk("ATA DONE\n");
	
	int r = f_mount(&fs,"",1);
	
	printk("mount %d\n", r);
	
	DIR dir;
	
	r = f_opendir(&dir,"");
	printk("opendir %d\n", r);
	
	while(1) {
		FILINFO fil;
		volatile struct DFDATE *d;
		int yer,mon,day,hou,min,sec;
		r = f_readdir(&dir,&fil);
		printk("readdir %d\n", r);
		int i;
		//for(i = 0; i < 12; i++) printk("%02x ", fil.fname[i]);
		if(fil.fname[0] == '\0') break;
		
		d = (struct DFDATE *)&(fil.fdate);
		
		/*printk("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9lu  %s\r\n", 
					(fil.fattrib & AM_DIR) ? 'D' : '-',
					(fil.fattrib & AM_RDO) ? 'R' : '-',
					(fil.fattrib & AM_HID) ? 'H' : '-',
					(fil.fattrib & AM_SYS) ? 'S' : '-',
					(fil.fattrib & AM_ARC) ? 'A' : '-',
					(fil.fdate >> 9) + 1980, (fil.fdate >> 5) & 15, fil.fdate & 31,
					(fil.ftime >> 11), (fil.ftime >> 5) & 63,
					fil.fsize, &(fil.fname[0]));*/
	}
}
