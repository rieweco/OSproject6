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
	pid_t pid;
	int spawnTime;
	int spawnSeconds;
	int spawnNanoseconds;
        int clockMemoryID;
        int opt = 0;
        int wait_status;
	int maxProcesses;
        filename = DEFAULT_FILENAME;
        int runtime = DEFAULT_RUNTIME;
	int maxUserProcesses = MAX_RUNNING_PROCESSES;
	int activeUserProcesses = 0;	
	int procArray[18];

	//statistics vars
	int memAccesses = 0;
	int pageFaults = 0;
	int segFaults = 0;	

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
	
	int i;
	for(i = 0; i < 18; i++)
	{
		procArray[i] = 0;
	}
	
	//while loop
	while(((sharedClock->seconds * 1000000000) + sharedClock->nanoseconds) < 2000000000)
	{
		int currentTime = ((sharedClock->seconds * 1000000000) + sharedClock->nanoseconds);
		//check current time to spawn new user process
		if(currentTime >= spawnTime)
		{
			if(activeUserProcesses < 18)
			{
				int j;
				for(j = 0; j < 18; j++)
				{
					if(procArray[j] == 0)
					{
						printf("Free spot in array. index = %d\n",j);
						procArray[j] = 1;
					}
					else
					{
						printf("No free spots available!\n");
					}
		
				}
				if(verboseFlag == 1)
				{
					fprintf(logfile, "OSS: Time to spawn a new User Process!\n"); fflush(logfile);
				}
				
				
				//spawn user
				pid = fork();
				
				//failed to fork()
				if(pid < 0)
				{
					perror("Failed to fork() user process!!\n");
					return 1;
				}
				
				//parent process
				else if(pid > 0)
				{
					activeUserProcesses++;
					fprintf(logfile, "OSS: User process spawned! Number of User Processes now: %d\n",activeUserProcesses);
					fflush(logfile);
					
				}

				//user process
				else
				{
					char numberBuffer[10];
					snprintf(numberBuffer, 10,"%d", activeUserProcesses);
					execl("./user","./user",numberBuffer,NULL);
				}

				
				
			}	
	
		}
	



		











	}
		

	


















































	return 0;

}











