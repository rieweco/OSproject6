//node.h file
#ifndef NODE_H
#define NODE_H

//shared memory keys
#define CLOCK_KEY 1400
#define PAGE_KEY 1000
#define REQ_MSG_KEY 1337
#define MASTER 200

//struct clock: holds seconds and nanoseconds
typedef struct Clock
{
        int seconds;
        int nanoseconds;
}
Clock;


//struct Page Table: 2d array to hold 18 processes with 32 pages each
typedef struct PageTable
{
	struct Pages pages[18][32];

}
PageTable;

//struct Pages: individual pages- contains frame location, dirty bit flag, and if it is in use
typedef struct Pages
{
	int location;
	int dirty;
	int used;
}
Pages;

//struct for message to pass between master/child.
//contains: pid, index,dirty flag, msgtype,termination flag, ...
typedef struct Message
{
	long type;
	long pid;
	int index;
	int dirty;
	int terminate;
	Reference ref;
}
Message;

//struct for page references
typedef struct Reference
{
	int pageNumber;
	int offset;
}
Reference;

//struct for Frames
typedef struct Frames
{
	int dirty;
	int index;
	int used;
	int pageNumber;
	int offset;
}
Frames;

//queue for suspended processes
typedef struct Queue
{
	int front;
	int rear;
	int size;
	unsigned capacity;
	Message msg;
}
Queue;


#endif
