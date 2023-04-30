#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h> // mmap + macros
#include <time.h>

#define SEM_CNT 3

int get_rand_num(int min, int max)
{
	max++;
	int difference = max - min;
	if(difference <= 0)
	{
		fprintf(stderr, "Error: min has to be exclusively bigger than max\n");
		exit(1);
	}

	int ret = rand() % difference;
	ret += min;

	return ret;
}

int main(void)
{
	srand(time(NULL));

	int min = 0;
	int max = 2;
	int rand_num;

	for(unsigned int i = 0; i < 1000000000; i++)
	{
		rand_num = get_rand_num(min, max);

		if(rand_num < min || rand_num > max)
		{
			printf("Error: random function failed\n");
		}
	}

	printf("finished!\n");

	return 0;
}
