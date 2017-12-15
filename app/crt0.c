#include <sys/api.h>

extern int main(void);

void MicroMain(void)
{
	x32_KeyboardEnable();
	
	main();
	
	x32_Exit();
}
