//user.c file
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

//vars
int msgQueue;
int check;
int clockMemoryID;
Clock* sharedClock;
int processNum;


//main
int main(int argc, char *argv[])
{
	//read args
	if(argc < 2)
	{
		printf("inside User Process: incorrect number of args!\n");
		exit(1);	
	}
	
	//get process number from argv
	processNum = atoi(argv[1]);
	printf("Inside P%d\n",processNum);
	
	//vars
	pid_t userID = (long)getpid();
	int totalReq = 0;	
	int maxReq = 100;

	//set up shared memory for clock
	clockMemoryID = shmget(CLOCK_KEY, sizeof(Clock), 0666);
	if(clockMemoryID < 0)
	{
		perror("Inside User Process: Creating clock shared memory Failed!!\n");
		exit(errno);
	}

	//attach clock
	sharedClock = shmat(clockMemoryID, NULL, 0);

	//set up message queue
	msgQueue = msgget(REQ_MSG_KEY,0666);


	//while loop
	while(1)
	{
		Message userMessage;
		if((check = msgrcv(msgQueue, &userMessage, sizeof(Message), processNum, 1)) == -1)
		{		
			printf("User: Failed to get Message!\n");
			return -1;
		}
		else 
		{
			printf("User: Process %d has recieved message from OSS!\n", userMessage.pid);
			totalReq++;
			
			//check termination request
			if(userMessage.terminate == 1)
			{
				return 0;
			}
		}


		//check number of req for termination
		if(totalReq >= maxReq)
		{
			fprintf("User: Process %d has reached its request limit! Sending message to terminate!\n", userMessage.pid);
			
			//create message for termination
			Message newMessage;
			newMessage.type = MASTER;
			newMessage.index = processNum;
			newMessage.terminate = 1;
			newMessage.pid = getpid();
			
			msgsnd(msgQueue, &newMessage, sizeof(Message), 1);
			
			return 0;
		}
		//check page fault chance
		else
		{
			int fault;
			fault = (rand() % 100) + 1;
			
			if(fault > 95)
			{
				fprintf("User: Process %d has faulted! Sending message to terminate!\n", userMessage.pid);
                        
                        	//create message for termination
                        	Message newMessage;
                        	newMessage.type = MASTER;
                                newMessage.index = processNum;
                        	newMessage.terminate = 1;
                        	newMessage.pid = getpid();
                       		msgsnd(msgQueue, &newMessage, sizeof(Message), 1);
			}
			else 
			{
				Message newMessage;
				newMessage.type = MASTER;
				newMessage.index = processNum;
				newMessage.terminate = 0;
				newMessage.ref.pageNumber = processNum;
				newMessage.ref.offset = rand() % 32;
				newMessage.pid = getpid();
				newMessage.dirty = 0;
			
				if(newMessage.ref.offset > 28)
				{
					newMessage.dirty = 1;
				}
				
				printf("User: Process %d is requesting Page# %d with offset: %d from OSS!\n", newMessage.pid, newMessage.ref.pageNumber, newMessage.ref.offset);

				//send message for page request
				msgsnd(msgQueue, &newMessage, sizeof(Message), 1);
			}
		}

	}	





















	return 0;
}
