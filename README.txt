Assignment 3
CMPT 300
Fall 2020
Trevor Bonas

os-sim is a rough os simulation.

A few comments about how os-sim works, the kind of
scheduling scheme used, and some unique approaches
to the assignment:

1.
os-sim uses three priority queues to organize proc-
ess control blocks (pcbs). The queues are not true 
queues, pcbs can be removed from the middle of a 
queue, but any pcb being added to a queue is 
appended.

Each queue allows for different amounts of time a 
pcb can be in it, though these limits are not real-
istic, and 'time' really ends up meaning quantums 
passed. The limits are as follow:

Level 0: 2 quantums
Level 1: 5 quantims
Level 2: limitless

This is an example of how this would work with one 
pcb starting in level 0 (note that since there is 
only one pcb in this example Q doesn't cause control
to be switched to any other pcb, and even after 
switching priorities the pcb is still the running 
process):

> c 0

New pcb id: 1

Switched to pcb 1 in level 0

> q

Not switching

> q

Not switching
pcb 1 has been demoted
Moved pcb 1 to level 1

> q

Not switching

> q

Not switching

> q

Not switching

> q

Not switching

> q

Not switching
pcb 1 has been demoted
Moved pcb 1 to level 2

> q

Not switching

> q

Not switching

When the currently running process switches os-sim 
checks level zero, then one, then finally two, and 
regarding this switch takes into account the level 
the previously running process was in, whether that
old process is now blocked, whether there are any 
processes in the system at all, etc., all to ensure
that the most appropriate pcb runs next.

2.
Regarding message sending, sending and receiving work
the way described in the assignment description exce-
pt when sending. The message, if the recipient is 
found, is put into the recipient's inbox right away,
and when that recipient uses R it is really just show-
ing a message that's already in its inbox. I thought
it would be too convoluted to store messages in an in 
between place and then deliver a message when a pcb 
uses R. pcbs also support holding multiple messages
as their inbox is a List.

Since pcbs can only send one message at a time
each pcb can hold only one reply at a time and
this reply is kept separate from its inbox and is
displayed and freed when said process becomes the
current process.

3.
Semaphores are created by the user with N.

A semaphore's value represents a resource quantity
therefore when a sem value is 0 and P is called
on it the calling pcb is blocked.
If this blocked pcb is ended the sem value is
increased: there is one less person in the lineup
to buy the thing.

4.
Init process is not shown with T; to show init
info you must type "I 0".

5.
If a process send a message to itself it is not
blocked, it simply gets the message, no reply
needed.

6.
Supports upper- and lower-case input.

7.
The priority queues are not really ready queues,
there are pcbs in them that are ready, only
the currently running process will be running, 
but this running process will NOT be removed from
its priority queue. If I were to remove the curr-
ently running process from its queue it would be
pointless, I'm not doing exponential averaging.
Any numbers displayed for the number of pcbs in it
does not exclude the running process, they are more
a reflection of process priorities, how the system
is hierarchical.

8.
The priority queues operate circularly when switching
the currently running process.

9.
For C, creating new pcbs, if the input number is less
than 0, the new process's priority is set to 0, if
it is greater than 2, it is set to 2. So any cases
if typing something like "C 9" and os-sim continuing
like nothing was strange is NOT A BUG. Also the
priority must be a single digit, os-sim will catch
this.

10.
The number of pcbs is limited by the number of List
heads available, which in this system is 100, since
each pcb has a List inbox. Though, ids are not
restricted by this, and it is possible to make
a maximum number of pcbs, end them all, and make
more pcbs with ids greater than 100.

11.
K can end a process wherever it is in the system.
Useful for when there doesn't exist a process
that can get another process unblocked from a 
queue or semaphore.