#include <stdlib.h>
#include <stdio.h>
#include <ctype.h> //call to isdigit
#include <semaphore.h>
#include <string.h>
#include <signal.h> // C-c safeguard
#include <unistd.h> // processes
#include <errno.h>  // TODO for errno, but it might be included already somewhere
#include <sys/mman.h> // mmap + macros
#include <sys/wait.h>
#include <time.h>
#include <stdbool.h>
#include <stdarg.h> // TODO might be already included somewhere

#define SEM_CNT 3
#define MUX_CNT 6

FILE *fp = NULL;

/* Queue array
	[0] -> queue 1 sem,
	[1] -> queue 2 sem,
	[2] -> queue 3 sem
*/
sem_t *sem_arr[SEM_CNT];

/*  General mutex array
	[0] -> row1 mux,
	[1] -> row2 mux,
	[2] -> row3 mux,
	[3] -> action count mux,
	[4] -> file write mux
	[5] -> is closed mux
*/
sem_t *mux_arr[MUX_CNT];

/* Customer count mutex array
	[0] -> row1 customer value mux,
	[1] -> row2 customer value mux,
	[2] -> row3 customer value mux,
*/
sem_t *cc_mux_arr[SEM_CNT];

int *customer_cnt[SEM_CNT];

bool *is_closed = NULL;
int *action_cnt = NULL;


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

void cleanup(int signum)
{
	(void) signum;
	if(fp != NULL)
	{
		fclose(fp);
	}
}

//TODO maybe make a better version of random. There are various problems with rand and this implementation in general
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

void is_num(char *str)
{
	for(unsigned int j = 0; j < strlen(str); j++)
	{
		if(!isdigit(str[j]))
		{
			fprintf(stderr, "Error: arguments can only be numbers\n");
			exit(1);
		}
	}
}

void run_sem_fce(int (*f)(sem_t *), sem_t *semaphore)
{
	if((*f)(semaphore) == -1)
	{
		perror("Error");
		exit(1);
	}
}

void check_args(int argc, char **argv)
{
	for(int i = 1; i < argc; i++)
	{
		is_num(argv[i]);
	}

	//atoi will only be run on numbered strings
	int NZ = atoi(argv[1]);
	if(NZ < 0)
	{
		fprintf(stderr, "Error: argument 'NZ' has to be bigger bigger or equal to 0\n");
		exit(1);
	}

	int NU = atoi(argv[2]);
	if(NU <= 0)
	{
		fprintf(stderr, "Error: argument 'NU' has to be bigger than 0\n");
		exit(1);
	}

	int TZ = atoi(argv[3]);
	if(TZ < 0 || TZ > 10000)
	{
		fprintf(stderr, "Error: argument 'TZ' has to be in the range of <0, 10000>\n");
		exit(1);
	}

	int TU = atoi(argv[4]);
	if(TU < 0 || TU > 100)
	{
		fprintf(stderr, "Error: argument 'TU' has to be in the range of <0, 100>\n");
		exit(1);
	}

	int F = atoi(argv[5]);
	if(F < 0 || F > 10000)
	{
		fprintf(stderr, "Error: argument 'F' has to be in the range of <0, 100>\n");
		exit(1);
	}
}

FILE *init_file()
{
	char filename[] = "proj2.out";
	FILE *fp = NULL;
	
	fp = fopen(filename, "w");
	if(fp == NULL)
	{
		fprintf(stderr, "Error: file could not be opened\n");
		exit(1);
	}

	return fp;
}
// TODO the things I believe I still need to do:
// I probably made a lot of sleepless mistakes, but other than that it should be almost done
// Change the random function (this is going to be fun huh)
// add doxygen style comments to every function (not that hard, I dont have a lot of functions)
int get_nonempty_queue(int num_arr[], int len)
{
	if(len <= 0)
	{
		return -1;
	}
	
	// -1 because array indexes are indexed from 0
	int index = num_arr[get_rand_num(0, len-1)];

	run_sem_fce(sem_wait, cc_mux_arr[index]);
	if(*(customer_cnt[index]) > 0)
	{
		// dec customer count in a mutex
		(*(customer_cnt[index]))--;

		run_sem_fce(sem_post, cc_mux_arr[index]);
		return index;
	}
	run_sem_fce(sem_post, cc_mux_arr[index]);
	
	int new_num_arr[SEM_CNT];

	int j = 0;
	for(int i = 0; i < len; i++)
	{
		if(num_arr[i] != index)
		{
			new_num_arr[j++] = num_arr[i];
		}
	}

	return get_nonempty_queue(new_num_arr, len-1);
}

void save_to_sem_file(char *msg, int *action_num, int id, int type)
{
	run_sem_fce(sem_wait, mux_arr[3]);
	run_sem_fce(sem_wait, mux_arr[4]);

	(*action_cnt)++;

	if(id != -1)
	{
		if(type != -1)
		{
			fprintf(fp, msg, *action_num, id, type);
		}
		else
		{
			fprintf(fp, msg, *action_num, id);
		}
	}
	else
	{
		fprintf(fp, msg, *action_num);
	}

	run_sem_fce(sem_post, mux_arr[4]);
	run_sem_fce(sem_post, mux_arr[3]);
}

void customer_logic(int id, int tz)
{
	save_to_sem_file("%d: Z %d: started\n", action_cnt, id, -1);

	int rand_num = get_rand_num(0, tz);
	usleep(rand_num);

	run_sem_fce(sem_wait, mux_arr[5]);
	if(*is_closed)
	{
		run_sem_fce(sem_post, mux_arr[5]);
		save_to_sem_file("%d: Z %d: going home\n", action_cnt, id, -1);
		return;
	}

	rand_num = get_rand_num(0, 2);
	save_to_sem_file("%d: Z %d: entering office for a service %d\n", action_cnt, id, rand_num+1);
	
	// inc customer count in a mutex
	run_sem_fce(sem_wait, cc_mux_arr[rand_num]);
	(*(customer_cnt[rand_num]))++;
	run_sem_fce(sem_post, cc_mux_arr[rand_num]);

	run_sem_fce(sem_post, mux_arr[5]);

	run_sem_fce(sem_wait, sem_arr[rand_num]);

	save_to_sem_file("%d: Z %d: called by office worker\n", action_cnt, id, -1);

	rand_num = get_rand_num(0, 10);
	usleep(rand_num);

	save_to_sem_file("%d: Z %d: going home\n", action_cnt, id, -1);
}

void clerk_logic(int id, int tu)
{
	save_to_sem_file("%d: U %d: started\n", action_cnt, id, -1);
	
	int index = -1, rand_num = 0;
	int index_arr[] = {0, 1, 2};
	while(true)
	{
		//TODO change 
		fflush(stdout);
		index = get_nonempty_queue(index_arr, 3);
		if(index == -1)
		{

			run_sem_fce(sem_wait, mux_arr[5]);
			if(*is_closed == true)
			{
				run_sem_fce(sem_post, mux_arr[5]);
				save_to_sem_file("%d: U %d: going home\n", action_cnt, id, -1);
				return;
			}

			save_to_sem_file("%d: U %d: taking a break\n", action_cnt, id, -1);
			run_sem_fce(sem_post, mux_arr[5]);

			rand_num = get_rand_num(0, tu);
			usleep(rand_num);
			save_to_sem_file("%d: U %d: break finished\n", action_cnt, id, -1);

			continue;
		}

		run_sem_fce(sem_wait, mux_arr[index]);

		// indexes of types are offset by 1
		save_to_sem_file("%d: U %d: serving a service of type %d\n", action_cnt, id, index+1);

		run_sem_fce(sem_post, sem_arr[index]);

		rand_num = get_rand_num(0, 10);
		usleep(rand_num);
		save_to_sem_file("%d: U %d: service finished\n", action_cnt, id, -1);

		run_sem_fce(sem_post, mux_arr[index]);

		continue;
	}
}

int create_process(char type, int *cnt, int tz, int tu)
{
	int pid = fork();

	if(pid == 0)
	{
		srand(time(NULL) ^ getpid());
		if(type == 'Z')
		{
			customer_logic(*cnt, tz);
		}
		else if(type == 'U')
		{
			clerk_logic(*cnt, tu);
		}
		else
		{
			fprintf(stderr, "Error: unknown process type\n");
			exit(1);
		}
		
		fclose(fp);
		exit(0);
	}
	else if(pid > 0)
	{
		// main program
		*cnt = *cnt + 1;
	}
	else
	{
		fprintf(stderr, "Error: creating process\n");
		exit(1);
	}

	return pid;
}

void destroy_sems()
{
	for(int i = 0; i < SEM_CNT; i++)
	{
		if(sem_arr[i] != NULL)
		{
			run_sem_fce(sem_destroy, sem_arr[i]);
		}
	}
}

void create_shared_memory()
{
	for(int i = 0; i < SEM_CNT; i++)
	{
		// semaphore creation
		sem_arr[i] = (sem_t *)mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
		if(sem_arr[i] == MAP_FAILED)
		{
			perror("Error");
			exit(1);
		}

		// customer count creation
		customer_cnt[i] = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
		if(customer_cnt[i] == MAP_FAILED)
		{
			perror("Error");
			exit(1);
		}

		// customer count mutex creation
		cc_mux_arr[i] = (sem_t *)mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
		if(cc_mux_arr[i] == MAP_FAILED)
		{
			perror("Error");
			exit(1);
		}
	}

	// mutex creation
	for(int i = 0; i < MUX_CNT; i++)
	{
		mux_arr[i] = (sem_t *)mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
		if(mux_arr[i] == MAP_FAILED)
		{
			perror("Error");
			exit(1);
		}
	}

	// is file closed
	is_closed = (bool *)mmap(NULL, sizeof(bool), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
	if(is_closed == MAP_FAILED)
	{
		perror("Error");
		exit(1);
	}

	// count the number of actions
	action_cnt = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
	if(action_cnt == MAP_FAILED)
	{
		perror("Error");
		exit(1);
	}
}

//TODO not sure this works correctly byt we'll see, especially with perror
void delete_shared_memory()
{
	for(int i = 0; i < SEM_CNT; i++)
	{
		if(munmap(sem_arr[i], sizeof(sem_t)) == -1)
		{
			perror("Error");
			exit(1);
		}
	}

	for(int i = 0; i < SEM_CNT; i++)
	{
		if(munmap(customer_cnt[i], sizeof(int)) == -1)
		{
			perror("Error");
			exit(1);
		}
	}

	if(munmap(is_closed, sizeof(bool)) == -1)
	{
		perror("Error");
		exit(1);
	}

	if(munmap(action_cnt, sizeof(int)) == -1)
	{
		perror("Error");
		exit(1);
	}
}

int main(int argc, char **argv)
{
	// get a pseudo random seed different for each process
	srand(time(NULL) ^ getpid());

	signal(SIGINT, cleanup);

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
		fprintf(stderr, "Error: Wrong usage (see %s (-h | --help)) for help\n", argv[0]);
		return 1;
	}

	check_args(argc, argv);
	
	// nz -> number of customers, nu -> number of clerks, tz -> maximum time in ms that a customer waits after creation before entering,
	// tu -> maximum length of a clerk's break in ms, f -> maximum time in ms that the post office is closed to new customers.
	int nz = atoi(argv[1]), nu = atoi(argv[2]), tz = atoi(argv[3]), tu = atoi(argv[4]), f = atoi(argv[5]);
	
	fp = init_file();
	setbuf(fp, NULL);

	for(int i = 0; i < SEM_CNT; i++)
	{
		sem_arr[i] = NULL;
		customer_cnt[i] = NULL;
		cc_mux_arr[i] = NULL;
	}

	for(int i = 0; i < MUX_CNT; i++)
	{
		mux_arr[i] = NULL;
	}

	create_shared_memory();

	*is_closed = false;
	*action_cnt = 0;
	
	// initialized mutexes
	for(int i = 0; i < MUX_CNT; i++)
	{
		if(mux_arr[i] == NULL)
		{
			fprintf(stderr, "Error: Non existant mutex trying to be initialized\n");
			exit(1);
		}

		if(sem_init(mux_arr[i], 1, 1) == -1)
		{
			perror("Error");
			return 1;
		}
	}
	
	// initialized semaphores
	for(int i = 0; i < SEM_CNT; i++)
	{
		if(sem_arr[i] == NULL)
		{
			fprintf(stderr, "Error: Non existant semaphore trying to be initialized\n");
			exit(1);
		}

		if(cc_mux_arr[i] == NULL)
		{
			fprintf(stderr, "Error: Non existant semaphore trying to be initialized\n");
			exit(1);
		}

		if(sem_init(sem_arr[i], 1, 0) == -1)
		{
			perror("Error");
			exit(1);
		}

		if(sem_init(cc_mux_arr[i], 1, 1) == -1)
		{
			perror("Error");
			exit(1);
		}
	}

	int customer_count = 1, clerk_count = 1;
	for(int i = 0; i < nz; i++)
	{
		create_process('Z', &customer_count, tz, tu);
	}

	for(int i = 0; i < nu; i++)
	{
		create_process('U', &clerk_count, tz, tu);
	}
	
	int rand_num = get_rand_num(f/2, f);
	usleep(rand_num);
	
	run_sem_fce(sem_wait, mux_arr[5]);

	*is_closed = true;
	save_to_sem_file("%d: closing\n", action_cnt, -1, -1);

	run_sem_fce(sem_post, mux_arr[5]);

	// Wait for child processes to finish
	while(wait(NULL) > 0);
	
	destroy_sems();
	delete_shared_memory();
	fclose(fp);
}
