#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "pcb.h"
#include "list.h"

// Comments for functions are in pcb.h

List* zero;
List* one;
List* two;

List* send;
List* receive;

List* semaphores;

pcb* ipcb;
pcb* current;

int id = 1;

int total = 0;

// Placeholder for returning an id
int getId() {
	int num = id;
	id++;
	return num;
}

_Bool idCompare(void* compare, void* input) {
	return ( *((int*)input) == ((pcb*)(compare))->id );
}

_Bool msgCompare(void* compare, void* input) {
	return ( *((int*)input) == ((message*)(compare))->senderPid );
}

_Bool sidCompare(void* compare, void* input) {
	sem* cmp = (sem*)compare;
	int in = *((int*)input);
	int result = (in == cmp->id);
	return result;
}

void messageFree(void* msg) {
	if (msg) {
		free(msg);
	}
}

void PCB_free(void* proc) {
	if (proc && List_count(((pcb*)proc)->inbox) > 0) {
		List_first(((pcb*)proc)->inbox);
		for (int i = 0; i < List_count(((pcb*)proc)->inbox); i++) {
			messageFree( ((message*)(List_curr(((pcb*)proc)->inbox)))->text);
			List_next(((pcb*)proc)->inbox);
		}
		free(proc);
	}
	else if (proc) {
		free(proc);
	}
	return;
}

void SEM_free() {
	sem* semaphore;
	if (List_count(semaphores) <= 0) {
		return;
	}

	List_first(semaphores);
	for (int i = 0; i < List_count(semaphores); i++) {
		semaphore = List_curr(semaphores);
		List_first(semaphore->waitList);
		List_free(semaphore->waitList, PCB_free);
		List_remove(semaphores);
		free(semaphore);
	}
}

pcb* PCB_init(int priority) {
	pcb* block = (pcb*)malloc(sizeof(pcb));
	if (!block) {
		printf("ERROR: Process could not be allocated on heap\n");
		return NULL;
	}
	block->priority = priority;
	block->inbox = List_create();
	if (block->inbox == NULL) {
		printf("ERROR: process inbox could not be created\n");
		free(block);
		return NULL;
	}
	block->time = 0;
	block->id = getId();
	block->reply = NULL;
	block->state = READY;

	if (priority == 1) {
		block->time = 2;
	}

	total++;

	return block;
}

void IPCB_init() {
	pcb* block = (pcb*)malloc(sizeof(pcb));
	if (!block) {
		printf("CRITICAL FAILURE: Init could not be created\n");
		exit(1);
	}
	block->priority = 2;
	block->inbox = List_create();
	if (!block->inbox) {
		printf("CRITICAL FAILURE: Init inbox could not be created\n");
		exit(1);
	}
	block->id = 0;
	block->reply = NULL;
	block->state = RUNNING;

	// Initialize the queues
	zero = List_create();
	one = List_create();
	two = List_create();
	send = List_create();
	receive = List_create();
	semaphores = List_create();
	if (!zero || !one || !two || !send || !receive) {
		printf("CRITICAL FAILURE: queues could not be created\n");
		exit(1);
	}

	ipcb = block;
}

// For when quantum passes
// Will only switch to a process of the same level of priority as the currently running process
// or higher priority level. If there is only one running process in any level this will
// switch to the init process
void PCB_next(bool flag) {
	List* currQueue;
	pcb* old = current;
	printf("\n");

	// Flag indicates "Switch to any running process, from level 0, to 2"
	// What it really means is for PCB_next to not try to orient itself
	// in any ready queue, i.e., not try to find what process is "next"
	// in a queue, because the current process was removed and there are
	// no other processes in its priority queue or for some other reason
	if (flag) {
		if (List_count(zero) > 0) {
			if (current) {
				current->state = READY;
				current = List_first(zero);
				current->state = RUNNING;
				printf("Switched to pcb %d in level 0\n\n", current->id);
				return;
			}
		}

		if (List_count(one) > 0) {
			if (current) {
				current->state = READY;
				current = List_first(one);
				current->state = RUNNING;
				printf("Switched to pcb %d in level 1\n\n", current->id);
				return;
			}
		}

		if (List_count(two) > 0) {
			if (current) {
				current->state = READY;
				current = List_first(two);
				current->state = RUNNING;
				printf("Switched to pcb %d in level 2\n\n", current->id);
				return;
			}
		}

		if (current) {
			current->state = READY;
		}
		current = ipcb;
		current->state = RUNNING;
		printf("Running init process\n\n");
		return;
	}

	if (current->id == 0 && total > 1 ) {
		PCB_next(1);
		return;
	}

	if (total == 0) {
		printf("No unblocked processes to switch to\n\n");
		current = ipcb;
		current->state = RUNNING;
		return;
	}

	if (current->priority == 0) {
		currQueue = zero;
	}
	else if (current->priority == 1) {
		currQueue = one;
	}
	else if (current->priority == 2) {
		currQueue = two;
	}
	else {
		printf("NEXT ERROR: Priority of current process could not be determined\n\n");
		return;
	}

	// init process doesn't get time advanced because it's never on a priority queue
	current->time++;

	// Level 0 has processing time limit of 2 ns
	// Level 1 has processing time limit of 5 ns
	// Level 2 has no processing time limit

	if (List_count(zero) > 0) {

		// If current is in level 0
		if (currQueue == zero) {
			if (List_count(zero) > 1) {
				current->state = READY;
				// Orient the list
				List_first(zero);
				List_search(zero, idCompare, &current->id);
				if (List_next(zero) == NULL) {
					current = List_first(zero);
				}
				else {
					// Checking List_next advances the list's current item
					// so just set current process to List_curr
					current = List_curr(zero);
				}
				current->state = RUNNING;
				printf("Switched to pcb %d in level 0\n", current->id);
			}
			else {
				printf("Not switching\n");
			}
			if (old->time >= 2) {
				printf("pcb %d has been demoted\n", old->id);
				List_first(zero);
				List_search(zero, idCompare, &old->id);
				List_remove(zero);
				List_append(one, old);
				old->priority = 1;
				printf("Moved pcb %d to level 1\n", old->id);
			}
			printf("\n");
			return;
		}

		// If current is not in level 0 but there are pcbs in level 0
		else {
			current->state = READY;
			current = List_first(zero);
			current->state = RUNNING;
			printf("Switched to pcb %d in level 0\n", current->id);
			printf("\n");
			return;
		}
	}

	else if (List_count(one) > 0) {

		// If current is in level 0
		if (currQueue == one) {
			if (List_count(one) > 1) {
				current->state = READY;
				// Orient the list
				List_first(one);
				List_search(one, idCompare, &current->id);
				if (List_next(one) == NULL) {
					current = List_first(one);
				}
				else {
					// Checking List_next advances the list's current item
					// so just set current process to List_curr
					current = List_curr(one);
				}
				current->state = RUNNING;
				printf("Switched to pcb %d in level 1\n", current->id);
			}
			else {
				printf("Not switching\n");
			}
			if (old->time >= 7) {
				printf("pcb %d has been demoted\n", old->id);
				List_first(one);
				List_search(one, idCompare, &old->id);
				List_remove(one);
				List_append(two, old);
				old->priority = 2;
				printf("Moved pcb %d to level 2\n", old->id);
			}
			printf("\n");
			return;
		}

		// If current is not in level 1 but there are pcbs in level 1
		else {
			current->state = READY;
			current = List_first(one);
			current->state = RUNNING;
			printf("Switched to pcb %d in level 1\n", current->id);
			printf("\n");
			return;
		}
	}

	else if (List_count(two) > 0) {

		// If current is in level 0
		if (currQueue == two) {
			if (List_count(two) > 1) {
				current->state = READY;
				// Orient the list
				List_first(two);
				List_search(two, idCompare, &current->id);
				if (List_next(two) == NULL) {
					current = List_first(two);
				}
				else {
					// Checking List_next advances the list's current item
					// so just set current process to List_curr
					current = List_curr(two);
				}
				current->state = RUNNING;
				printf("Switched to pcb %d in level 2\n\n", current->id);
				return;
			}
			else {
				printf("Not switching\n\n");
				return;
			}
		}

		// If current is not in level 2 but there are pcbs in level 2
		// Would never reach here, but just in case
		else {
			current->state = READY;
			current = List_first(two);
			current->state = RUNNING;
			printf("Switched to pcb %d in level 2\n\n", current->id);
			return;
		}
	}

	printf("NEXT ERROR: Something went wrong\n\n");
	return;
}

// Displays all the information of every process running or not
// Does not print the messages in any of the processes, only the number in inbox
void PCB_displayAll() {
	pcb* printPCB;
	printf("\n###################### SYSTEM STATE ######################\n\n");
	printf("%d Total processes\n\n", total);
	printf("Level 0:\n%d\n", List_count(zero));
	if (List_count(zero) <= 0) {
		printf("\tNo processes in level 0\n\n");
	}
	else {
		List_first(zero);
		for (int i = 0; i < List_count(zero); i++) {
			printPCB = List_curr(zero);
			printf("\tid: %d\n", printPCB->id);
			printf("\tpriority: %d\n", printPCB->priority);
			printf("\tstate: %s\n", printPCB->state);
			printf("\tmessages in inbox: %d\n", List_count(printPCB->inbox));
			printf("\n");

			List_next(zero);
		}
	}

	printf("Level 1:\n%d\n\n", List_count(one));
	if (List_count(one) <= 0) {
		printf("\tNo processes in level 1\n\n");
	}
	else {
		List_first(one);
		for (int i = 0; i < List_count(one); i++) {
			printPCB = List_curr(one);
			printf("\tid: %d\n", printPCB->id);
			printf("\tpriority: %d\n", printPCB->priority);
			printf("\tstate: %s\n", printPCB->state);
			printf("\tmessages in inbox: %d\n", List_count(printPCB->inbox));
			printf("\n");

			List_next(one);
		}
	}

	printf("Level 2:\n%d\n\n", List_count(two));
	if (List_count(two) <= 0) {
		printf("\tNo processes in level 2\n\n");
	}
	else {
		List_first(two);
		for (int i = 0; i < List_count(two); i++) {
			printPCB = List_curr(two);
			printf("\tid: %d\n", printPCB->id);
			printf("\tpriority: %d\n", printPCB->priority);
			printf("\tstate: %s\n", printPCB->state);
			printf("\tmessages in inbox: %d\n", List_count(printPCB->inbox));
			printf("\n");

			List_next(two);
		}
	}

	printf("Send queue:\n%d\n\n", List_count(send));
	if (List_count(send) <= 0) {
		printf("\tNo processes in send queue\n\n");
	}
	else {
		List_first(send);
		for (int i = 0; i < List_count(send); i++) {
			printPCB = List_curr(send);
			printf("\tid: %d\n", printPCB->id);
			printf("\tpriority: %d\n", printPCB->priority);
			printf("\tstate: %s\n", printPCB->state);
			printf("\tmessages in inbox: %d\n", List_count(printPCB->inbox));
			printf("\n");

			List_next(send);
		}
	}

	printf("Receive queue:\n%d\n\n", List_count(receive));
	if (List_count(receive) <= 0) {
		printf("\tNo processes in receive queue\n\n");
	}
	else {
		List_first(receive);
		for (int i = 0; i < List_count(receive); i++) {
			printPCB = List_curr(receive);
			printf("\tid: %d\n", printPCB->id);
			printf("\tpriority: %d\n", printPCB->priority);
			printf("\tstate: %s\n", printPCB->state);
			printf("\tmessages in inbox: %d\n", List_count(printPCB->inbox));
			printf("\n");

			List_next(receive);
		}
	}

	if (List_count(semaphores) > 0) {
		printf("Semaphores\n%d\n\n", List_count(semaphores));
		List_first(semaphores);
		for (int i = 0; i < List_count(semaphores); i++) {
			printf("\tsem id: %d\n", ((sem*)List_curr(semaphores))->id);
			printf("\tsem current value: %d\n", ((sem*)List_curr(semaphores))->val);
			printf("\tpcbs in waitlist: %d\n\n", List_count(((sem*)List_curr(semaphores))->waitList));

			List_next(semaphores);
		}
	}
	else {
		printf("No semaphores\n\n");
	}

	printf("##########################################################\n\n");
}

void PCB_display(pcb* process) {
	if (process == NULL) {
		printf("ERROR: Process to be displayed is NULL\n");
		return;
	}
	printf("\nid: %d\n", process->id);
	printf("priority: %d\n", process->priority);
	printf("state: %s\n", process->state);
	printf("messages in inbox: %d\n", List_count(process->inbox));
	printf("\n");
}

void PCB_fork(pcb* process) {
	printf("\n");
	if (process == NULL) {
		printf("ERROR: Cannot fork process, for it is NULL\n\n");
		return;
	}
	else if (process->id == 0) {
		printf("ERROR: Cannot fork init process\n\n");
		return;
	}
	pcb* copy = (pcb*)malloc(sizeof(pcb));
	if (copy == NULL) {
		printf("ERROR: Copy could not be allocated\n\n");
		return;
	}
	copy->id = getId();

	// Doesn't need to copy contents of the inbox
	copy->inbox = List_create();

	copy->priority = process->priority;
	copy->state = READY;
	copy->time = 0;

	switch(copy->priority) {
		case 0:
			List_append(zero, copy);
			break;
		case 1:
			List_append(one, copy);
			break;
		case 2:
			List_append(two, copy);
			break;
		default:
			printf("ERROR: Something went wrong in fork process, aborting\n\n");
			free(copy);
			return;
	}
	printf("Process successfuly forked\n");
	printf("Forked process id: %d\n", copy->id);
	total++;
	printf("\n");
	return;
}

void PCB_kill(int id) {
	pcb* destroy;
	List* currQueue; // The queue the current process is in
	List* destQueue; // The queue the to-be-ended process is in
	sem* destSem; // The semaphore the queue could be in
	printf("\n");
	if (id == 0 && total > 0) {
		printf("ERROR: Cannot end init process while other processes are in system\n\n");
		return;
	}
	else if (id == 0 && total <= 0) {
		printf("Exiting os-sim\nGoodbye\n");
		List_free(ipcb->inbox, messageFree);
		free(ipcb);
		SEM_free();
		exit(1);
	}

	// Need to find the queue the current process belongs to
	// in case we are killing the current process
	List_first(send);
	List_first(receive);
	List_first(semaphores);
	if (current->priority == 0) {
		currQueue = zero;
	} else if (current->priority == 1) {
		currQueue = one;
	} else if (current->priority == 2) {
		currQueue = two;
	} else {
		printf("KILL ERROR: Something went wrong, check if current is in a ready queue\n\n");
		return;
	}

	// Case where we are ending the current process
	if (current->id == id) {
		destroy = current;

		// Case where the current to-be-destroyed process
		// is the only one in its ready queue
		if (List_count(currQueue) == 1) {
			List_first(currQueue);
			List_search(currQueue, idCompare, &destroy->id);
			if (List_remove(currQueue) == NULL) {
				printf("LIST ERROR: Item could not be removed, try again\n");
				return;
			}
			List_free(destroy->inbox, messageFree);
			if (destroy->reply) {
				free(destroy->reply);
			}
			free(destroy);
			total--;
			PCB_next(1);
			printf("Ended current process\n\n");
			return;
		}

		// Case where the current to-be-destroyed process
		// is NOT the only one in its ready queue
		// PCB_next(NULL) tells PCB_next to orient itself
		// in regard to whre the current process is in its ready
		// queue and to properly transfer to the next process in that queue
		else {
			PCB_next(NULL);
			List_first(currQueue);
			List_search(currQueue, idCompare, &destroy->id);
			if (List_remove(currQueue) == NULL) {
				printf("LIST ERROR: Item could not be removed, try again\n");
				return;
			}
			List_free(destroy->inbox, messageFree);
			if (destroy->reply) {
				free(destroy->reply);
			}
			free(destroy);
			total--;
			printf("Ended current process\n\n");
			return;
		}
	}

	// Searching for the process to be ended
	// List is kind of crappy so all lists have
	// to be set to first to actually search entire list
	List_first(zero);
	List_first(one);
	List_first(two);
	List_first(send);
	List_first(receive);
	List_first(semaphores);
	if (List_search(zero, idCompare, &id)) {
		destQueue = zero;
	}
	else if (List_search(one, idCompare, &id)) {
		destQueue = one;
	}
	else if (List_search(two, idCompare, &id)) {
		destQueue = two;
	}
	else if (List_search(send, idCompare, &id)) {
		destQueue = send;
	}
	else if (List_search(receive, idCompare, &id)) {
		destQueue = receive;
	}
	// Searching for a pcb with id in all semaphores
	else if (List_count(semaphores) > 0 ) {
		for (int i = 0; i < List_count(semaphores); i++) {
			if (List_count( ((sem*)List_curr(semaphores))->waitList) > 0) {
				List_first( ((sem*)List_curr(semaphores))->waitList);
				if (List_search(((sem*)List_curr(semaphores))->waitList, idCompare, &id) ) {
					destQueue = ((sem*)List_curr(semaphores))->waitList;
					destSem = ((sem*)List_curr(semaphores));
					break;
				}
			}
			List_next(semaphores);
		}
	}
	else {
		printf("A process with that id could not be found anywhere\n\n");
		return;
	}

	destroy = List_curr(destQueue);
	if (List_remove(destQueue) == NULL) {
		printf("LIST ERROR: Item could not be removed, try again\n");
		return;
	}
	if (destSem && destQueue == destSem->waitList) {
		destSem->val++;
		printf("Found process removed from a semaphore\n");
		printf("Semaphore value increased\n");
	}
	List_free(destroy->inbox, messageFree);
	free(destroy);
	total--;
	printf("Successfuly found and ended process\n\n");
	return;
}

// A deeper send function with more messy arguments
int mail(List* fromQueue, List* toQueue, int pid, message* msg) {
	pcb* target;
	pcb* sender = current;
	List* returnQueue; // Queue to return recipient process to if it's in receive queue

	printf("In mail\n");
	if (fromQueue == NULL || pid < 0 || msg == NULL) {
		printf("SEND ERROR: Something went wrong sending message\n\n");
		return 1;
	}

	if (pid == current->id) {
		List_append(current->inbox, msg);
		printf("You sent a message to yourself\n\n");
		return 0;
	}

	// If toQueue is passed in as NULL that means send to ipcb, the init process
	if (!toQueue) {
		target = ipcb;
	} else {
		List_first(toQueue);
		target = List_search(toQueue, idCompare, &pid);
	}
	if (target) {
		if (toQueue && target->priority == 0) {
			returnQueue = zero;
		}
		else if (toQueue && target->priority == 1) {
			returnQueue = one;
		}
		else if (toQueue && target->priority == 2) {
			returnQueue = two;
		}
		List_first(fromQueue);
		List_append(target->inbox, msg);
		if (toQueue == receive) {
			List_remove(toQueue);
			List_append(returnQueue, target);
			target->state = READY;
			printf("Recipient now unblocked\n\n");
		}

		if (List_count(fromQueue) <= 1) {
			List_first(fromQueue);
			List_search(fromQueue, idCompare, &sender->id);
			List_remove(fromQueue);
			PCB_next(1);
		} else {
			PCB_next(NULL);
			List_first(fromQueue);
			List_search(fromQueue, idCompare, &sender->id);
			List_remove(fromQueue);
		}
		List_append(send, sender);
		sender->state = BLOCKED;
		return 0;
	}

	printf("ERROR: Something went wrong sending message\n\n");
	return 1;
}

// Function called from main
// Clean interface, the function does some detective work to search
// for the target, etc., then hands off the actual sending to the mail function
int PCB_send(int pid, message* msg) {
	List* currQueue; // The queue the current process is in
	List* destQueue;
	pcb* sender = current;
	int result;

	printf("\n");

	if (pid == ipcb->id && current->id == 0) {
		printf("You sent a message to yourself\n\n");
		List_append(ipcb->inbox, msg);
		return 0;
	}

	// Processes that are in a ready queue are only allowed
	// to send messages, so it is assumed current is in levels 0, 1, or 2
	if (sender->priority == 0) {
		currQueue = zero;
	} else if (sender->priority == 1) {
		currQueue = one;
	} else if (sender->priority == 2) {
		currQueue = two;
	} else {
		printf("ERROR: Something went wrong sending message\n\n");
		return -1;
	}

	List_first(zero);
	List_first(one);
	List_first(two);
	List_first(send);
	List_first(receive);
	List_first(semaphores);
	if (pid == 0) {
		result = mail(currQueue, NULL, pid, msg);
	}
	else if (List_search(zero, idCompare, &pid)) {
		result = mail(currQueue, zero, pid, msg);
	}
	else if (List_search(one, idCompare, &pid)) {
		result = mail(currQueue, one, pid, msg);
	}
	else if (List_search(two, idCompare, &pid)) {
		result = mail(currQueue, two, pid, msg);
	}
	else if (List_search(send, idCompare, &pid)) {
		result = mail(currQueue, send, pid, msg);
	}
	else if (List_search(receive, idCompare, &pid)) {
		result = mail(currQueue, receive, pid, msg);
	}
	// Searching all semaphores for a pcb with recipient id
	// The target pcb being blocked on a semaphore works the same
	// way as if the target were blocked on the send queue,
	// getting a message won't unblock the target pcb 
	else if (List_count(semaphores) > 0 ) {
		int semCount = List_count(semaphores);
		for (int i = 0; i < semCount; i++) {
			sem* currSem = List_curr(semaphores);
			int waitCount = List_count(currSem->waitList);
			if (waitCount > 0) {
				List_first(currSem->waitList);
				if (List_search(currSem->waitList, idCompare, &pid) ) {
					destQueue = currSem->waitList;
					result = mail(currQueue, destQueue, pid, msg);
					break;
				}
			}
			List_next(semaphores);
		}
	}
	else {
		printf("Process with that id could not be found\n\n");
		return -1;
	}

	if (!result) {
		return 0;
	} else {
		printf("Something went wrong\n\n");
		return -1;
	}
}

void PCB_receive() {
	List* currQueue;
	pcb* recipient = current;
	printf("\n");
	if (List_count(current->inbox) > 0) {
		printf("Total messages in inbox: %d\n", List_count(current->inbox));
		printf("Oldest message in inbox:\n\n");
		printf("\t%s\n\n", ((message*)(List_first(current->inbox)))->text);
		printf("From process %d\n\n", ((message*)(List_first(current->inbox)))->senderPid);
		return;
	}

	printf("There are no messages in inbox\n");
	
	if (recipient->priority == 0) {
		currQueue = zero;
	} else if (recipient->priority == 1) {
		currQueue = one;
	} else if (recipient->priority == 2) {
		currQueue = two;
	} else {
		printf("ERROR: Something went wrong, check current process is in a ready queue\n");
		return;
	}

	if (List_count(currQueue) <= 1 && current->id != 0) {
		List_first(currQueue);
		List_search(currQueue, idCompare, &recipient->id);
		List_remove(currQueue);
		PCB_next(1);
		List_append(receive, recipient);
		recipient->state = BLOCKED;
		printf("Current process has been blocked and placed on receive queue\n");
		printf("Process will become unblocked when it receives a message\n\n");
		return;
	} 
	else if (List_count(currQueue) > 1 && current->id != 0) {
		PCB_next(NULL);
		List_first(currQueue);
		List_search(currQueue, idCompare, &recipient->id);
		List_remove(currQueue);
		List_append(receive, recipient);
		recipient->state = BLOCKED;
		printf("Current process has been blocked and placed on receive queue\n");
		printf("Process will become unblocked when it receives a message\n\n");
		return;
	}
	else if (current->id == 0) {
		printf("Running init, process not blocked\n\n");
	}
	else {
		printf("RECEIVE ERROR: Something went wrong\n\n");
		return;
	}
}

int PCB_reply(int senderPid, char* reply) {
	List* returnQueue;
	pcb* sender;
	message* msg;
	
	if (List_count(current->inbox) <= 0) {
		printf("No messages in inbox to reply to\n\n");
		free(reply);
		return -1;
	}

	List_first(current->inbox);
	if (List_search(current->inbox, msgCompare, &senderPid) == NULL) {
		printf("A message from given pid is not in inbox\n\n");
		free(reply);
		return -1;
	}

	if (senderPid == current->id) {
		printf("Message was from yourself\n");
		msg = (message*)List_curr(current->inbox);
		List_remove(current->inbox);
		free(msg);

		// Reply is not needed
		free(reply);

		printf("Message freed\n");
		return 0;
	}

	if (senderPid == 0) {
		msg = (message*)List_curr(current->inbox);
		List_remove(current->inbox);
		free(msg);
		ipcb->reply = reply;
		printf("Reply sent\n\n");
		return 0;
	}

	List_first(send);
	if (List_search(send, idCompare, &senderPid)) {
		sender = List_curr(send);
		List_remove(send);
		if (sender->priority == 0) {
			returnQueue = zero;
		}
		else if (sender->priority == 1) {
			returnQueue = one;
		}
		else if (sender->priority == 2) {
			returnQueue = two;
		}
		List_append(returnQueue, sender);
		sender->state = READY;
		sender->reply = reply;
		printf("Sender %d unblocked from send queue\n\n", sender->id);
		msg = (message*)List_curr(current->inbox);
		List_remove(current->inbox);
		free(msg);
		return 0;
	}
	else {
		printf("Sender could not be found in send queue\n\n");
		free(reply);
		return -1;
	}
}

void PCB_dump(int id) {
	pcb* process;

	List_first(zero);
	List_first(one);
	List_first(two);
	List_first(send);
	List_first(receive);

	if (id == 0) {
		process = ipcb;
	}

	else if (List_search(zero, idCompare, &id)) {
		process = List_curr(zero);
	}
	else if (List_search(one, idCompare, &id)) {
		process = List_curr(one);
	}
	else if (List_search(two, idCompare, &id)) {
		process = List_curr(one);
	}
	else if (List_search(send, idCompare, &id)) {
		process = List_curr(one);
	}
	else if (List_search(receive, idCompare, &id)) {
		process = List_curr(one);
	}
	else if (List_count(semaphores) > 0 ) {
		for (int i = 0; i < List_count(semaphores); i++) {
			if (List_count( ((sem*)List_curr(semaphores))->waitList) > 0) {
				List_first( ((sem*)List_curr(semaphores))->waitList);
				if (List_search(((sem*)List_curr(semaphores))->waitList, idCompare, &id) ) {
					process = List_curr(((sem*)List_curr(semaphores))->waitList);
					break;
				}
			}
			List_next(semaphores);
		}
	}
	else {
		printf("A process with that id could not be found\n");
		return;
	}

	printf("\tid: %d\n", process->id);
	printf("\tpriority: %d\n", process->priority);
	printf("\tstate: %s\n", process->state);
	if (List_count(process->inbox) > 0) {
		printf("All messages:\n");
		List_first(process->inbox);
		for (int i = 0; i < List_count(process->inbox); i++) {
			printf("%s\n", ((message*)List_curr(process->inbox))->text);
			printf("From pcb %d\n", ((message*)List_curr(process->inbox))->senderPid);
			List_next(process->inbox);
		}
	}
	else {
			printf("\tNo messages in pcb inbox\n");
	}

	return;
}

void SEM_create(int id, int val) {
	if (id < 0 || id > 4) {
		printf("SEM ERROR: input id out of range\n");
		return;
	}
	if (val < 1) {
		printf("Value must be greater than 0\n");
		return;
	}

	List_first(semaphores);
	if (List_search(semaphores, sidCompare, &id)) {
		printf("SEM ERROR: A semaphore with that id already exists\n");
		return;
	}

	sem* nSem = (sem*)malloc(sizeof(sem));
	nSem->id = id;
	nSem->initVal = val;
	nSem->val = val;
	nSem->waitList = List_create();

	List_append(semaphores, nSem);
	printf("New semaphore created with id %d\n", nSem->id);
	return;
}

void SEM_P(int id) {
	sem* semaphore;
	List* currQueue;
	pcb* old = current;
	if (id < 0 || id > 4) {
		printf("SEM ERROR: input id out of range\n");
		return;
	}

	List_first(semaphores);
	if (List_search(semaphores, sidCompare, &id) == NULL) {
		printf("SEM ERROR: A semaphore with that id cannot be found\n");
		return;
	}

	semaphore = List_curr(semaphores);
	semaphore->val--;
	if (semaphore->val < 0) {
		if (current->id == 0) {
			semaphore->val++;
			printf("Resource controlled by semaphore could not be granted to init\n");
			return;
		}

		printf("Blocking process %d on semaphore %d\n", current->id, semaphore->id);

		if (current->priority == 0) {
			currQueue = zero;
		}
		else if (current->priority == 1) {
			currQueue = one;
		}
		else if (current->priority == 2) {
			currQueue = two;
		}

		if (List_count(currQueue) > 1) {
			PCB_next(NULL);
			List_first(currQueue);
			List_search(currQueue, idCompare, &old->id);
			List_remove(currQueue);
			List_append(semaphore->waitList, old);
			old->state = BLOCKED;
		}
		else if (List_count(currQueue) <= 1) {
			List_first(currQueue);
			List_search(currQueue, idCompare, &old->id);
			List_remove(currQueue);
			List_append(semaphore->waitList, old);
			old->state = BLOCKED;
			PCB_next(1);
		}
		return;
	}
	else {
		printf("Process not blocked\n");
		return;
	}
	return;
}

void SEM_V(int id) {
	sem* semaphore;
	List* returnQueue;
	pcb* returnPCB;
	printf("\n");
	if (id < 0 || id > 4) {
		printf("SEM ERROR: input id out of range\n\n");
		return;
	}

	List_first(semaphores);
	if (List_search(semaphores, sidCompare, &id) == NULL) {
		printf("SEM ERROR: A semaphore with that id cannot be found\n\n");
		return;
	}

	semaphore = List_curr(semaphores);
	if (semaphore->val + 1 > semaphore->initVal) {
		printf("Semaphore at its maximum resource value\n\n");
		return;
	}
	semaphore->val++;

	if (semaphore->val == 0 && List_count(semaphore->waitList) > 0) {
		returnPCB = List_first(semaphore->waitList);
		List_remove(semaphore->waitList);
		if (returnPCB->priority == 0) {
			returnQueue = zero;
		}
		else if (returnPCB->priority == 1) {
			returnQueue = one;
		}
		else if (returnPCB->priority == 2) {
			returnQueue = two;
		}

		List_append(returnQueue, returnPCB);
		returnPCB->state = READY;
		printf("Unblocked pcb %d from semaphore\n\n", returnPCB->id);
		return;
	}

	printf("No processes unblocked from semaphore\n");
	printf("Number of pcbs on semaphore waitlist: %d\n\n", List_count(semaphore->waitList));
	return;
}

void introMessage() {
	printf("Welcome to os-sim, a rough os simulation\n");
	printf("Created November 2020 by Trevor Bonas\n\n");
	printf("Type 'H' for a list of supported commands\n");
	printf("Type '!' to view the readme\n\n");
}

void helpMessage() {
	char* filename = "help.txt";
	char current;
	FILE* f;

	f = fopen(filename, "r");
	printf("\n");
	if (!f) {
		printf("README ERROR: README.txt could not be opened\n");
		return;
	}

	current = fgetc(f);
	while (current != EOF) {
		printf("%c", current);
		current = fgetc(f);
	}

	fclose(f);
	printf("\n\n");
	return;
}

void readmeMessage() {
	char* filename = "README.txt";
	char current;
	FILE* f;

	f = fopen(filename, "r");
	printf("\n");
	if (!f) {
		printf("README ERROR: README.txt could not be opened\n");
		return;
	}

	current = fgetc(f);
	while (current != EOF) {
		printf("%c", current);
		current = fgetc(f);
	}

	fclose(f);
	printf("\n\n");
	return;
}

void PCB_checkReply() {
	if (current->reply) {
		printf("Reply received:\n");
		printf("%s\n", current->reply);
		free(current->reply);
		current->reply = NULL; // Just in case
	}
}