#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pcb.h"
#include "list.h"

int main() {
	states[0] = "ready";
	states[1] = "running";
	states[2] = "blocked";
	// Start the init process
	IPCB_init();

	if (ipcb == NULL) {
		printf("ERROR: Init process not created\n");
		return -1;
	}

	// Pointer to currently running process
	// Initialized to init process
	current = ipcb;

	introMessage();

	// Listening for user input
	while (1) {
		PCB_checkReply();
		printf("> ");
		char input[50];
		memset(&input, 0, 50);
		fgets(input, 70, stdin);
		char text[50]; // Text input, used for pids and messages

		switch(input[0]) {
			case'C':
			case'c':
				if (input[1] != ' ') {
					printf("ERROR: Create format is C (priority)\n");
					break;
				}
				else if (input[3] != '\n') {
					printf("ERROR: Create must have a single-digit priority\n");
					break;
				}
				int priority = input[2] - '0';
				if (priority > 2) {
					priority = 2;
				} else if (priority < 0) {
					priority = 0;
				}

				pcb* nBlock = PCB_init(priority);
				if (!nBlock) {
					printf("ERROR: Process not created\n");
					break;
				}

				List* queue;
				if (priority == 0) {
					queue = zero;
				} else if (priority == 1) {
					queue = one;
				} else if (priority == 2) {
					queue = two;
				} else {
					printf("ERROR: priority out of range\n");
					free(nBlock);
					break;
				}

				if (List_append(queue, nBlock) == 0) {
					printf("\nNew pcb id: %d\n", nBlock->id);
				} else {
					printf("ERROR: New pcb could not be created\n\n");
					free(nBlock);
					break;
				}

				if (total == 1 && current->id == 0) {
					PCB_next(1);
				}
				else if (total == 1 && current->id != 0) {
					PCB_next(NULL);
				}

				break;
			case'F':
			case'f':
				if (input[1] != ' ' && input[1] != '\n') {
					printf("\nERROR: Fork format is F\n\n");
					break;
				}
				PCB_fork(current);
				break;
			case'K':
			case'k':
				if (input[1] != ' ') {
					printf("\nERROR: Kill format is K (process id)\n\n");
					break;
				}
				char* curr = input + 2;
				int i = 0;
				while (*curr != '\n') {
					text[i] = *curr;
					i++;
					curr++;
				}
				text[++i] = '\0';
				int kid = atoi(text);
				PCB_kill(kid);
				break;
			case'E':
			case'e':
				if (input[1] != ' ' && input[1] != '\n') {
					printf("\nERROR: End format is E\n");
					break;
				}
				PCB_kill(current->id);
				break;
			case'Q':
			case'q':
				if (input[1] != ' ' && input[1] != '\n') {
					printf("\nERROR: Quantum expire format is Q\n\n");
					break;
				}
				if (current->id == 0) {
					PCB_next(1);
				}
				else {
					PCB_next(NULL);
				}
				break;
			case'S':
			case's':
				if (List_search(send, idCompare, &current->id)) {
					printf("\nERROR: Current process can't send another message at this time\n\n");
					break;
				} else if (List_search(receive, idCompare, &current->id)) {
					printf("\nERROR: Current process is waiting for messages\n\n");
					break;
				}

				if (input[1] != ' ') {
					printf("\nERROR: Send format is S (destination pid) (message)\n\n");
					break;
				}

				// First get the pid
				curr = input + 2;
				i = 0;
				memset(&text, 0, 50);
				while (*curr != ' ' && *curr != '\n') {
					text[i] = *curr;
					i++;
					curr++;
				}
				if (*curr == '\n') {
					printf("\nERROR: No message attached\n");
					printf("Send format is S (destination pid) (message)\n\n");
					break;
				}
				int pid = atoi(text);

				// Now get the message
				memset(&text, 0, 50);
				i = 0;
				curr++;
				while (*curr != '\n') {
					text[i] = *curr;
					i++;
					curr++;
				}
				text[++i] = '\0';

				// Allocate the message, easier to handle it here
				message* msg = (message*)malloc(sizeof(message)); // i is length of written part of text
				strcpy(msg->text, text);
				msg->senderPid = current->id;

				if (PCB_send(pid, msg) == 0) {
					printf("Message successfuly sent\n\n");
				} else {
					printf("ERROR: Message was not sent\n\n");
					free(msg);
				}
				break;
			case'R':
			case'r':
				if (input[1] != ' ' && input[1] != '\n') {
					printf("\nERROR: Receive format is R\n\n");
					break;
				}
				PCB_receive();
				break;
			case'Y':
			case'y':
				if (List_search(send, idCompare, &current->id)) {
					printf("\nERROR: Current process can't send another message at this time\n\n");
					break;
				} else if (List_search(receive, idCompare, &current->id)) {
					printf("\nERROR: Current process is waiting for messages\n\n");
					break;
				}

				if (input[1] != ' ') {
					printf("\nERROR: Reply format is R (destination pid) (message)\n\n");
					break;
				}

				// First get the pid
				curr = input + 2;
				i = 0;
				memset(&text, 0, 50);
				while (*curr != ' ' && *curr != '\n') {
					text[i] = *curr;
					i++;
					curr++;
				}
				if (*curr == '\n') {
					printf("\nERROR: No message attached\n");
					printf("Reply format is R (destination pid) (message)\n\n");
					break;
				}
				pid = atoi(text);

				// Now get the message
				memset(&text, 0, 50);
				i = 0;
				curr++;
				while (*curr != '\n') {
					text[i] = *curr;
					i++;
					curr++;
				}
				text[++i] = '\0';

				// Allocate the message, easier to handle it here
				char* reply = (char*)malloc(i); // i is length of written part of text
				strcpy(reply, text);

				if (PCB_reply(pid, reply) == 0) {
					printf("Message successfuly sent\n\n");
				} else {
					printf("ERROR: Message was not sent\n\n");
				}
				break;
			case'N':
			case'n':
				if (input[1] != ' ') {
					printf("ERROR: New Semaphore format is N (semaphore id) (value)\n");
					break;
				}

				// First get the id
				curr = input + 2;
				i = 0;
				memset(&text, 0, 50);
				while (*curr != ' ' && *curr != '\n') {
					text[i] = *curr;
					i++;
					curr++;
				}
				if (*curr == '\n') {
					printf("ERROR: No value input\n");
					printf("New Semaphore format is N (semaphore id) (value)\n\n");
					break;
				}
				int id = atoi(text);

				// Now get the value
				memset(&text, 0, 50);
				i = 0;
				curr++;
				while (*curr != '\n') {
					text[i] = *curr;
					i++;
					curr++;
				}
				int value = atoi(text);
				SEM_create(id, value);
				break;
			case'P':
			case'p':
				if (input[1] != ' ') {
					printf("ERROR: P format is P (semaphore id)\n\n");
					break;
				}

				// First get the id
				curr = input + 2;
				i = 0;
				memset(&text, 0, 50);
				while (*curr != ' ' && *curr != '\n') {
					text[i] = *curr;
					i++;
					curr++;
				}
				id = text[0] - '0';
				SEM_P(id);
				break;
			case'V':
			case'v':
				if (input[1] != ' ') {
					printf("\nERROR: V format is V (semaphore id)\n\n");
					break;
				}

				// First get the id
				curr = input + 2;
				i = 0;
				memset(&text, 0, 50);
				while (*curr != ' ' && *curr != '\n') {
					text[i] = *curr;
					i++;
					curr++;
				}
				id = text[0] - '0';
				SEM_V(id);
				break;
			case'I':
			case'i':
				if (input[1] != ' ') {
					printf("ERROR: Procinfo format is I (pid)\n\n");
					break;
				}

				curr = input + 2;
				i = 0;
				memset(&text, 0, 50);
				while (*curr != ' ' && *curr != '\n') {
					text[i] = *curr;
					i++;
					curr++;
				}
				pid = atoi(text);

				PCB_dump(pid);
				break;
			case'T':
			case't':
				if (input[1] != ' ' && input[1] != '\n') {
					printf("\nERROR: Totalinfo format is T\n\n");
					break;
				}
				PCB_displayAll();
				break;
			case'!':
				if (input[1] != ' ' && input[1] != '\n') {
					printf("\nERROR: Display Readme format is !\n\n");
					break;
				}
				readmeMessage();
				break;
			case'H':
			case'h':
				if (input[1] != ' ' && input[1] != '\n') {
					printf("\nERROR: Help format is H\n\n");
					break;
				}
				helpMessage();
				break;
			default: 
				printf("ERROR: Input not recognized\n\n");
		}

	}
	return 0;
}