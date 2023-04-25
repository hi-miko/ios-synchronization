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

	fp = fopen(filename, "a+");
	if(fp == NULL)
	{
		fprintf(stderr, "Error: file could not be opened\n");
		exit(1);
	}

	return fp;
}

void customer_logic(int id)
{
	printf("Hello from customer id: %d\n", id);
}

void clerk_logic(int id)
{
	printf("Hello from clerk id: %d\n", id);
}

int create_process(char type, int *cnt, sem_t *exit_cnt)
{
	int pid = fork();
	if(pid == 0)
	{
		if(type == 'Z')
		{
			usleep(300000);
			customer_logic(*cnt);
		}
		else if(type == 'U')
		{
			usleep(300000);
			clerk_logic(*cnt);
		}
		else
		{
			fprintf(stderr, "Error: unknown process type\n");
			exit(1);
		}
		
		fclose(fp);
		sem_post(exit_cnt);
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
	void *exit_cnt = 0;

	if((exit_cnt = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0)) == (void *)-1)
	{
		perror("Error");
		exit(1);
	}

	exit_cnt = (sem_t *) exit_cnt;

	if(sem_init(exit_cnt, 1, 0) == -1)
	{
		perror("Error");
		return 1;
	}
	
	// main program loop
	int customer_cnt = 1, clerk_cnt = 1;
	for(int i = 0; i < nz; i++)
	{
		int pid1 = create_process('Z', &customer_cnt, exit_cnt);
		printf("Main process created child process 'Z' with pid: %d\n", pid1);
	}

	for(int i = 0; i < nu; i++)
	{
		int pid1 = create_process('U', &clerk_cnt, exit_cnt);
		printf("Main process created child process 'U' with pid: %d\n", pid1);
	}
	
	// TODO delete debug
	int rand_num = get_rand_num(f/2, f);
	printf("Sleeping for %d us\n", rand_num);
	usleep(rand_num);
	
	// semaphore here
	char msg[] = "A: closing\n";
	fwrite(msg, 1, strlen(msg), fp);

	// Wait for child processes to finish
	for(int i = 0; i < nu+nz; i++)
	{
		if(sem_wait(exit_cnt) == -1)
		{
			perror("Error");
			return 1;
		}
	}

	sem_destroy(exit_cnt);
	fclose(fp);
}
