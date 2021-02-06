make:
	gcc -ggdb -Wall -o os-sim list.o driver.c pcb.c
clean:
	rm os-sim