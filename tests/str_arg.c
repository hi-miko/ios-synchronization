#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

void save_to_sem_file(char *str, int argc, ...)
{
	va_list argv;
	va_start(argv, argc);

	vfprintf(stdout, str, argv);
	fflush(stdout);
}

int main(void)
{
	save_to_sem_file("Hello World!\n", 0);
}
