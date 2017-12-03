#include "mutex.h"
#include "bootpack.h"

void init_mutex(MUTEX *mutex)
{
	mutex->mutex = 1;
	return;
}

int get_mutex(MUTEX *mutex)
{
	int l;
	io_cli();
	if(mutex->mutex==1) {
		mutex->mutex--;
		l=0;
	} else {
		l=1;
	}
	io_sti();
	return l;
}

int free_mutex(MUTEX *mutex)
{
	io_cli();
	if(mutex->mutex==0) {
		mutex->mutex++;
	}
	io_sti();
	return 0;
}
