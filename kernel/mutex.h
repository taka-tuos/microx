#ifndef __MUTEX_H__
#define __MUTEX_H__

typedef struct MUTEX {
	int mutex;
} MUTEX, *PMUTEX;

void init_mutex(MUTEX *mutex);
int get_mutex(MUTEX *mutex);
int free_mutex(MUTEX *mutex);

#endif
