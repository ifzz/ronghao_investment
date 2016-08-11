#include <stdio.h>
#include <stdlib.h>

void * __dso_handle = 0;

void  E15_dll_main()
{
	printf("build at: %s\n",E15_PROJECT_BUILD_TIME);
	printf("author: %s\n",E15_PROJECT_AUTHOR);
	printf("version: %s\n",E15_PROJECT_VERSION);
	exit(0);
}

