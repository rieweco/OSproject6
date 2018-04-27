//oss.c file
#include "node.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <getopt.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <limits.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <string.h>

//Macros
#define DEFAULT_FILENAME "logfile"
#define DEFAULT_RUNTIME 3
#define MAX_LINES 100000
#define MAX_RUNNING_PROCESSES 18




//globals
FILE *logfile;
char* filename;
Clock *sharedClock;
int userProcesses;
Frames *frames[256];



//main
int main(int argc, char *argv[])
{
	//set up signal handler
	signal(SIGINT, int_Handler);

	//declare vars
	int spawnTime;
	int spawnSeconds;
	int spawnNanoseconds;
        int clockMemoryID;
        int opt = 0;
        int wait_status;
	int maxProcesses;
        filename = DEFAULT_FILENAME;
        int runtime = DEFAULT_RUNTIME;
	int maxUserProcesses = DEFAULT_USER_PROCESSES;
	
	srand(time(NULL));

	//read command line options
	while((opt = getopt(argc, argv, "hvu:l:t:")) != -1)
        {
                switch(opt)
                {
                        case 'h': printf("option h pressed\n");
                                helpOptionPrint();
                                break;
			case 'u': maxProcesses = atoi(argv[2]);
				if(maxProcesses > 18)
				{
					fprintf(stderr, "Max User Processes set to: %d\n", maxUserProcesses);
				}
				else if(maxProcesses > 0)
				{
					maxUserProcesses = maxProcesses;
					fprintf(stderr, "Max User Processes set to: %d\n", maxUserProcesses);
				}
				else 
					fprintf(stderr, "Max Users must be more than 0!");
					exit(EXIT_FAILURE);
				}
                        case 'l': filename = argv[2];
                                fprintf(stderr, "Log file name set to: %s\n", filename);
                                break;
                        case 't': runtime = atoi(argv[2]);
                                fprintf(stderr, "Max runtime set to: %d\n", runtime);
                                break;	
			case 'v': verboseFlag = 1;
				printf("Verbose Logging style turned **ON**\n");
				break;
                        default: perror("Incorrect Argument! Type -h for help!");
                                exit(EXIT_FAILURE);
                }

        }

	//print out prog settings
	programRunSettingsPrint(filename,runtime,verboseFlag,maxUserProcesses);

	//set up signal handler
	if(signal(SIGALRM, int_Handler) == SIG_ERR)
	{
		perror("SINGAL ERROR: SIGALRM failed catch!\n");
		exit(errno);
	}

	alarm(runtime);

	//set up shared memory for clock
	clockMemoryID = shmget(CLOCK_KEY, sizeof(Clock), IPC_CREAT | 0666);
	if(clockMemoryID < 0)
	{
		perror("Creating clock shared memory Failed!!\n");
		exit(errno);
	}

	//attach clock
	sharedClock = shmat(clockMemoryID, NULL, 0);

	//initialize clock
	sharedClock->seconds = 0;
	sharedClock->nanoseconds = 0;
	
	printf("OSS: CLOCK: sec: %d nano: %d req: %d\n",sharedClock->seconds,sharedClock->nanoseconds,sharedClock->numberOfRequests);

	//initialize logfile
	logfile = fopen(filename, "w");
	fprintf(logfile, "OSS: Initializing Logfile!\n"); fflush(logfile);
	fclose(logfile);

	//reopen logfile, set to append
	logfile = fopen(filename, "a");
	fprintf(logfile, "OSS: Beginning Memory Management Protocol...\n"); fflush(logfile);
	

	

	


















































	return 0;

}











