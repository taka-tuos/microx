/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "diskio.h"		/* FatFs lower layer API */
#include "ata.h"

enum {
	cmos_address = 0x70,
	cmos_data    = 0x71
};

int inited = -1;

int ldrv[2] = { -1, -1 };

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	if(ldrv[pdrv] >= 0) return 0;

	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	if(pdrv >= 2) return STA_NOINIT;
	
	if(inited < 0) inited = ata_dev.open();
	
	if(inited <= 0) return STA_NOINIT;
	
	if(inited & 0x01) {
		ldrv[0] = 0;
		if(inited & 0x02) {
			ldrv[1] = 1;
		}
	} else if(inited & 0x02) {
		ldrv[0] = 1;
	}
	
	if(ldrv[pdrv] < 0) return STA_NOINIT;
	
	return 0;
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
	if(pdrv >= 2 || ldrv[pdrv] < 0) return RES_PARERR;
	int i;
	//for(i = 0; i < count; i++) {
		if(ata_dev.read(ldrv[pdrv],sector,buff,256*count) < 0) return RES_PARERR;
	//}

	return RES_OK;
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
	if(pdrv >= 2 || ldrv[pdrv] < 0) return RES_PARERR;
	int i;
	//for(i = 0; i < count; i++) {
		if(ata_dev.write(ldrv[pdrv],sector,buff,256*count) < 0) return RES_PARERR;
	//}

	return RES_OK;
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
