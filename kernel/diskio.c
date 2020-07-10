/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "diskio.h"		/* FatFs lower layer API */
//#include "ata.h"
#include "ide.h"
#include "wrapper.h"

extern int enter_diskio(int op, int drv, int secs, int lba, int addr);
extern void wait_diskio(int id);

enum {
	cmos_address = 0x70,
	cmos_data    = 0x71
};

int inited = -1;

int ldrv[4] = { -1, -1, -1, -1 };

extern int package[16];

int atomic = 0;

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	if(pdrv < 4 && ldrv[pdrv] >= 0) return 0;

	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	if(pdrv >= 4) return STA_NOINIT;
	
	while(atomic != 0);
	atomic++;
	
	if(inited < 0) {
		//cli();
		int r = ide_initialize(0x1F0, 0x3F6, 0x170, 0x376, 0x000);
		//sti();
		inited = 0;
		
		for(int i=0; i<4; i++) {
			if(r & (1 << i)) {
				for(int j=0; j<4; j++) {
					if(ldrv[j] < 0) {
						ldrv[j] = i;
						break;
					}
				}
			}
		}
		
		printk("ldrv = ");
		printk("%d, ",ldrv[0]);
		printk("%d, ",ldrv[1]);
		printk("%d, ",ldrv[2]);
		printk("%d\n",ldrv[3]);
	}
	
	atomic--;
	
	if(ldrv[pdrv] >= 0) return 0;
	
	/*if(inited & 0x01) {
		ldrv[0] = 0;
		if(inited & 0x02) {
			ldrv[1] = 1;
		}
	} else if(inited & 0x02) {
		ldrv[0] = 1;
	}
	
	if(ldrv[pdrv] < 0) return STA_NOINIT;*/
	
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	//if(pdrv >= 2 || ldrv[pdrv] < 0) return RES_PARERR;
	int i;
	//for(i = 0; i < count; i++) {
		//if(ata_dev.read(ldrv[pdrv],sector,buff,256*count) < 0) return RES_PARERR;
	//}
	
	if(pdrv >= 4) return RES_PARERR;
	if(ldrv[pdrv] < 0) return RES_NOTRDY;
	
	package[0] = 0;
	
	//cli();
	while(atomic != 0);
	atomic++;
	//printk("ENTER READ ATOMIC %08x\n",sector+(DWORD)buff);
	ide_read_sectors(ldrv[pdrv],count,sector,0,(unsigned int)buff);
	//printk("LEAVE READ ATOMIC %08x\n",sector+(DWORD)buff);
	atomic--;
	//sti();
	//int id = enter_diskio(0,ldrv[pdrv],count,sector,(unsigned int)buff);
	//wait_diskio(id);
	
	if(package[0] == 1) return RES_NOTRDY;
	if(package[0] == 2) return RES_PARERR;

	return 0;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	//if(pdrv >= 2 || ldrv[pdrv] < 0) return RES_PARERR;
	int i;
	//for(i = 0; i < count; i++) {
		//if(ata_dev.write(ldrv[pdrv],sector,buff,256*count) < 0) return RES_PARERR;
	//}
	
	if(pdrv >= 4) return RES_PARERR;
	if(ldrv[pdrv] < 0) return RES_NOTRDY;
	
	package[0] = 0;
	
	while(atomic != 0);
	atomic++;
	//printk("ENTER WRITE ATOMIC %08x\n",sector+(DWORD)buff);
	ide_write_sectors(ldrv[pdrv],count,sector,0,(unsigned int)buff);
	//printk("LEAVE WRITE ATOMIC %08x\n",sector+(DWORD)buff);
	atomic--;
	
	if(package[0] == 1) return RES_NOTRDY;
	if(package[0] == 2) return RES_PARERR;

	return 0;
}



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	return RES_OK;
}

unsigned char __read_rtc_register(int reg) {
	outb(cmos_address, reg);
	return inb(cmos_data);
}


DWORD __bcd_to_int(
	DWORD bcd
)
{
	DWORD ret = 0;
	int i;
	for(i = 0; i < 8; i++) {
		int d;
		ret *= 10;
		d = (bcd >> ((7 - i) * 4)) & 0x0f;
		ret += d;
	}
	
	return ret;
}

DWORD get_fattime(
	void
)
{
	DWORD year,month,day,hour,minute,second;
	
	second = __bcd_to_int(__read_rtc_register(0x00));
	minute = __bcd_to_int(__read_rtc_register(0x02));
	hour = __bcd_to_int(__read_rtc_register(0x04));
	day = __bcd_to_int(__read_rtc_register(0x07));
	month = __bcd_to_int(__read_rtc_register(0x08));
	year = __bcd_to_int(__read_rtc_register(0x09));
	
	year += 2000;
	
	DWORD ret = (year << 25) | (month << 21) | (day << 16) | (hour << 11) | (minute << 5) | (second << 0);
	
	return ret;
}
