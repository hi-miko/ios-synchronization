#include <stdlib.h>
#include <stdio.h>
#include <ctype.h> //call to isdigit
#include <pthread.h>
#include <string.h>

//TODO before finishing add werror to makefile
void help_menu()
{
	printf("Usage:\n");
	printf("\t./proj2 NZ NU TZ TU F\n\n");
	printf("Arguments:\n");
	printf("\tNZ: number of customers\n");
	printf("\tNU: number of clerks\n");
	printf("\tTZ: maximum time in milliseconds that a customer waits after creation before entering the post office ");
	printf("\n\t\t(or leaving without being served). 0<=TZ<=10000\n");
	printf("\tTU: maximum length of a clerk's break in milliseconds. 0<=TU<=100\n");
	printf("\tF: maximum time in milliseconds that the post office is closed to new customers. 0<=F<=10000\n");
}

void is_num(char *str)
{
	for(int j = 0; j < strlen(str); j++)
	{
		if(!isdigit(str[j]))
		{
			printf("Error: arguments can only be numbers\n");
			exit(1);
		}
	}
}

void check_args(int argc, char **argv)
{
	for(int i = 1; i < argc; i++)
	{
		is_num(argv[i]);
	}

	//atoi will only be run on numbered strings
	int TZ = atoi(argv[3]);
	if(TZ < 0 || TZ > 10000)
	{
		printf("Error: argument 'TZ' has to be in the range of <0, 10000>\n");
		exit(1);
	}

	int TU = atoi(argv[4]);
	if(TU < 0 || TU > 100)
	{
		printf("Error: argument 'TU' has to be in the range of <0, 100>\n");
		exit(1);
	}

	int F = atoi(argv[5]);
	if(F < 0 || F > 10000)
	{
		printf("Error: argument 'F' has to be in the range of <0, 100>\n");
		exit(1);
	}
}

int main(int argc, char **argv)
{
	if(strcmp(argv[1], "-h") == 0)
	{
		help_menu();
		return 0;
	}
	else if(strcmp(argv[1], "--help") == 0)
	{
		help_menu();
		return 0;
	}
	
	if(argc != 6)
	{
		printf("Error: Wrong usage (see %s (-h | --help)) for help\n", argv[0]);
		return 1;
	}

	check_args(argc, argv);

	int nz = atoi(argv[1]), nu = atoi(argv[2]), tz = atoi(argv[3]), tu = atoi(argv[4]), f = atoi(argv[5]);

	printf("NZ: %d, NU: %d, TZ: %d, TU: %d, F: %d\n", nz, nu, tz, tu, f);
}
