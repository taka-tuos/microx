#include <string.h>
#include <stdint.h>
#include "wrapper.h"

#ifndef __ATA_H__
#define __ATA_H__

#ifndef bool
#define true 1
#define false 0
typedef int bool;
#endif

typedef uint16_t sector_t;

struct block_driver_operations {
	char name[32]; // driver name.
	int (*open)(void);
	int (*close)(void);
	int (*read)(int device, uint32_t sector, 
		    sector_t *buf, size_t buf_size);
	int (*write)(int device, uint32_t sector,
		     sector_t *buf, size_t buf_size);
	int (*ioctl)(void); // temporary definition.
	int (*scattered_io)(void); // temporary definition.
};

extern struct block_driver_operations ata_dev;

bool init_ata_disk_driver(void);

#endif
