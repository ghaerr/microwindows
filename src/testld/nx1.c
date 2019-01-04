#include <stdio.h>

extern void x1();

void nx1()
{
	printf("nx1 calling x1\n");
	x1();
}

#if 0
#include <dlfcn.h>
void *dynamic_load_x1()
{
	static int init = 0;
	void *s;

	if (!init)
	{
		s = dlopen("./realx1.so", RTLD_DEEPBIND|RTLD_NOW);
		init = 1;
	}

	int (*f)() = dlsym(s, "x1");
	printf("%x, %x\n", f, x1);
	printf("nx1 calling x1\n");
	f();
	//x1();
}
#endif
