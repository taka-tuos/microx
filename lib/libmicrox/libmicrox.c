#include <libmicrox.h>

void mx32api_call(int *p)
{
	asm volatile ("movl %0,%%eax\n\t" :: "r"(p));
	asm volatile ("int $0x40\n\t");
}
