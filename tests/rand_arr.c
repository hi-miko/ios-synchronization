#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h> // mmap + macros

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

int get_nonempty_queue(sem_t *sem_arr[], int num_arr[], int len)
{
	if(len <= 0)
	{
		return -1;
	}

	int index = num_arr[get_rand_num(0, len-1)];
	int sem_val;

	if(sem_getvalue(sem_arr[index], &sem_val) == -1)
	{
		perror("Error");
		exit(1);
	}

	if(sem_val < 0)
	{
		return index;
	}
	
	int new_num_arr[SEM_CNT];

	int j = 0;
	for(int i = 0; i < len; i++)
	{
		if(num_arr[i] != index)
		{
			new_num_arr[j++] = num_arr[i];
		}
	}

	return get_nonempty_queue(sem_arr, new_num_arr, len-1);
}

int create_process(sem_t *sem_arr[], int wait)
{
	int pid = fork();

	if(pid == 0)
	{
		sem_wait(sem_arr[wait]);

		sem_post(sem_arr[0]);
		exit(0);
	}
	else if(pid > 0)
	{
		return pid;
	}
	else
	{
		fprintf(stderr, "Error: creating process\n");
		exit(1);
	}
}

void create_shared_memory(sem_t *(*sem_arr)[])
{
	// semaphore creation
	for(int i = 0; i < SEM_CNT; i++)
	{
		if(((*sem_arr)[i] = (sem_t *)mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0)) == (sem_t *)-1)
		{
			perror("Error");
			exit(1);
		}
	}
}

int main(void)
{
	sem_t *sem_arr[SEM_CNT];

	for(int i = 0; i < SEM_CNT; i++)
	{
		sem_arr[i] = NULL;
	}

	create_shared_memory(&sem_arr);
	
	/* for(int i = 0; i < SEM_CNT; i++) */
	/* { */
	/* 	if(sem_arr[i] == NULL) */
	/* 	{ */
	/* 		fprintf(stderr, "Error: Non existant semaphore trying to be initialized\n"); */
	/* 		exit(1); */
	/* 	} */

	/* 	if(sem_init(sem_arr[i], 1, 3) == -1) */
	/* 	{ */
	/* 		perror("Error"); */
	/* 		return 1; */
	/* 	} */
	/* } */
	if(sem_init(sem_arr[0], 1, 3) == -1)
	{
		perror("Error");
		return 1;
	}

	if(sem_init(sem_arr[1], 1, 0) == -1)
	{
		perror("Error");
		return 1;
	}

	if(sem_init(sem_arr[2], 1, 0) == -1)
	{
		perror("Error");
		return 1;
	}
	
	create_process(sem_arr, 2);
	create_process(sem_arr, 2);
	create_process(sem_arr, 2);
	create_process(sem_arr, 2);
	create_process(sem_arr, 2);
	int sem1_val, sem2_val, sem3_val;

	sem_getvalue(sem_arr[0], &sem1_val);
	sem_getvalue(sem_arr[1], &sem2_val);
	sem_getvalue(sem_arr[2], &sem3_val);

	printf("sem1 val: %d\nsem2 val: %d\nsem3 val: %d\n", sem1_val, sem2_val, sem3_val);

	/* int num_arr[] = {0, 1, 2}; */
	/* int nonempty = get_nonempty_queue(sem_arr, num_arr, 3); */
	/* printf("Nonempty gotten: %d\n", nonempty); */

	return 0;
	/* create_process(sem_arr, 3); */
}
