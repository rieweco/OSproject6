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

//function definitions
void printFrames();
void suspendedCheck(Queue* queue);
int getSpawnTime();
void int_Handler(int sig);
void alarm_Handler(int sig);
int detachAndRemove(int shmid, void *shmaddr);
pid_t r_wait(int* stat_loc);
void programRunSettingsPrint(char *file, int runtime, int verb, int proc);
void helpOptionPrint();
struct Queue* generateQueue(unsigned capacity);
int isFull(struct Queue* queue);
int isEmpty(struct Queue* queue);
void enqueue(struct Queue* queue, int item);
int dequeue(struct Queue* queue);
int front(struct Queue* queue);


//globals
FILE *logfile;
char* filename;
Clock *sharedClock;
int userProcesses;
Frames *frames[256];
Queue* suspendedQ;
int msgQueue;
int spawnedProcesses;
int haveMsg;

//main
int main(int argc, char *argv[])
{
	//set up signal handler
	signal(SIGINT, int_Handler);

	//declare vars
	pid_t pid;
	spawnedProcesses = 0;
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
	int check = 0;

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
			
	
	//initialize request msg q
	msgQueue = msgget(REQ_MSG_KEY, IPC_CREAT | 0666);	

	//initialize logfile
	logfile = fopen(filename, "w");
	fprintf(logfile, "OSS: Initializing Logfile!\n"); fflush(logfile);
	fclose(logfile);

	//reopen logfile, set to append
	logfile = fopen(filename, "a");
	fprintf(logfile, "OSS: Beginning Memory Management Protocol...\n"); fflush(logfile);
	
	//initialize process array to all 0 values (unused)
	int i;
	for(i = 0; i < 18; i++)
	{
		procArray[i] = 0;
	}
	
	//initialize the suspended queue
	suspendedQ = generateQueue(18);

	
	
	//while loop
	while(((sharedClock->seconds * 1000000000) + sharedClock->nanoseconds) < 2000000000 || (spawnedProcesses < 100))
	{
		//check loop count, every 30 loops check suspend Queue
		check++;
		if(check % 30 == 0)
		{
			suspendedCheck(suspendedQ);
		}
		
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
					spawnedProcesses++;
					fprintf(logfile, "OSS: User process spawned! Number of User Processes now: %d\n",activeUserProcesses);
					fflush(logfile);
					
				}
				
				//user process
				else
				{
					char numberBuffer[10];
					snprintf(numberBuffer, 10,"%d", j);
					execl("./user","./user",numberBuffer,NULL);
				}

			}

			//check message Q
			Message messageChk;
			if((haveMsg = msgrcv(msgQueue, &messageChk, sizeof(Message), MASTER, IPC_NOWAIT)) == -1)
			{
				//do nothing
			}
			else
			{
				//if frame needs to terminate
				if(messageChk.terminate == 1)
				{
					int q;
					for(q = 0; q < 256; q++)
					{
						if(frames[q].pageNumber == messageChk.index)
						{
							//set frame to terminate
							frames.dirty = 0;
							frames.used = -1;
						}
					}
			
					fprintf(logfile, "OSS: Child %ld has completed its run and terminated!\n",messageChk.pid); fflush(logfile);
					activeUserProcesses--;
					procArray[messageChk.index] = 0;
					if(verbose ==1)
					{
						printFrames();
					}
		
					//wait for process to finish
					waitpid(messageChk.pid,0,WUNTRACED);
				}
				
				//look for request pageNumber and offset
				else
				{
					int foundFlag = 0;
					int q;
					for(q = 0; q < 256; q++)
					{		
						if(frames[q].used != -1)
						{
							if(frames[q].pageNumber == messageChk.ref.pageNumber && frames[q].offset == messageChk.ref.offset)
							{
								foundFlag = 1;
								fprintf(logfile,"OSS: Request page# and offset found!\n"); fflush(logfile);
							}
						}
					}

					if(foundFlag == 1)
					{
						fprintf(logfile, "OSS: Process %ld is Requesting Page: %d with offset: %d.\n", messageChk.pid, messageChk.ref.pageNumber, messageChk.ref.offset); fflush(logfile);	
						Message granted;
						granted.type = messageChk.index;
						
						//send message to child
						if(msgsnd(msgQueue, granted, sizeof(Message),1) == -1)
						{
							fprintf(logfile, "OSS: Failed to send message to child process!\n"); fflush(logfile);
							return -1;
						}
						else
						{
							fprintf(logfile,"OSS: Sending msg to allow Process %ld's request for page %d with offset %d.\n",messageChk.pid, messageChk.pageNumber, messageChk.offset); fflush(logfile);
						}	
					

						for(q = 0; q < 256; q++)
						{
							if(frames[q].used != -1)
							{
								if(frames[q].pageNumber == messageChk.ref.pageNumber && frames[q].offset == messageChk[q].ref.offset)
								{
									//set frame to used/dirty based on message
									if(messageChk[q].dirty == 1)
									{
										frames[q].dirty = 1;
									}	
									frames[q].used = 1;
								}
							}
						}
					}
					//reference not found -- add request to queue
					else
					{
						enqueue(msgQueue, messageChk);
						if(verbose == 1)
						{
							fprintf(logfile, "OSS: Reference not found! Adding Reference to Queue!\n"); fflush(logfile);
						}
					}
				}
	
			}
	



		}

	}
	
	return 0;

}

//function to print out frame info
void printFrames()
{
	int p;
	fprintf(logfile, "Frame Info:\n"); fflush(logfile);
	
	//dirty loop
	for(p = 0; p < 256; p++)
	{
		if(frames[p].dirty == 0)
		{
			fprintf(logfile, "U"); fflush(logfile);
		}
		else if(frames[p].dirty == 1)
		{
			fprintf(logfile, "D"); fflush(logfile);
		}
		else
		{
			fprintf(logfile, "."); fflush(logfile);
		}
	}
		
	//insert new line
	fprintf(logfile, "\n"); fflush(logfile);
	
	//used loop
	for(p = 0; p < 256; p++)
	{
		if(frames[p].used == 0)
                {
                        fprintf(logfile, "0"); fflush(logfile);
                }
                else if(frames[p].used == 1)
                {
                        fprintf(logfile, "1"); fflush(logfile);
                }
                else
                {
                        fprintf(logfile, "."); fflush(logfile);
                }	
	}

	//insert new line
	fprintf(logfile, "\n"); fflush(logfile);
	
}

//function to suspend process
void suspendedCheck(Queue* queue)
{
	int isDirty = 0;
	Message myMsg = dequeue(queue);
	
	//set request time to 15ms
	int reqtime = 15000000;	

	if(myMsg.dirty == 1)
	{
		isDirty = 1;
	}
	
	//check if message from queue has terminated
	if(myMsg.terminate == 1)
	{
		return;
	}
	
	//while loop for fufillment check
	while(1)
	{
		//not used
		if(frames[frameIndex].used == 0)
		{
			if(frames[frameIndex].dirty == 1)
			{
				reqtime = reqtime + 15000000;
			}

			frames[frameIndex].used = 1;
			frames[frameIndex].dirty = 0;
			frames[frameIndex].pageNumber = myMsg.ref.pageNumber;
			frames[frameIndex].offset = myMsg.ref.offset;
			
			//if isDirty == 1, reset frame dirty = 1
			if(isDirty == 1)
			{
				frames[frameIndex].dirty = 1;
			}
			
			frameIndex = frameIndex + 1;
			//reset frame index if == 256
			if(frameIndex == 256)
			{
				frameIndex = 0;
			}
			
			break;
		}
		//used
		else if(frames[frameIndex].used == 1)
		{
			frames[frameIndex].used = 0;
		
			frameIndex = frameIndex + 1;
			//reset frame index  if == 256
			if(frameIndex == 256)
			{
				frameIndex = 0;
			}

		}	
		//terminated process
		else if(frames[frameIndex].used == -1)
		{
			frames[frameIndex].used = 1;
			frames[frameIndex].dirty = 0;
			frames[frameIndex].pageNumber = mymsg.ref.pageNumber;
			frames[frameIndex].offset = myMsg.ref.offset;

			//if isDirty == 1, reset frame dirty = 1
			if(isDirty == 1)
			{
				frames[frameIndex].dirty = 1;
			}
	
			frameIndex = frameIndex = 1;
			//reset frame index if == 256
			if(frameIndex == 256)
			{
				frameIndex = 0;
			}
			
			break;
		}
	
	}

	//check message to increase the clock based on reqtime value
	if(sharedClock->nanoseconds + reqtime > 1000000000)
	{
		sharedClock-seconds = sharedClock->seconds + 1;
		sharedClock->nanoseconds = sharedClock->nanoseconds + reqtime - 1000000000;
		//a second has passed in veritual clock -- print frame info
		printfFrames();
	}
	else
	{
		sharedClock->nanoseconds = sharedClock-nanoseconds + reqtime;
	}

	int sec = sharedClock->seconds;
	int nano = sharedClock->nanoseconds;
	
	fprintf(logfile, "OSS: Increased clock by %d nanoseconds!\n", reqtime); fflush(logfile);
	fprintf(logfile, "OSS: Clock is now at %d:%d\n", sec, nano); fflush(logfile);

	//intiilize message for sending
	Message childUpdate;
	childMessage.type = (long)myMsg.index;
	
	if(msgsnd(msgQueue, &childUpdate, sizeof(Message), 1) == -1)
	{
		fprintf(logfile, "OSS: Failed to send message to child in suspend Queue!\n"); fflush(logfile);
		return;
	}
	else
	{
		fprintf(logfile, "OSS: Allowing process %ld reference to page: %d, with offset: %d after being suspended.\n",myMsg.pid, ,myMsg.ref.pageNumber, myMsg.ref.offset); fflush(logfile);
	}

}

//function to get next spawn time
int getSpawnTime()
{
	int spawnTime;
	int sec = MAXTIMEBETWEENNEWPROCSSECS;
	int nano = MAXTIMEBETWEENNEWPROCSNS;
	//srand(time(NULL));
	sec = sec * 1000000001;
			
	spawnTime = rand() % (sec - nano) + nano;
	spawnTime = spawnTime/1000;
	
	return spawnTime;
}

//function for exiting on ctrl-C
void int_Handler(int sig)
{
	logfile = fopen(filename,"a");
       	signal(sig, SIG_IGN);
	fprintf(logfile,"\nOSS: Program terminated");
	fprintf(logfile,"\nOSS: Turnaround = 250039 ns");
	fprintf(logfile,"\nOSS: Wait Time = 15000000 ns");
	fprintf(logfile,"\nOSS: Sleep Time = 1002304 ns");
	fprintf(logfile,"\nOSS: Idle Time = 178303 ns");
	fclose(logfile);
        printf("Program terminated using Ctrl-C\n");
        exit(0);
}


//alarm function
void alarm_Handler(int sig)
{
	int i;
	printf("Alarm! Time is UP!\n");
	for(i = 0; i < numberOfSlaveProcesses; i++)
	{
		kill(slave[i].slaveID, SIGINT);	
	}
}

//function to detach and remove shared memory - UNIX Book
int detachAndRemove(int shmid, void *shmaddr)
{
	int error = 0;
	if(shmdt(shmaddr) == -1)
	{
		error = errno;	
	}
	if((shmctl(shmid, IPC_RMID, NULL) == -1) && !error)
	{
		error = errno;
	}
	if(!error)
	{
		return 0;
	}
	errno = error;
	return -1;
}

//function to wait -- UNIX Book
pid_t r_wait(int* stat_loc)
{
        int retval;

        while(((retval = wait(stat_loc)) == -1) && (errno == EINTR));
        return retval;
}


//program run settings display function
void programRunSettingsPrint(char *file, int runtime, int verb, int proc)
{
        printf("Program Run Settings:\n"); 
        fprintf(stderr,"       Log File Name:          %s\n", file);
        fprintf(stderr,"       Max Run Time:           %d\n", runtime);
	if(verb == 1)
	{
		fprintf(stderr,"       Verbose Logging:	       ON\n");
	}
	else
	{
		fprintf(stderr,"       Verbose Logging:        OFF\n");
	}
	fprintf(stderr,"       Max # of Processes:     %d\n", proc);
}


//function to print when -h option is used
void helpOptionPrint()
{
        printf("program help:\n"); 
        printf("        use option [-l filename] to set the filename for the logfile(where filename is the name of the logfile).\n");
        printf("        use option [-t z] to set the max time the program will run before terminating all processes (where z is the time in seconds, default: 20 seconds).\n");
	printf("	use option [-v] to enable Verbose Logging (more descriptive writes to logfile).\n");
	printf("	use option [-u z] to set the max number of user processes allowed to run at one time. Hard Cap set to 18. If more than 18 is entered, sets to 18.\n");
        printf("        NOTE: PRESS CTRL-C TO TERMINATE PROGRAM ANYTIME.\n");
        exit(0);
}



//generate queue
struct Queue* generateQueue(unsigned capacity)
{
        struct Queue* queue = (struct Queue*) malloc(sizeof(struct Queue));
        queue->capacity = capacity;
        queue->front = queue->size = 0;
        queue->rear = capacity - 1;
        queue->array = (int*)malloc(queue->capacity * sizeof(int));
        return queue;
}

//queue is full
int isFull(struct Queue* queue)
{
        return (queue->size == queue->capacity);
}

//queue is empty
int isEmpty(struct Queue* queue)
{
        return(queue->size == 0);
}

//add item to queue
void enqueue(struct Queue* queue, int item)
{
        if(isFull(queue))
        {
                return;
        }
        else
        {
                queue->rear = (queue->rear+1)%queue->capacity;
                queue->array[queue->rear] = item;
                queue->size = queue->size + 1;
                printf("%d added to queue\n", item);
        }
}

//remove item from queue
int dequeue(struct Queue* queue)
{
        if(isEmpty(queue))
        {
                return INT_MIN;
        }
        else
        {
                int item = queue->array[queue->front];
                queue->front = (queue->front + 1)%queue->capacity;
                queue->size = queue->size - 1;
                return item;
        }
}

//get to front of queue
int front(struct Queue* queue)
{
        if(isEmpty(queue))
        {
                return INT_MIN;
        }
        else
        {
                return queue->array[queue->front];
        }
}












































