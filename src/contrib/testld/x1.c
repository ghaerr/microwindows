#include <stdio.h>
extern void nx1();

void x1()
{
	printf("x1 calling nx1\n");
	nx1();
}
