#include <string.h>
#include <stdbool.h>
#include "list.h"

// States are ready, running, and blocked
// an ended process won't be displayed
char* states[3];

#define READY states[0]
#define RUNNING states[1]
#define BLOCKED states[2]

// Process Control Block
typedef struct pcb_t {
	int id;
	int priority;
	char* state; // Whether running, blocked, etc.
	int time; // Total time spent as current process
	List* inbox;
	char* reply; // Reply from recipient of a message
}pcb;

// A semaphore
typedef struct sem_t {
	int initVal; // The initial (limit) value of the semaphore
	int val;
	int id;
	List* waitList;
}sem;

typedef struct message_t {
	int senderPid;
	char text[50];
}message;

// Priority ready queues
// zero: greatest, one: medium, two: lowest
// Uses RR pre-emptive process scheduling algo with quantum = N/A
// Processes are appended to the queues and can be removed from any position
List* zero;
List* one;
List* two;

// A list of pcbs waiting on a send operation
List* send;

// A list of pcbs waiting on a receive operation
List* receive;

// A list of semaphores
// A max of 5 semaphores
List* semaphores;

// Init process
// Literally Init Process Control Block
pcb* ipcb;

// A pointer to the currently running process
pcb* current;

// Number of total processes in system
// doesn't include ipcb (init process)
int total;

// Hands out ids for processes upon creation
// not sophisticated at all and not useful for a massive number of processes
int getId();

// Initializes a pcb and puts it on the specified ready queue
// returns NULL if something went wrong
pcb* PCB_init(int priority);

// Initializes the init process
// closes the program if something goes wrong as os-sim cannot run
// without an init process in the background
void IPCB_init();

// Expires quantum and switches to the most worthy process
// if appropriate
// setting flag to 1 tells PCB_next to disregard location of
// the current process: it tells PCB_next to look for the
// first available process starting from level 0 downards
void PCB_next(bool flag);

// Displays the entire state of the system, if there are any processes
// Does not display any info about init process, that must be specified
// with "I 0"
// Messages inside pcbs are not displayed, as that would be too verbose
// instead the number of messages in the inboxes are displayed
void PCB_displayAll();

// Displays info for one process
void PCB_display(pcb* process);

// Forks the named process
// Does not fork init process
void PCB_fork(pcb* process);

_Bool idCompare(void* input, void* compare);

_Bool msgCompare(void* compare, void* input);

_Bool sidCompare(void* compare, void* input);

void PCB_kill(int id);

int PCB_send(int pid, message* msg);

void PCB_receive();

int PCB_reply(int senderPid, char* reply);

void PCB_dump(int id);

void SEM_create(int id, int val);

void SEM_P(int id);

void SEM_V(int id);

void introMessage();

void helpMessage();

void readmeMessage();

void PCB_checkReply();