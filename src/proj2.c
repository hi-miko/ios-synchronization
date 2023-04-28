#include <stdlib.h>
#include <stdio.h>
#include <ctype.h> //call to isdigit
#include <semaphore.h>
#include <string.h>
#include <signal.h> // C-c safeguard
#include <unistd.h> // processes
#include <errno.h>  // TODO for errno, but it might be included already somewhere
#include <sys/mman.h> // mmap + macros
#include <time.h>
#include <stdbool.h>
#include <stdarg.h> // TODO might be already included somewhere

#define SEM_CNT 6
#define ROW_CNT 3

FILE *fp = NULL;
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
	
	// TODO the thought is that it will first clear the file and then open for appending, but I might be able to just open for writing
	fp = fopen(filename, "w");
	if(fp == NULL)
	{
		fprintf(stderr, "Error: file could not be opened\n");
		exit(1);
	}

	return fp;
}
// TODO the things I believe I still need to do:
// think about all the race conditions that could happen and try to figure out a way to fix them
// maybe change the way memory is managed (if it works than just ignore this)
// add doxygen style comments to every function (not that hard, I dont have a lot of functions)
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

	printf("The current sem_getvalue at sem %d is %d\n", index, sem_val);
	fflush(stdout);

	if(sem_val <= 0)
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

void save_to_sem_file(sem_t *sem_arr[], int *action_cnt, char *str, int argc,...)
{
	va_list argv;
	va_start(argv, argc);

	run_sem_fce(sem_wait, sem_arr[5]);
	(*action_cnt)++;
	run_sem_fce(sem_post, sem_arr[5]);

	run_sem_fce(sem_wait, sem_arr[1]);
	vfprintf(fp, str, argv);
	fflush(fp);
	run_sem_fce(sem_post, sem_arr[1]);
}

void customer_logic(int id, sem_t *sem_arr[], bool *is_closed, int *action_cnt, int tz)
{
	save_to_sem_file(sem_arr, action_cnt, "%d: Z %d: started\n", 2, *action_cnt, id);

	int rand_num = get_rand_num(0, tz);
	usleep(rand_num);

	if(*is_closed)
	{
		save_to_sem_file(sem_arr, action_cnt, "%d: Z %d: going home\n", 2, *action_cnt, id);
		return;
	}

	rand_num = get_rand_num(1, 3);
	save_to_sem_file(sem_arr, action_cnt, "%d: Z %d: entering office for a service %d\n", 3, *action_cnt, id, rand_num);

	// rows are indexed as row number + 1
	printf("Customer waiting at row with the index: %d\n", rand_num+1);
	fflush(stdout);

	run_sem_fce(sem_wait, sem_arr[rand_num+1]);
	save_to_sem_file(sem_arr, action_cnt, "%d: Z %d: called by office worker\n", 2, *action_cnt, id);

	rand_num = get_rand_num(0, 10);
	usleep(rand_num);

	save_to_sem_file(sem_arr, action_cnt, "%d: Z %d: going home\n", 2, *action_cnt, id);
}

void clerk_logic(int id, sem_t *sem_arr[], bool *is_closed, int *action_cnt, int tu)
{
	save_to_sem_file(sem_arr, action_cnt, "%d: U %d: started\n", 2, *action_cnt, id);
	
	int non_empty_index = -1, rand_num = 0;
	int index_arr[] = {2, 3, 4};
	while(true)
	{
		non_empty_index = get_nonempty_queue(sem_arr, index_arr, 3);
		if(non_empty_index == -1)
		{
			printf("is closed?(%d)\n", *is_closed);
			fflush(stdout);

			if(*is_closed == true)
			{
				printf("yes its closed\n");
				fflush(stdout);
				save_to_sem_file(sem_arr, action_cnt, "%d: U %d: going home\n", 2, *action_cnt, id);

				return;
			}
			
			printf("no its open\n");
			fflush(stdout);
			save_to_sem_file(sem_arr, action_cnt, "%d: U %d: taking a break\n", 2, *action_cnt, id);

			rand_num = get_rand_num(0, tu);
			usleep(rand_num);
			save_to_sem_file(sem_arr, action_cnt, "%d: U %d: break finished\n", 2, *action_cnt, id);

			continue;
		}

		// indexes of types are offset by 2
		save_to_sem_file(sem_arr, action_cnt, "%d: U %d: serving a service of type %d\n", 3, *action_cnt, id, non_empty_index-2);

		printf("Clerk tending to row with the index: %d\n", non_empty_index);
		fflush(stdout);
		run_sem_fce(sem_post, sem_arr[non_empty_index]);

		rand_num = get_rand_num(0, 10);
		usleep(rand_num);
		save_to_sem_file(sem_arr, action_cnt, "%d: U %d: service finished\n", 2, *action_cnt, id);

		continue;
	}
}

int create_process(char type, int *cnt, sem_t *sem_arr[], bool *is_closed, int *action_cnt, int tz, int tu)
{
	int pid = fork();

	if(pid == 0)
	{
		if(type == 'Z')
		{
			customer_logic(*cnt, sem_arr, is_closed, action_cnt, tz);
		}
		else if(type == 'U')
		{
			clerk_logic(*cnt, sem_arr, is_closed, action_cnt, tu);
		}
		else
		{
			fprintf(stderr, "Error: unknown process type\n");
			exit(1);
		}
		
		fclose(fp);
		sem_post(sem_arr[0]);
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

void destroy_sems(sem_t *sem_arr[])
{
	for(int i = 0; i < SEM_CNT; i++)
	{
		if(sem_arr[i] != NULL)
		{
			run_sem_fce(sem_destroy, sem_arr[i]);
		}
	}
}

//TODO typedef this shit cause I have no idea if even I can understand this
void create_shared_memory(sem_t *(*sem_arr)[], int *(*customer_cnt)[], int **action_cnt, bool **is_closed)
{
	// semaphore creation
	for(int i = 0; i < SEM_CNT; i++)
	{
		//TODO fail?
		if(((*sem_arr)[i] = (sem_t *)mmap(NULL, sizeof(sem_t *), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0)) == (sem_t *)-1)
		{
			perror("Error");
			exit(1);
		}
	}

	// customer count creation
	for(int i = 0; i < SEM_CNT; i++)
	{
		if(((*customer_cnt)[i] = (int *)mmap(NULL, sizeof(int *), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0)) == (int *)-1)
		{
			perror("Error");
			exit(1);
		}
	}

	// is file closed
	if((*is_closed = (bool *)mmap(NULL, sizeof(bool), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0)) == (bool *)-1)
	{
		perror("Error");
		exit(1);
	}

	// count the number of actions
	if((*action_cnt = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0)) == (int *)-1)
	{
		perror("Error");
		exit(1);
	}
}

int main(int argc, char **argv)
{
	// get a pseudo random seed
	srand(time(NULL));

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
	//anything that happens here is thread safe, because no other process will ever get here and its started before any processes are forked
	// [0] -> exit counter, [1] -> writer mutex, [2] -> row/line 1, [3] -> row/line 2, [4] -> row/line3
	sem_t *sem_arr[SEM_CNT];
	int *customer_cnt[ROW_CNT];

	for(int i = 0; i < SEM_CNT; i++)
	{
		sem_arr[i] = NULL;
	}

	for(int i = 0; i < ROW_CNT; i++)
	{
		customer_cnt[i] = NULL;
	}
	
	bool *is_closed = NULL;
	int *action_cnt = NULL;

	create_shared_memory(&sem_arr, &customer_cnt, &action_cnt, &is_closed);

	*is_closed = false;
	*action_cnt = 1;
	
	//TODO maybe think about changing this 2 indents
	for(int i = 0; i < SEM_CNT; i++)
	{
		if(sem_arr[i] == NULL)
		{
			fprintf(stderr, "Error: Non existant semaphore trying to be initialized\n");
			exit(1);
		}

		if((i == 1) || (i == 5))
		{
			if(sem_init(sem_arr[i], 1, 1) == -1)
			{
				perror("Error");
				return 1;
			}
		}
		else
		{
			if(sem_init(sem_arr[i], 1, 0) == -1)
			{
				perror("Error");
				return 1;
			}
		}
	}

	// main program loop
	int customer_count = 1, clerk_cnt = 1;
	for(int i = 0; i < nz; i++)
	{
		create_process('Z', &customer_count, sem_arr, is_closed, action_cnt, tz, tu);
	}

	for(int i = 0; i < nu; i++)
	{
		create_process('U', &clerk_cnt, sem_arr, is_closed, action_cnt, tz, tu);
	}
	
	int rand_num = get_rand_num(f/2, f);
	usleep(rand_num);
	
	printf("Its not closed!\n");
	fflush(stdout);
	*is_closed = true;
	printf("Its CLOSED FUCKERS!\n");
	fflush(stdout);

	save_to_sem_file(sem_arr, action_cnt, "%d: closing\n", 1, *action_cnt);

	// Wait for child processes to finish
	for(int i = 0; i < nu+nz; i++)
	{
		run_sem_fce(sem_wait, sem_arr[0]);	
	}
	
	printf("finished!\n");
	destroy_sems(sem_arr);
	fclose(fp);
}
